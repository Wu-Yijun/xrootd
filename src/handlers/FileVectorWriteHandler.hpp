// src/handlers/FileVectorWriteHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>
#include <vector>

#include "../utils/type_conversions.h"

namespace XrdNode {

/**
 * @brief 文件多块并发写入 (VectorWrite / WriteV) 操作的异步回调处理器。
 * @details 负责锁定多个 JS 传入的 Buffer，确保底层 I/O 期间内存不被 GC 回收。
 */
class FileVectorWriteHandler : public XrdCl::ResponseHandler {
 public:
  FileVectorWriteHandler(Napi::Env env, Napi::Promise::Deferred deferred)
      : deferred_(deferred) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), "XRootD_FileVectorWrite", 0, 1
    );
  }

  virtual ~FileVectorWriteHandler() {
    tsfn_.Release();
    // 析构时显式释放所有的强引用锁定
    for (auto& ref : locked_buffers_) {
      if (!ref.IsEmpty()) {
        ref.Reset();
      }
    }
  }

  // 为 VectorWrite 使用
  XrdCl::ChunkList& GetChunkList() { return chunkList_; }
  void AddChunk(uint64_t offset, uint32_t size, Napi::Buffer<char> jsBuffer) {
    locked_buffers_.push_back(Napi::Persistent(jsBuffer.As<Napi::Object>()));
    chunkList_.push_back(XrdCl::ChunkInfo(offset, size, jsBuffer.Data()));
  }

  // 为 WriteV 使用
  std::vector<struct iovec>& GetIovecList() { return iovecList_; }
  void AddIovec(Napi::Buffer<char> jsBuffer) {
    locked_buffers_.push_back(Napi::Persistent(jsBuffer.As<Napi::Object>()));
    struct iovec iov;
    iov.iov_base = jsBuffer.Data();
    iov.iov_len = jsBuffer.Length();
    iovecList_.push_back(iov);
  }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    napi_status callStatus = tsfn_.BlockingCall(
        [this, status = *status](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK()) {
            this->deferred_.Resolve(env.Undefined());
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          // 清理操作将在 ~FileVectorWriteHandler 中自动执行
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
  std::vector<Napi::ObjectReference> locked_buffers_;
  
  XrdCl::ChunkList chunkList_;
  std::vector<struct iovec> iovecList_;
};

}  // namespace XrdNode
