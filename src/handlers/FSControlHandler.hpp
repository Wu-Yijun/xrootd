// src/handlers/FSControlHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

/**
 * @brief 文件系统通用控制类操作的异步回调处理器 (Handler)。
 * @details 适用于不需要返回具体数据载荷的操作（如 Rm, MkDir, RmDir, ChMod, Mv, Truncate, Ping
 * 等）。
 *
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象、操作名称 opName。
 * - **内部处理**：通过 Napi::ThreadSafeFunction 将后台网络线程跳回 V8
 * 主线程；如果操作成功则忽略底层 AnyObject 数据，失败则转换错误码。完成后释放 TSFN 并 delete this
 * 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve `undefined`，失败 Reject `Napi::Error`。
 */
class FSControlHandler : public XrdCl::ResponseHandler {
 public:
  FSControlHandler(Napi::Env env, Napi::Promise::Deferred deferred, const std::string& opName)
      : deferred_(deferred), opName_(opName) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        ("XRootD_FS_" + opName_).c_str(),
        0,
        1
    );
  }

  virtual ~FSControlHandler() { tsfn_.Release(); }

  virtual void
  HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* /*response*/) override {
    napi_status callStatus =
        tsfn_.BlockingCall([this, status = *status](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK()) {
            this->deferred_.Resolve(env.Undefined());
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
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
  std::string opName_;
};

}  // namespace XrdNode
