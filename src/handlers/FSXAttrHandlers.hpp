// src/handlers/FSXAttrHandlers.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

// ============================================================================
// 1. FSXAttrStatusHandler (SetXAttr, DelXAttr)
// ============================================================================
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

    tsfn_.BlockingCall(
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

// ============================================================================
// 2. FSXAttrDataHandler (GetXAttr, ListXAttr)
// ============================================================================
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

    tsfn_.BlockingCall(
        [this, status = *status, attrList](Napi::Env env, Napi::Function /*jsCallback*/) {
          if (status.IsOK() && attrList) {
            Napi::Object obj = Utils::XAttrVectorToObject(env, *attrList);
            this->deferred_.Resolve(obj);
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          delete attrList;
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
