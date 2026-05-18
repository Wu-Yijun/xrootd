// src/handlers/FileControlHandler.h
#pragma once
#include "../utils/type_conversions.h" // 假设包含之前的 StatusToError 工具
#include <XrdCl/XrdClFile.hh>
#include <napi.h>

namespace XrdNode {

class FileControlHandler : public XrdCl::ResponseHandler {
public:
  FileControlHandler(Napi::Env env, Napi::Promise::Deferred deferred,
                     const std::string &opName)
      : deferred_(deferred), opName_(opName) {

    // 创建线程安全函数，用于从 XRootD 异步网络线程跳回 V8 主线程
    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo &) {}),
        ("XRootD_" + opName_).c_str(), 0, 1);
  }

  virtual ~FileControlHandler() { tsfn_.Release(); }

  // XRootD 异步操作完成时，由底层网络线程触发
  virtual void HandleResponse(XrdCl::XRootDStatus *status,
                              XrdCl::AnyObject * /*response*/) override {

    tsfn_.BlockingCall([this, statusStr = status->ToString(),
                        isOk = status->IsOK(), statusCode = status->code,
                        statusErr = status->errNo](
                           Napi::Env env, Napi::Function /*jsCallback*/) {
      if (isOk) {
        // 控制类操作成功，直接 Resolve undefined
        this->deferred_.Resolve(env.Undefined());
      } else {
        // 失败时，组装包含底层错误码的 JS Error
        Napi::Error err =
            Napi::Error::New(env, "[" + this->opName_ + " FATAL] " + statusStr);
        Napi::Object errObj = err.Value();
        errObj.Set("code", Napi::Number::New(env, statusCode));
        errObj.Set("errNo", Napi::Number::New(env, statusErr));

        this->deferred_.Reject(errObj);
      }

      // 释放计数，允许 Node 进程正常退出，并销毁自身防泄漏
      this->tsfn_.Release();
      delete this;
    });
  }

private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  std::string opName_;
};

} // namespace XrdNode