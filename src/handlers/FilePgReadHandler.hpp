// src/handlers/FilePgReadHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>
#include <vector>

#include "../utils/type_conversions.h"

namespace XrdNode {

/**
 * @brief 文件页读取 (PgRead) 操作的异步回调处理器。
 * @details 负责读取页对齐数据，并返回包含 Buffer 数据和校验和的 JS 对象。
 */
class FilePgReadHandler : public XrdCl::ResponseHandler {
 public:
  FilePgReadHandler(Napi::Env env, Napi::Promise::Deferred deferred, uint32_t size)
      : deferred_(deferred), size_(size), buffer_transferred_(false) {
    buffer_ = new char[size];
    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), "XRootD_FilePgRead", 0, 1
    );
  }

  virtual ~FilePgReadHandler() {
    tsfn_.Release();
    if (!buffer_transferred_ && buffer_) {
      delete[] buffer_;
    }
  }

  void* GetBuffer() const { return buffer_; }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    XrdCl::PageInfo* pageInfo = nullptr;
    if (status->IsOK() && response) {
      response->Get(pageInfo);
    }

    napi_status callStatus = tsfn_.BlockingCall(
        [this, status = *status, pageInfo](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK() && pageInfo) {
            this->buffer_transferred_ = true;
            uint32_t bytesRead = pageInfo->GetLength();

            // 构造 Buffer
            Napi::Buffer<char> jsBuffer = Napi::Buffer<char>::New(
                env,
                this->buffer_,
                bytesRead,
                [](Napi::Env, char* finalizeData) {
                  delete[] finalizeData;
                }
            );

            // 提取校验和数组
            std::vector<uint32_t>& cksums = pageInfo->GetCksums();
            size_t cksumLen = cksums.size();
            Napi::Uint32Array jsCksums = Napi::Uint32Array::New(env, cksumLen);
            for (size_t i = 0; i < cksumLen; ++i) {
              jsCksums[i] = cksums[i];
            }

            // 构造返回对象 { data, checksums }
            Napi::Object result = Napi::Object::New(env);
            result.Set("data", jsBuffer);
            result.Set("checksums", jsCksums);

            this->deferred_.Resolve(result);
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          delete pageInfo;
          delete this;
        }
    );

    if (callStatus != napi_ok) {
      delete pageInfo;
      delete this;
    }
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  uint32_t size_;
  char* buffer_;
  bool buffer_transferred_;
};

}  // namespace XrdNode
