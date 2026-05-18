// src/handlers/FileWriteHandler.h
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>

#include "utils/type_conversions.h"

namespace XrdNode {

class FileWriteHandler : public XrdCl::ResponseHandler {
 public:
  FileWriteHandler(Napi::Env env, Napi::Promise::Deferred deferred, Napi::Buffer<char> jsBuffer)
      : deferred_(deferred) {
    // 核心：强引用 (Pin) JS 的 Buffer！
    // 这就相当于在 V8 里给这个对象加上了 "不可回收" 的锁
    bufferRef_ = Napi::Persistent(jsBuffer.As<Napi::Object>());

    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), "XRootD_FileWrite", 0, 1
    );
  }

  virtual ~FileWriteHandler() { tsfn_.Release(); }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    // Write 操作的 response 通常为空，我们只关心 status

    tsfn_.BlockingCall([this, status = *status](Napi::Env env, Napi::Function /*jsCallback*/) {
      if (status.IsOK()) {
        this->deferred_.Resolve(env.Undefined());
      } else {
        Napi::Error err = Utils::StatusToError(env, status);
        this->deferred_.Reject(err.Value());
      }

      // 核心：释放强引用，允许 V8 GC 回收那个 Buffer
      this->bufferRef_.Reset();

      this->tsfn_.Release();
      delete this;
    });
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  // 使用 ObjectReference 持有 Buffer，防止底层 I/O 期间被回收
  Napi::ObjectReference bufferRef_;
};

}  // namespace XrdNode