// src/handlers/FSBufferHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

class FSBufferHandler : public XrdCl::ResponseHandler {
 public:
  FSBufferHandler(Napi::Env env, Napi::Promise::Deferred deferred, const std::string& opName)
      : deferred_(deferred), opName_(opName) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        ("XRootD_FS_" + opName_).c_str(),
        0,
        1
    );
  }

  virtual ~FSBufferHandler() { tsfn_.Release(); }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    XrdCl::Buffer* xrdBuffer = nullptr;
    if (status->IsOK() && response) {
      response->Get(xrdBuffer);
    }

    tsfn_.BlockingCall(
        [this, status = *status, xrdBuffer](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK() && xrdBuffer) {
            // 使用 Copy 确保数据被复制到 V8 堆内存中
            Napi::Buffer<char> jsBuf =
                Napi::Buffer<char>::Copy(env, xrdBuffer->GetBuffer(), xrdBuffer->GetSize());
            this->deferred_.Resolve(jsBuf);
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          delete xrdBuffer;
          this->tsfn_.Release();
          delete this;
        }
    );
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  std::string opName_;
};

}  // namespace XrdNode
