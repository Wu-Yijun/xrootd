// src/handlers/FileVectorReadHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>
#include <vector>

#include "../utils/type_conversions.h"

namespace XrdNode {

/**
 * @brief 文件多块连续预读/分散读取 (VectorRead) 操作的异步回调处理器 (Handler)。
 * @details 处理 XrdCl::File::VectorRead。返回 Promise<Buffer[]>。
 */
class FileVectorReadHandler : public XrdCl::ResponseHandler {
 public:
  FileVectorReadHandler(Napi::Env env, Napi::Promise::Deferred deferred)
      : deferred_(deferred), chunks_transferred_(false) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "XRootD_FileVectorRead",
        0,
        1
    );
  }

  virtual ~FileVectorReadHandler() {
    tsfn_.Release();
    // 防漏：如果在转移给 V8 之前失败了，需要清理分配的内存
    if (!chunks_transferred_) {
      for (char* buf : allocated_buffers_) {
        delete[] buf;
      }
      allocated_buffers_.clear();
    }
  }

  XrdCl::ChunkList& GetChunkList() { return chunkList_; }

  void AddChunk(uint64_t offset, uint32_t size) {
    char* buf = new char[size];
    allocated_buffers_.push_back(buf);
    chunkList_.push_back(XrdCl::ChunkInfo(offset, size, buf));
  }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    XrdCl::VectorReadInfo* vReadInfo = nullptr;
    if (status->IsOK() && response) {
      response->Get(vReadInfo);
    }

    napi_status callStatus = tsfn_.BlockingCall(
        [this, status = *status, vReadInfo](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK() && vReadInfo) {
            // 标记所有权转移，防止析构时执行 double free
            this->chunks_transferred_ = true;

            const XrdCl::ChunkList& resChunks = vReadInfo->GetChunks();
            Napi::Array jsArray = Napi::Array::New(env, resChunks.size());

            for (size_t i = 0; i < resChunks.size(); ++i) {
              const XrdCl::ChunkInfo& chunk = resChunks[i];
              // 注意：resChunks 里的 buffer 就是我们最初 new 出来的地址
              char* bufPtr = static_cast<char*>(chunk.buffer);
              uint32_t actualLength = chunk.length;

              Napi::Buffer<char> jsBuf = Napi::Buffer<char>::New(
                  env, bufPtr, actualLength, [](Napi::Env, char* finalizeData) {
                    delete[] finalizeData;  // 移交 V8 GC
                  }
              );
              jsArray.Set(i, jsBuf);
            }
            this->deferred_.Resolve(jsArray);
          } else {
            // 失败时依然报错。我们不需要做 this->chunks_transferred_ = true，
            // 因为在 ~FileVectorReadHandler 中会自动清理。
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          delete vReadInfo;
          delete this;
        }
    );

    if (callStatus != napi_ok) {
      delete vReadInfo;
      delete this;
    }
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  XrdCl::ChunkList chunkList_;
  std::vector<char*> allocated_buffers_;
  bool chunks_transferred_;
};

}  // namespace XrdNode
