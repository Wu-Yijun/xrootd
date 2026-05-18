#include "XrdNodeCopyProcess.h"

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
  // TODO
}

XrdNodeCopyProcess::~XrdNodeCopyProcess() {
  // TODO
}

Napi::Value XrdNodeCopyProcess::AddJob(const Napi::CallbackInfo& info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeCopyProcess::Prepare(const Napi::CallbackInfo& info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeCopyProcess::Run(const Napi::CallbackInfo& info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeCopyProcess::CancelJob(const Napi::CallbackInfo& info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeCopyProcess::SetEventListener(const Napi::CallbackInfo& info) {
  // TODO
  return Napi::Value();
}
