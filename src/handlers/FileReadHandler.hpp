// src/handlers/FileReadHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

class FileReadHandler : public XrdCl::ResponseHandler {
 public:
  FileReadHandler(Napi::Env env, Napi::Promise::Deferred deferred, uint32_t requestSize)
      : deferred_(deferred), requestSize_(requestSize) {
    // 1. 在发起请求前，就在 C++ 层分配好内存
    buffer_ = new char[requestSize];

    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo &) {}), "XRootD_FileRead", 0, 1
    );
  }

  virtual ~FileReadHandler() { tsfn_.Release(); }

  // 获取底层内存指针供 XRootD 使用
  char *GetBuffer() const { return buffer_; }

  virtual void HandleResponse(XrdCl::XRootDStatus *status, XrdCl::AnyObject *response) override {
    uint32_t bytesRead = 0;

    // 解析实际读取的字节数 (处理 EOF 的关键)
    if (status->IsOK() && response) {
      XrdCl::ChunkInfo *chunkInfo = nullptr;
      response->Get(chunkInfo);
      if (chunkInfo) {
        bytesRead = chunkInfo->length;
        delete chunkInfo;  // 释放 XRootD 返回的元数据结构
      }
    }

    tsfn_.BlockingCall(
        [this, statusCopy = *status, bytesRead](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (statusCopy.IsOK()) {
            // 核心：零拷贝包装！
            // 我们把 C++ 的 buffer_ 直接塞给 Napi::Buffer，并告诉它在被 GC
            // 时执行 delete[]
            Napi::Buffer<char> jsBuffer = Napi::Buffer<char>::New(
                env,
                this->buffer_,
                bytesRead,  // 注意：这里用实际读取的长度，而不是请求的长度
                [](Napi::Env, char *finalizeData) {
                  delete[] finalizeData;  // V8 垃圾回收时触发
                }
            );
            this->deferred_.Resolve(jsBuffer);
          } else {
            // 如果失败，依然需要手动清理我们自己申请的内存，防泄漏
            delete[] this->buffer_;
            Napi::Error err = Utils::StatusToError(env, statusCopy);
            this->deferred_.Reject(err.Value());
          }

          // 释放计数，允许 Node 进程正常退出，并销毁自身防泄漏
          this->tsfn_.Release();
          delete this;
        }
    );
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  char *buffer_;
  uint32_t requestSize_;
};

}  // namespace XrdNode