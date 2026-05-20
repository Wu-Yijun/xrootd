// src/handlers/FilePgWriteHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>
#include <vector>

#include "../utils/type_conversions.h"

namespace XrdNode {

/**
 * @brief 文件页写入 (PgWrite) 操作的异步回调处理器。
 * @details 锁定 Buffer，并持有 checksums 数组生命周期直至底层写入完成。
 */
class FilePgWriteHandler : public XrdCl::ResponseHandler {
 public:
  FilePgWriteHandler(
      Napi::Env env, Napi::Promise::Deferred deferred, Napi::Buffer<char> jsBuffer,
      const std::vector<uint32_t>& cksums
  )
      : deferred_(deferred), cksums_(cksums) {
    bufferRef_ = Napi::Persistent(jsBuffer.As<Napi::Object>());
    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), "XRootD_FilePgWrite", 0, 1
    );
  }

  virtual ~FilePgWriteHandler() {
    tsfn_.Release();
    if (!bufferRef_.IsEmpty()) {
      bufferRef_.Reset();
    }
  }

  std::vector<uint32_t>& GetCksums() { return cksums_; }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    napi_status callStatus =
        tsfn_.BlockingCall([this, status = *status](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK()) {
            this->deferred_.Resolve(env.Undefined());
          } else {
            Napi::Error err = Utils::StatusToOkError(env, status);
            this->deferred_.Reject(err.Value());
          }
          delete this;
        });

    if (callStatus != napi_ok) {
      delete this;
    }
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  Napi::ObjectReference bufferRef_;
  std::vector<uint32_t> cksums_;
};

}  // namespace XrdNode
