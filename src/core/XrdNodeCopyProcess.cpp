#include "XrdNodeCopyProcess.h"

namespace {

class NodeProgressHandler : public XrdCl::CopyProgressHandler {
 public:
  NodeProgressHandler(Napi::ThreadSafeFunction tsfn, std::atomic<bool>& cancelFlag)
      : tsfn_(tsfn), cancelFlag_(cancelFlag) {}

  virtual void JobProgress(uint32_t jobNum, uint64_t bytesProcessed, uint64_t bytesTotal) override {
    if (!tsfn_) return;  // Check if TSFN is valid

    tsfn_.BlockingCall(
        [jobNum, bytesProcessed, bytesTotal](Napi::Env env, Napi::Function jsCallback) {
          if (!jsCallback.IsEmpty()) {
            jsCallback.Call(
                {Napi::Number::New(env, jobNum),
                 Napi::Number::New(env, static_cast<double>(bytesProcessed)),
                 Napi::Number::New(env, static_cast<double>(bytesTotal))}
            );
          }
        }
    );
  }

  virtual bool ShouldCancel(uint32_t /*jobNum*/) override { return cancelFlag_.load(); }

 private:
  Napi::ThreadSafeFunction tsfn_;
  std::atomic<bool>& cancelFlag_;
};

class CopyRunWorker : public Napi::AsyncWorker {
 public:
  CopyRunWorker(
      Napi::Env env, Napi::Promise::Deferred deferred, XrdNodeCopyProcess* owner,
      XrdCl::CopyProcess* cp, std::vector<XrdCl::PropertyList*>& results,
      std::atomic<bool>& cancelFlag, const Napi::FunctionReference& progressCb
  )
      : Napi::AsyncWorker(env, "CopyRunWorker"),
        deferred_(deferred),
        owner_(owner),
        cp_(cp),
        results_(results) {
    // Prevent owner from being GC'd while worker is running
    owner_->Ref();

    if (!progressCb.IsEmpty()) {
      tsfn_ = Napi::ThreadSafeFunction::New(env, progressCb.Value(), "XrdCopyProgress", 0, 1);
    }
    handler_ = new NodeProgressHandler(tsfn_, cancelFlag);
  }

  ~CopyRunWorker() override {
    if (tsfn_) {
      tsfn_.Release();
    }
    delete handler_;

    // Allow owner to be GC'd now that worker is done
    owner_->Unref();
  }

  void Execute() override {
    status_ = cp_->Run(handler_);
    if (!status_.IsOK()) {
      SetError(status_.ToString());
    }
  }

  void OnOK() override {
    Napi::Env env = Env();
    Napi::Array jsResults = Napi::Array::New(env, results_.size());

    for (size_t i = 0; i < results_.size(); i++) {
      Napi::Object resObj = Napi::Object::New(env);
      XrdCl::PropertyList* p = results_[i];

      uint64_t size = 0;
      if (p->Get("size", size))
        resObj.Set("size", Napi::Number::New(env, static_cast<double>(size)));

      std::string srcCksm;
      if (p->Get("sourceCheckSum", srcCksm))
        resObj.Set("sourceCheckSum", Napi::String::New(env, srcCksm));

      std::string tgtCksm;
      if (p->Get("targetCheckSum", tgtCksm))
        resObj.Set("targetCheckSum", Napi::String::New(env, tgtCksm));

      std::string realTarget;
      if (p->Get("realTarget", realTarget))
        resObj.Set("realTarget", Napi::String::New(env, realTarget));

      jsResults.Set(i, resObj);
    }

    deferred_.Resolve(jsResults);
  }

  void OnError(const Napi::Error& e) override { deferred_.Reject(e.Value()); }

 private:
  Napi::Promise::Deferred deferred_;
  XrdNodeCopyProcess* owner_;
  XrdCl::CopyProcess* cp_;
  std::vector<XrdCl::PropertyList*>& results_;
  XrdCl::XRootDStatus status_;
  NodeProgressHandler* handler_;
  Napi::ThreadSafeFunction tsfn_;
};

class CopyPrepareWorker : public Napi::AsyncWorker {
 public:
  CopyPrepareWorker(
      Napi::Env env, Napi::Promise::Deferred deferred, XrdNodeCopyProcess* owner,
      XrdCl::CopyProcess* cp
  )
      : Napi::AsyncWorker(env, "CopyPrepareWorker"), deferred_(deferred), owner_(owner), cp_(cp) {
    owner_->Ref();
  }

  ~CopyPrepareWorker() override { owner_->Unref(); }

  void Execute() override {
    status_ = cp_->Prepare();
    if (!status_.IsOK()) {
      SetError(status_.ToString());
    }
  }

  void OnOK() override { deferred_.Resolve(Env().Undefined()); }

  void OnError(const Napi::Error& e) override { deferred_.Reject(e.Value()); }

 private:
  Napi::Promise::Deferred deferred_;
  XrdNodeCopyProcess* owner_;
  XrdCl::CopyProcess* cp_;
  XrdCl::XRootDStatus status_;
};
}  // namespace

Napi::Object XrdNodeCopyProcess::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
      env,
      "CopyProcess",
      {InstanceMethod("AddJob", &XrdNodeCopyProcess::AddJob),
       InstanceMethod("Prepare", &XrdNodeCopyProcess::Prepare),
       InstanceMethod("Run", &XrdNodeCopyProcess::Run),
       InstanceMethod("CancelJob", &XrdNodeCopyProcess::CancelJob),
       InstanceMethod("SetEventListener", &XrdNodeCopyProcess::SetEventListener)}
  );

  exports.Set("CopyProcess", func);

  return exports;
}

XrdNodeCopyProcess::XrdNodeCopyProcess(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<XrdNodeCopyProcess>(info) {
  cp_ = new XrdCl::CopyProcess();
}

XrdNodeCopyProcess::~XrdNodeCopyProcess() {
  if (cp_) {
    delete cp_;
    cp_ = nullptr;
  }
  for (auto* prop : jobResults_) {
    delete prop;
  }
  jobResults_.clear();
}

Napi::Value XrdNodeCopyProcess::AddJob(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::TypeError::New(env, "Expected job config object").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Object config = info[0].As<Napi::Object>();
  XrdCl::PropertyList props;

  if (config.Has("source"))
    props.Set("source", config.Get("source").As<Napi::String>().Utf8Value());
  if (config.Has("target"))
    props.Set("target", config.Get("target").As<Napi::String>().Utf8Value());
  if (config.Has("force")) props.Set("force", config.Get("force").As<Napi::Boolean>().Value());
  if (config.Has("makeDir"))
    props.Set("makeDir", config.Get("makeDir").As<Napi::Boolean>().Value());
  if (config.Has("chunkSize"))
    props.Set("chunkSize", config.Get("chunkSize").As<Napi::Number>().Uint32Value());
  if (config.Has("parallelChunks"))
    props.Set(
        "parallelChunks",
        static_cast<uint8_t>(config.Get("parallelChunks").As<Napi::Number>().Uint32Value())
    );

  XrdCl::PropertyList* resultProps = new XrdCl::PropertyList();
  jobResults_.push_back(resultProps);

  XrdCl::XRootDStatus status = cp_->AddJob(props, resultProps);
  if (!status.IsOK()) {
    Napi::Error::New(env, status.ToString()).ThrowAsJavaScriptException();
    return env.Null();
  }
  return env.Undefined();
}

Napi::Value XrdNodeCopyProcess::Prepare(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto deferred = Napi::Promise::Deferred::New(env);

  auto* worker = new CopyPrepareWorker(env, deferred, this, cp_);
  worker->Queue();

  return deferred.Promise();
}

Napi::Value XrdNodeCopyProcess::Run(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto deferred = Napi::Promise::Deferred::New(env);

  auto* worker =
      new CopyRunWorker(env, deferred, this, cp_, jobResults_, isCancelled_, progressCallback_);
  worker->Queue();

  return deferred.Promise();
}

Napi::Value XrdNodeCopyProcess::CancelJob(const Napi::CallbackInfo& info) {
  isCancelled_.store(true);
  return info.Env().Undefined();
}

Napi::Value XrdNodeCopyProcess::SetEventListener(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction()) {
    Napi::TypeError::New(env, "Expected (eventName: string, listener: function)")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string eventName = info[0].As<Napi::String>().Utf8Value();
  if (eventName == "progress") {
    progressCallback_ = Napi::Persistent(info[1].As<Napi::Function>());
  }

  return env.Undefined();
}
