// src/handlers/FileReadHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

/**
 * @brief 单文件分块读取 (Read) 操作的异步回调处理器 (Handler)。
 * @details 专用于处理从 XrdCl::File 读取指定长度数据块的零拷贝传输。
 *
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象、请求读取的字节数
 * requestSize。
 * - **内部处理**：在 C++ 层预先分配好 buffer_ 内存；底层响应到达后提取 ChunkInfo 获取实际读取长度
 * bytesRead；通过 TSFN 跳回 V8 主线程后，使用 Napi::Buffer::New
 * 实现外部内存的零拷贝绑定，并挂载析构回调（在 V8 GC 回收该 Buffer 时自动触发 delete[]
 * buffer_）。完成后释放 TSFN 并 delete this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve 包含实际读取数据的 `Napi::Buffer<char>`，失败 Reject
 * `Napi::Error` 并在 Reject 前释放预分配的内存。
 */
class FileReadHandler : public XrdCl::ResponseHandler {
 public:
  FileReadHandler(Napi::Env env, Napi::Promise::Deferred deferred, uint32_t requestSize)
      : deferred_(deferred), requestSize_(requestSize), buffer_transferred_(false) {
    // 1. 在发起请求前，就在 C++ 层分配好内存
    buffer_ = new char[requestSize];

    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), "XRootD_FileRead", 0, 1
    );
  }

  virtual ~FileReadHandler() {
    tsfn_.Release();
    if (buffer_ && !buffer_transferred_) {
      delete[] buffer_;
      buffer_ = nullptr;
    }
  }

  // 获取底层内存指针供 XRootD 使用
  char* GetBuffer() const { return buffer_; }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    uint32_t bytesRead = 0;

    // 解析实际读取的字节数 (处理 EOF 的关键)
    if (status->IsOK() && response) {
      XrdCl::ChunkInfo* chunkInfo = nullptr;
      response->Get(chunkInfo);
      if (chunkInfo) {
        bytesRead = chunkInfo->length;
        delete chunkInfo;  // 释放 XRootD 返回的元数据结构
      }
    }

    napi_status callStatus = tsfn_.BlockingCall(
        [this, status = *status, bytesRead](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK()) {
            // 核心：零拷贝包装！
            // 我们把 C++ 的 buffer_ 直接塞给 Napi::Buffer，并告诉它在被 GC
            // 时执行 delete[]
            this->buffer_transferred_ = true;
            Napi::Buffer<char> jsBuffer = Napi::Buffer<char>::New(
                env,
                this->buffer_,
                bytesRead,  // 注意：这里用实际读取的长度，而不是请求的长度
                [](Napi::Env, char* finalizeData) {
                  delete[] finalizeData;  // V8 垃圾回收时触发
                }
            );
            this->deferred_.Resolve(jsBuffer);
          } else {
            // 如果失败，防泄漏，析构中也会处理
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          // 销毁自身防泄漏 (析构函数处理 tsfn_.Release())
          delete this;
        }
    );

    if (callStatus != napi_ok) {
      delete this;
    }
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  char* buffer_;
  uint32_t requestSize_;
  bool buffer_transferred_;
};

}  // namespace XrdNode