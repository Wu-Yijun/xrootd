// src/handlers/FileControlHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

class FileControlHandler : public XrdCl::ResponseHandler {
 public:
  FileControlHandler(Napi::Env env, Napi::Promise::Deferred deferred, const std::string &opName)
      : deferred_(deferred), opName_(opName) {
    // 创建线程安全函数，用于从 XRootD 异步网络线程跳回 V8 主线程
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo &) {}),
        ("XRootD_" + opName_).c_str(),
        0,
        1
    );
  }

  virtual ~FileControlHandler() { tsfn_.Release(); }

  // XRootD 异步操作完成时，由底层网络线程触发
  virtual void HandleResponse(XrdCl::XRootDStatus *status, XrdCl::AnyObject * /*response*/)
      override {
    tsfn_.BlockingCall([this, status = *status](Napi::Env env, Napi::Function /*jsCallback*/) {
      if (status.IsOK()) {
        // 控制类操作成功，直接 Resolve undefined
        this->deferred_.Resolve(env.Undefined());
      } else {
        // 失败时，组装包含底层错误码的 JS Error
        Napi::Error err = Utils::StatusToError(env, status);
        this->deferred_.Reject(err.Value());
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

}  // namespace XrdNode