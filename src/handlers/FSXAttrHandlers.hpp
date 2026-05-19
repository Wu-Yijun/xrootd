// src/handlers/FSXAttrHandlers.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

// ============================================================================
// 1. FSXAttrStatusHandler (SetXAttr, DelXAttr)
// ============================================================================
/**
 * @brief 扩展属性状态返回 (SetXAttr / DelXAttr) 操作的异步回调处理器 (Handler)。
 * @details 专用于处理针对多个扩展属性进行修改或删除时，底层返回的每个属性的操作状态列表。
 *
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象、操作名称 opName。
 * - **内部处理**：从底层的 AnyObject 中提取出 std::vector<XrdCl::XAttrStatus> 指针，通过 TSFN 跳回
 * V8 主线程后，遍历向量中的每个元素，提取 name、状态码及错误信息，组装为 JS
 * 数组，随后释放状态向量、释放 TSFN 并 delete this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve 包含 { name, isOk, code, message } 的 JS Array，失败
 * Reject `Napi::Error`。
 */
class FSXAttrStatusHandler : public XrdCl::ResponseHandler {
 public:
  FSXAttrStatusHandler(Napi::Env env, Napi::Promise::Deferred deferred, const std::string& opName)
      : deferred_(deferred), opName_(opName) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        ("XRootD_FS_" + opName_).c_str(),
        0,
        1
    );
  }

  virtual ~FSXAttrStatusHandler() { tsfn_.Release(); }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    std::vector<XrdCl::XAttrStatus>* statusList = nullptr;
    if (status->IsOK() && response) {
      response->Get(statusList);
    }

    napi_status callStatus = tsfn_.BlockingCall(
        [this, status = *status, statusList](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK() && statusList) {
            uint32_t size = statusList->size();
            Napi::Array arr = Napi::Array::New(env, size);

            for (uint32_t i = 0; i < size; ++i) {
              const auto& item = (*statusList)[i];
              Napi::Object obj = Napi::Object::New(env);
              obj.Set("name", Napi::String::New(env, item.name));
              obj.Set("isOk", Napi::Boolean::New(env, item.status.IsOK()));
              obj.Set("code", Napi::Number::New(env, static_cast<uint32_t>(item.status.code)));
              obj.Set("message", Napi::String::New(env, item.status.GetErrorMessage()));

              arr.Set(i, obj);
            }

            this->deferred_.Resolve(arr);
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          delete statusList;
          delete this;
        }
    );

    if (callStatus != napi_ok) {
      delete statusList;
      delete this;
    }
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  std::string opName_;
};

// ============================================================================
// 2. FSXAttrDataHandler (GetXAttr, ListXAttr)
// ============================================================================
/**
 * @brief 扩展属性数据查询 (GetXAttr / ListXAttr) 操作的异步回调处理器 (Handler)。
 * @details 专用于获取文件或目录上挂载的扩展属性键值对列表。
 *
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象、操作名称 opName。
 * - **内部处理**：从底层的 AnyObject 中提取出 std::vector<XrdCl::XAttr> 指针，通过 TSFN 跳回 V8
 * 主线程后，调用 Utils::XAttrVectorToObject 将其反序列化为 Record<string, string> 结构的 JS
 * Object，随后释放属性向量、释放 TSFN 并 delete this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve 包含属性键值对的 JS Object，失败 Reject
 * `Napi::Error`。
 */
class FSXAttrDataHandler : public XrdCl::ResponseHandler {
 public:
  FSXAttrDataHandler(Napi::Env env, Napi::Promise::Deferred deferred, const std::string& opName)
      : deferred_(deferred), opName_(opName) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        ("XRootD_FS_" + opName_).c_str(),
        0,
        1
    );
  }

  virtual ~FSXAttrDataHandler() { tsfn_.Release(); }

  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    std::vector<XrdCl::XAttr>* attrList = nullptr;
    if (status->IsOK() && response) {
      response->Get(attrList);
    }

    napi_status callStatus = tsfn_.BlockingCall(
        [this, status = *status, attrList](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK() && attrList) {
            Napi::Object obj = Utils::XAttrVectorToObject(env, *attrList);
            this->deferred_.Resolve(obj);
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          delete attrList;
          delete this;
        }
    );

    if (callStatus != napi_ok) {
      delete attrList;
      delete this;
    }
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
  std::string opName_;
};

}  // namespace XrdNode
