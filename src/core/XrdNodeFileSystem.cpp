#include "XrdNodeFileSystem.h"

#include <XrdCl/XrdClFileSystem.hh>

#include "handlers/AsyncStatHandler.hpp"
#include "handlers/FSBufferHandler.hpp"
#include "handlers/FSComplexHandlers.hpp"
#include "handlers/FSControlHandler.hpp"
#include "handlers/FSXAttrHandlers.hpp"
#include "utils/type_conversions.h"

using XrdNode::AsyncStatHandler;
using XrdNode::FSBufferHandler;
using XrdNode::FSControlHandler;
using XrdNode::FSDirListHandler;
using XrdNode::FSLocateHandler;
using XrdNode::FSStatVFSHandler;
using XrdNode::FSXAttrDataHandler;
using XrdNode::FSXAttrStatusHandler;

Napi::Object XrdNodeFileSystem::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
      env,
      "FileSystem",
      {InstanceMethod("Locate", &XrdNodeFileSystem::Locate),
       InstanceMethod("DeepLocate", &XrdNodeFileSystem::DeepLocate),
       InstanceMethod("Mv", &XrdNodeFileSystem::Mv),
       InstanceMethod("Query", &XrdNodeFileSystem::Query),
       InstanceMethod("Truncate", &XrdNodeFileSystem::Truncate),
       InstanceMethod("Rm", &XrdNodeFileSystem::Rm),
       InstanceMethod("MkDir", &XrdNodeFileSystem::MkDir),
       InstanceMethod("RmDir", &XrdNodeFileSystem::RmDir),
       InstanceMethod("ChMod", &XrdNodeFileSystem::ChMod),
       InstanceMethod("Ping", &XrdNodeFileSystem::Ping),
       InstanceMethod("Stat", &XrdNodeFileSystem::Stat),
       InstanceMethod("StatVFS", &XrdNodeFileSystem::StatVFS),
       InstanceMethod("Protocol", &XrdNodeFileSystem::Protocol),
       InstanceMethod("DirList", &XrdNodeFileSystem::DirList),
       InstanceMethod("SendInfo", &XrdNodeFileSystem::SendInfo),
       InstanceMethod("Prepare", &XrdNodeFileSystem::Prepare),
       InstanceMethod("SendCache", &XrdNodeFileSystem::SendCache),
       InstanceMethod("GetProperty", &XrdNodeFileSystem::GetProperty),
       InstanceMethod("SetProperty", &XrdNodeFileSystem::SetProperty),
       InstanceMethod("SetXAttr", &XrdNodeFileSystem::SetXAttr),
       InstanceMethod("GetXAttr", &XrdNodeFileSystem::GetXAttr),
       InstanceMethod("DelXAttr", &XrdNodeFileSystem::DelXAttr),
       InstanceMethod("ListXAttr", &XrdNodeFileSystem::ListXAttr)}
  );

  exports.Set("FileSystem", func);

  return exports;
}

XrdNodeFileSystem::XrdNodeFileSystem(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<XrdNodeFileSystem>(info) {
  std::string url = info[0].As<Napi::String>().Utf8Value();
  this->fs_ = new XrdCl::FileSystem(url);
}

XrdNodeFileSystem::~XrdNodeFileSystem() { delete this->fs_; }

Napi::Value XrdNodeFileSystem::Locate(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  XrdCl::OpenFlags::Flags flags =
      static_cast<XrdCl::OpenFlags::Flags>(info[1].As<Napi::Number>().Uint32Value());

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSLocateHandler* handler = new FSLocateHandler(env, deferred, "Locate");
  auto status = this->fs_->Locate(path, flags, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::DeepLocate(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  XrdCl::OpenFlags::Flags flags =
      static_cast<XrdCl::OpenFlags::Flags>(info[1].As<Napi::Number>().Uint32Value());

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSLocateHandler* handler = new FSLocateHandler(env, deferred, "DeepLocate");
  auto status = this->fs_->DeepLocate(path, flags, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::Mv(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string source = info[0].As<Napi::String>().Utf8Value();
  std::string dest = info[1].As<Napi::String>().Utf8Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSControlHandler* handler = new FSControlHandler(env, deferred, "Mv");
  auto status = this->fs_->Mv(source, dest, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::Query(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  XrdCl::QueryCode::Code code =
      static_cast<XrdCl::QueryCode::Code>(info[0].As<Napi::Number>().Uint32Value());
  Napi::Buffer<char> argBuf = info[1].As<Napi::Buffer<char>>();
  XrdCl::Buffer xrdBuf(argBuf.Length());
  xrdBuf.Append(argBuf.Data(), argBuf.Length());

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSBufferHandler* handler = new FSBufferHandler(env, deferred, "Query");
  auto status = this->fs_->Query(code, xrdBuf, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::Truncate(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  uint64_t size = 0;
  if (info[1].IsBigInt()) {
    bool lossless;
    size = info[1].As<Napi::BigInt>().Uint64Value(&lossless);
  } else {
    size = info[1].As<Napi::Number>().Int64Value();
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSControlHandler* handler = new FSControlHandler(env, deferred, "Truncate");
  auto status = this->fs_->Truncate(path, size, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::Rm(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSControlHandler* handler = new FSControlHandler(env, deferred, "Rm");
  auto status = this->fs_->Rm(path, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::MkDir(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  XrdCl::MkDirFlags::Flags flags =
      static_cast<XrdCl::MkDirFlags::Flags>(info[1].As<Napi::Number>().Uint32Value());
  XrdCl::Access::Mode mode =
      static_cast<XrdCl::Access::Mode>(info[2].As<Napi::Number>().Uint32Value());

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSControlHandler* handler = new FSControlHandler(env, deferred, "MkDir");
  auto status = this->fs_->MkDir(path, flags, mode, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::RmDir(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSControlHandler* handler = new FSControlHandler(env, deferred, "RmDir");
  auto status = this->fs_->RmDir(path, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::ChMod(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  XrdCl::Access::Mode mode =
      static_cast<XrdCl::Access::Mode>(info[1].As<Napi::Number>().Uint32Value());

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSControlHandler* handler = new FSControlHandler(env, deferred, "ChMod");
  auto status = this->fs_->ChMod(path, mode, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::Ping(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSControlHandler* handler = new FSControlHandler(env, deferred, "Ping");
  auto status = this->fs_->Ping(handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::Stat(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();

  // 1. 创建 Promise
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  // 2. new 一个 Handler。注意：生命周期由 XRootD 接管，网络返回后自动 delete
  AsyncStatHandler* handler = new AsyncStatHandler(env, deferred);

  // 3. 调用底层的异步方法，瞬间返回！
  auto status = this->fs_->Stat(path, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  // 4. 返回 Promise 给 JS
  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::StatVFS(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSStatVFSHandler* handler = new FSStatVFSHandler(env, deferred);
  auto status = this->fs_->StatVFS(path, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::Protocol(const Napi::CallbackInfo& info) {
  // TODO: ProtocolInfo if needed
  return Napi::Value();
}

Napi::Value XrdNodeFileSystem::DirList(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  XrdCl::DirListFlags::Flags flags =
      static_cast<XrdCl::DirListFlags::Flags>(info[1].As<Napi::Number>().Uint32Value());

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSDirListHandler* handler = new FSDirListHandler(env, deferred);
  auto status = this->fs_->DirList(path, flags, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::SendInfo(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string infoStr = info[0].As<Napi::String>().Utf8Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSBufferHandler* handler = new FSBufferHandler(env, deferred, "SendInfo");
  auto status = this->fs_->SendInfo(infoStr, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::Prepare(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Array arr = info[0].As<Napi::Array>();
  std::vector<std::string> fileList;
  uint32_t len = arr.Length();
  for (uint32_t i = 0; i < len; ++i) {
    fileList.push_back(arr.Get(i).As<Napi::String>().Utf8Value());
  }
  XrdCl::PrepareFlags::Flags flags =
      static_cast<XrdCl::PrepareFlags::Flags>(info[1].As<Napi::Number>().Uint32Value());
  uint8_t priority = static_cast<uint8_t>(info[2].As<Napi::Number>().Uint32Value());

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSBufferHandler* handler = new FSBufferHandler(env, deferred, "Prepare");
  auto status = this->fs_->Prepare(fileList, flags, priority, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::SendCache(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string infoStr = info[0].As<Napi::String>().Utf8Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSBufferHandler* handler = new FSBufferHandler(env, deferred, "SendCache");
  auto status = this->fs_->SendCache(infoStr, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::GetProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  std::string value;
  bool success = this->fs_->GetProperty(name, value);

  Napi::Object obj = Napi::Object::New(env);
  obj.Set("success", Napi::Boolean::New(env, success));
  obj.Set("value", Napi::String::New(env, value));
  return obj;
}

Napi::Value XrdNodeFileSystem::SetProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  std::string value = info[1].As<Napi::String>().Utf8Value();
  bool success = this->fs_->SetProperty(name, value);
  return Napi::Boolean::New(env, success);
}

Napi::Value XrdNodeFileSystem::SetXAttr(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  Napi::Object obj = info[1].As<Napi::Object>();
  std::vector<XrdCl::xattr_t> attrs = XrdNode::Utils::ObjectToXAttrVector(env, obj);

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSXAttrStatusHandler* handler = new FSXAttrStatusHandler(env, deferred, "SetXAttr");
  auto status = this->fs_->SetXAttr(path, attrs, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::GetXAttr(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  Napi::Array arr = info[1].As<Napi::Array>();
  std::vector<std::string> keys;
  uint32_t len = arr.Length();
  for (uint32_t i = 0; i < len; ++i) {
    keys.push_back(arr.Get(i).As<Napi::String>().Utf8Value());
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSXAttrDataHandler* handler = new FSXAttrDataHandler(env, deferred, "GetXAttr");
  auto status = this->fs_->GetXAttr(path, keys, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::DelXAttr(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  Napi::Array arr = info[1].As<Napi::Array>();
  std::vector<std::string> keys;
  uint32_t len = arr.Length();
  for (uint32_t i = 0; i < len; ++i) {
    keys.push_back(arr.Get(i).As<Napi::String>().Utf8Value());
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSXAttrStatusHandler* handler = new FSXAttrStatusHandler(env, deferred, "DelXAttr");
  auto status = this->fs_->DelXAttr(path, keys, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFileSystem::ListXAttr(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  FSXAttrDataHandler* handler = new FSXAttrDataHandler(env, deferred, "ListXAttr");
  auto status = this->fs_->ListXAttr(path, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
  }

  return deferred.Promise();
}
