// src/handlers/FileWriteHandler.h
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>

#include "utils/type_conversions.h"

namespace XrdNode {

/**
 * @brief 单文件分块写入 (Write) 操作的异步回调处理器 (Handler)。
 * @details 专用于处理向 XrdCl::File 写入指定 Buffer 数据的异步传输与内存保持。
 *
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象、传入的 JS 内存对象
 * Napi::Buffer<char>。
 * - **内部处理**：构造时通过 Napi::Persistent 强引用 (Pin) JS 传入的 Buffer 对象，防止在底层异步
 * I/O 期间被 V8 垃圾回收；通过 TSFN 跳回 V8 主线程后，重置 Persistent 引用锁以恢复正常 GC
 * 机制。完成后释放 TSFN 并 delete this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve `undefined`，失败 Reject `Napi::Error`。
 */
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

    napi_status callStatus =
        tsfn_.BlockingCall([this, status = *status](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK()) {
            this->deferred_.Resolve(env.Undefined());
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          // 核心：释放强引用，允许 V8 GC 回收那个 Buffer
          this->bufferRef_.Reset();
          delete this;
        });

    if (callStatus != napi_ok) {  // TODO: discuss bufferRef
      // 在底层线程被丢弃时，重置引用可能不安全，但在析构中会隐式清理
      // 因此 delete this 足矣
      delete this;
    }
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  // 使用 ObjectReference 持有 Buffer，防止底层 I/O 期间被回收
  Napi::ObjectReference bufferRef_;
};

}  // namespace XrdNode