// src/handlers/FSComplexHandlers.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

// ============================================================================
// 1. FSStatVFSHandler
// ============================================================================
/**
 * @brief 虚拟文件系统状态 (StatVFS) 操作的异步回调处理器 (Handler)。
 * @details 专用于获取集群或文件系统层面的节点与存储利用率统计信息。
 * 
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象。
 * - **内部处理**：从底层的 AnyObject 中提取出 XrdCl::StatInfoVFS 指针，通过 TSFN 跳回 V8 主线程后，将 64 位整数字段通过 Napi::BigInt 包装，数值字段通过 Napi::Number 包装，随后释放 StatInfoVFS、释放 TSFN 并 delete this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve 包含 nodesRW/freeRW/utilizationRW 等属性的 JS Object，失败 Reject `Napi::Error`。
 */
class FSStatVFSHandler : public XrdCl::ResponseHandler {
 public:
  FSStatVFSHandler(Napi::Env env, Napi::Promise::Deferred deferred) : deferred_(deferred) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), "XRootD_FS_StatVFS", 0, 1
    );
  }

  virtual ~FSStatVFSHandler() { tsfn_.Release(); }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    XrdCl::StatInfoVFS* vfsInfo = nullptr;
    if (status->IsOK() && response) {
      response->Get(vfsInfo);
    }

    tsfn_.BlockingCall([this,
                        status = *status,
                        vfsInfo](Napi::Env env, Napi::Function /*jsCallback*/) {
      if (status.IsOK() && vfsInfo) {
        Napi::Object result = Napi::Object::New(env);
        result.Set("nodesRW", Napi::BigInt::New(env, vfsInfo->GetNodesRW()));
        result.Set("freeRW", Napi::BigInt::New(env, vfsInfo->GetFreeRW()));
        result.Set("utilizationRW", Napi::Number::New(env, vfsInfo->GetUtilizationRW()));
        result.Set("nodesStaging", Napi::BigInt::New(env, vfsInfo->GetNodesStaging()));
        result.Set("freeStaging", Napi::BigInt::New(env, vfsInfo->GetFreeStaging()));
        result.Set("utilizationStaging", Napi::Number::New(env, vfsInfo->GetUtilizationStaging()));

        this->deferred_.Resolve(result);
      } else {
        Napi::Error err = Utils::StatusToError(env, status);
        this->deferred_.Reject(err.Value());
      }

      delete vfsInfo;
      this->tsfn_.Release();
      delete this;
    });
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
};

// ============================================================================
// 2. FSDirListHandler
// ============================================================================
/**
 * @brief 获取目录列表 (DirList) 操作的异步回调处理器 (Handler)。
 * @details 专用于列出目录下的所有子项及其可选的 Stat 元数据。
 * 
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象。
 * - **内部处理**：从底层的 AnyObject 中提取出 XrdCl::DirectoryList 指针，通过 TSFN 跳回 V8 主线程后，循环遍历每一个 ListEntry，提取 name、hostAddress，并在存在 statInfo 时调用 Utils::StatInfoToObject 转换，组装为 JS 数组，随后释放 DirectoryList、释放 TSFN 并 delete this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve 包含所有目录项结构体的 JS Array，失败 Reject `Napi::Error`。
 */
class FSDirListHandler : public XrdCl::ResponseHandler {
 public:
  FSDirListHandler(Napi::Env env, Napi::Promise::Deferred deferred) : deferred_(deferred) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), "XRootD_FS_DirList", 0, 1
    );
  }

  virtual ~FSDirListHandler() { tsfn_.Release(); }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    XrdCl::DirectoryList* dirList = nullptr;
    if (status->IsOK() && response) {
      response->Get(dirList);
    }

    tsfn_.BlockingCall(
        [this, status = *status, dirList](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK() && dirList) {
            uint32_t size = dirList->GetSize();
            Napi::Array arr = Napi::Array::New(env, size);

            for (uint32_t i = 0; i < size; ++i) {
              XrdCl::DirectoryList::ListEntry* entry = dirList->At(i);
              Napi::Object obj = Napi::Object::New(env);
              obj.Set("name", Napi::String::New(env, entry->GetName()));
              obj.Set("hostAddress", Napi::String::New(env, entry->GetHostAddress()));

              if (entry->GetStatInfo()) {
                obj.Set("stat", Utils::StatInfoToObject(env, entry->GetStatInfo()));
              } else {
                obj.Set("stat", env.Null());
              }

              arr.Set(i, obj);
            }

            this->deferred_.Resolve(arr);
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          delete dirList;
          this->tsfn_.Release();
          delete this;
        }
    );
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
};

// ============================================================================
// 3. FSLocateHandler
// ============================================================================
/**
 * @brief 数据定位 (Locate / DeepLocate) 操作的异步回调处理器 (Handler)。
 * @details 专用于获取文件所在的实际数据服务器地址列表和访问权限类型。
 * 
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象、操作名称 opName。
 * - **内部处理**：从底层的 AnyObject 中提取出 XrdCl::LocationInfo 指针，通过 TSFN 跳回 V8 主线程后，循环遍历每一个 Location 节点，提取 address、type 和 accessType 枚举值并组装为 JS 数组，随后释放 LocationInfo、释放 TSFN 并 delete this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve 包含 { address, type, accessType } 的 JS Array，失败 Reject `Napi::Error`。
 */
class FSLocateHandler : public XrdCl::ResponseHandler {
 public:
  FSLocateHandler(Napi::Env env, Napi::Promise::Deferred deferred, const std::string& opName)
      : deferred_(deferred), opName_(opName) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        ("XRootD_FS_" + opName_).c_str(),
        0,
        1
    );
  }

  virtual ~FSLocateHandler() { tsfn_.Release(); }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    XrdCl::LocationInfo* locInfo = nullptr;
    if (status->IsOK() && response) {
      response->Get(locInfo);
    }

    tsfn_.BlockingCall([this,
                        status = *status,
                        locInfo](Napi::Env env, Napi::Function /*jsCallback*/) {
      if (status.IsOK() && locInfo) {
        uint32_t size = locInfo->GetSize();
        Napi::Array arr = Napi::Array::New(env, size);

        for (uint32_t i = 0; i < size; ++i) {
          XrdCl::LocationInfo::Location& loc = locInfo->At(i);
          Napi::Object obj = Napi::Object::New(env);
          obj.Set("address", Napi::String::New(env, loc.GetAddress()));
          obj.Set("type", Napi::Number::New(env, static_cast<uint32_t>(loc.GetType())));
          obj.Set("accessType", Napi::Number::New(env, static_cast<uint32_t>(loc.GetAccessType())));

          arr.Set(i, obj);
        }

        this->deferred_.Resolve(arr);
      } else {
        Napi::Error err = Utils::StatusToError(env, status);
        this->deferred_.Reject(err.Value());
      }

      delete locInfo;
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
