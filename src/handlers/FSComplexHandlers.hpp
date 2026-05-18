// src/handlers/FSComplexHandlers.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

// ============================================================================
// 1. FSStatVFSHandler
// ============================================================================
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
