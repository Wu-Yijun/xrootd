// src/handlers/FSBufferHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

/**
 * @brief 文件系统返回二进制数据流操作的异步回调处理器 (Handler)。
 * @details 适用于底层响应包含 XrdCl::Buffer 的操作（如 Query, SendInfo, SendCache, Prepare 等）。
 *
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象、操作名称 opName。
 * - **内部处理**：从底层的 AnyObject 提取出 XrdCl::Buffer 指针，通过 TSFN 跳回 V8 主线程后，使用
 * Napi::Buffer::Copy 安全地将二进制内容深拷贝到 V8 堆中，随后释放底层 Buffer、释放 TSFN 并 delete
 * this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve `Napi::Buffer<char>`，失败 Reject `Napi::Error`。
 */
class FSBufferHandler : public XrdCl::ResponseHandler {  // TODO: need check
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
