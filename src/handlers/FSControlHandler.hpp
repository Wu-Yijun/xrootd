// src/handlers/FSControlHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

#include "../utils/type_conversions.h"

namespace XrdNode {

class FSControlHandler : public XrdCl::ResponseHandler {
 public:
  FSControlHandler(Napi::Env env, Napi::Promise::Deferred deferred, const std::string& opName)
      : deferred_(deferred), opName_(opName) {
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        ("XRootD_FS_" + opName_).c_str(),
        0,
        1
    );
  }

  virtual ~FSControlHandler() { tsfn_.Release(); }

  virtual void
  HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* /*response*/) override {
    tsfn_.BlockingCall([this, status = *status](Napi::Env env, Napi::Function /*jsCallback*/) {
      if (status.IsOK()) {
        this->deferred_.Resolve(env.Undefined());
      } else {
        Napi::Error err = Utils::StatusToError(env, status);
        this->deferred_.Reject(err.Value());
      }

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
