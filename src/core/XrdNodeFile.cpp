#include "XrdNodeFile.h"

#include "handlers/AsyncStatHandler.hpp"
#include "handlers/FSBufferHandler.hpp"
#include "handlers/FSXAttrHandlers.hpp"
#include "handlers/FileControlHandler.hpp"
#include "handlers/FilePgReadHandler.hpp"
#include "handlers/FilePgWriteHandler.hpp"
#include "handlers/FileReadHandler.hpp"
#include "handlers/FileVectorReadHandler.hpp"
#include "handlers/FileVectorWriteHandler.hpp"
#include "handlers/FileWriteHandler.hpp"

using XrdNode::FilePgReadHandler, XrdNode::FilePgWriteHandler;
using XrdNode::FileReadHandler, XrdNode::FileWriteHandler, XrdNode::FileControlHandler;
using XrdNode::FileVectorReadHandler, XrdNode::FileVectorWriteHandler;
using XrdNode::FSBufferHandler, XrdNode::AsyncStatHandler;
using XrdNode::FSXAttrStatusHandler, XrdNode::FSXAttrDataHandler;

Napi::Object XrdNodeFile::Init(Napi::Env env, Napi::Object exports) {
  // 1. 定义 JS 类的名称和它原型链上的所有方法
  Napi::Function func = DefineClass(
      env,
      "File",
      {// InstanceMethod 会把 C++ 方法挂载到 JS 的 prototype 上
       InstanceMethod("Open", &XrdNodeFile::Open),
       InstanceMethod("Close", &XrdNodeFile::Close),
       InstanceMethod("Stat", &XrdNodeFile::Stat),
       InstanceMethod("Read", &XrdNodeFile::Read),
       InstanceMethod("Write", &XrdNodeFile::Write),
       InstanceMethod("WriteFd", &XrdNodeFile::WriteFd),
       InstanceMethod("Sync", &XrdNodeFile::Sync),
       InstanceMethod("Truncate", &XrdNodeFile::Truncate),
       InstanceMethod("PreRead", &XrdNodeFile::PreRead),

       // 向量读写与特殊操作
       InstanceMethod("VectorRead", &XrdNodeFile::VectorRead),
       InstanceMethod("ReadChunks", &XrdNodeFile::ReadChunks),
       InstanceMethod("VectorWrite", &XrdNodeFile::VectorWrite),
       InstanceMethod("WriteV", &XrdNodeFile::WriteV),
       InstanceMethod("ReadV", &XrdNodeFile::ReadV),
       InstanceMethod("PgRead", &XrdNodeFile::PgRead),
       InstanceMethod("PgWrite", &XrdNodeFile::PgWrite),
       InstanceMethod("Fcntl", &XrdNodeFile::Fcntl),
       InstanceMethod("Visa", &XrdNodeFile::Visa),

       // 同步方法
       InstanceMethod("IsOpen", &XrdNodeFile::IsOpen),
       InstanceMethod("IsSecure", &XrdNodeFile::IsSecure),
       InstanceMethod("TryOtherServer", &XrdNodeFile::TryOtherServer),

       // 扩展属性
       InstanceMethod("GetProperty", &XrdNodeFile::GetProperty),
       InstanceMethod("SetProperty", &XrdNodeFile::SetProperty),
       InstanceMethod("SetXAttr", &XrdNodeFile::SetXAttr),
       InstanceMethod("GetXAttr", &XrdNodeFile::GetXAttr),
       InstanceMethod("DelXAttr", &XrdNodeFile::DelXAttr),
       InstanceMethod("ListXAttr", &XrdNodeFile::ListXAttr),

       InstanceMethod("Clone", &XrdNodeFile::Clone)
      }
  );

  // (可选) 如果你需要在 C++ 内部 (比如在 Clone 方法中) 实例化这个 JS 对象，
  // 你需要把这个 func (构造函数) 保存到一个静态的 Napi::FunctionReference 中。
  // constructor() = Napi::Persistent(func);
  // constructor().SuppressDestruct();

  // 2. 将创建好的 JS 构造函数挂载到 exports 对象上
  // 等价于 JavaScript 中的：exports.File = function File() { ... }
  exports.Set("File", func);

  return exports;
}

// 必须显式调用父类 Napi::ObjectWrap 的构造函数
XrdNodeFile::XrdNodeFile(const Napi::CallbackInfo& info) : Napi::ObjectWrap<XrdNodeFile>(info) {
  Napi::Env env = info.Env();

  // 检查：是否是从 C++ 内部 (如 Clone 方法) 传进来的已有指针？
  if (info.Length() > 0 && info[0].IsExternal()) {
    // 解包接收这个已有的 C++ 指针
    this->file_ = info[0].As<Napi::External<XrdCl::File>>().Data();
  } else {
    // 普通的 JS 调用 `new File()`
    // 调用 XrdCl::File 的默认构造函数
    this->file_ = new XrdCl::File();
  }
}

XrdNodeFile::~XrdNodeFile() {
  // 只要 JavaScript 对象被 GC 回收，我们就释放底层的 XRootD File 实例
  if (this->file_) {
    // XrdCl::File 的析构函数内部会安全地处理资源释放，
    // 即使文件没有被显式 Close，底层的 C++ 析构也会负责清理网络句柄。
    delete this->file_;
    this->file_ = nullptr;
  }
}

Napi::Value XrdNodeFile::IsOpen(const Napi::CallbackInfo& info) {
  return Napi::Boolean::New(info.Env(), file_->IsOpen());
}

Napi::Value XrdNodeFile::IsSecure(const Napi::CallbackInfo& info) {
  return Napi::Boolean::New(info.Env(), file_->IsSecure());
}

Napi::Value XrdNodeFile::TryOtherServer(const Napi::CallbackInfo& info) {
  XrdCl::XRootDStatus status = file_->TryOtherServer();
  return Napi::Boolean::New(info.Env(), status.IsOK());
}

// ============================================================================
// 1. Open 接口实现
// ============================================================================
Napi::Value XrdNodeFile::Open(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  // 1. 状态防火墙：防止重复打开和未 await close 的情况
  if (this->file_->IsOpen()) {
    Napi::Error err =
        Napi::Error::New(env, "File is already open. You must await close() before reopening.");
    deferred.Reject(err.Value());
    return deferred.Promise();
  }

  // 参数提取与校验
  if (info.Length() < 3 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsNumber()) {
    Napi::Error err = Napi::TypeError::New(
        env, "Invalid arguments for Open: path, flags, and mode are required."
    );
    deferred.Reject(err.Value());
    return deferred.Promise();  // 安全返回 rejected promise
  }

  std::string url = info[0].As<Napi::String>().Utf8Value();
  uint32_t flags = info[1].As<Napi::Number>().Uint32Value();
  uint32_t mode = info[2].As<Napi::Number>().Uint32Value();

  // 实例化复用的控制 Handler
  auto* handler = new FileControlHandler(env, deferred, "Open");

  // 调用 XRootD 异步 Open，瞬间返回
  XrdCl::XRootDStatus status = this->file_->Open(
      url,
      static_cast<XrdCl::OpenFlags::Flags>(flags),
      static_cast<XrdCl::Access::Mode>(mode),
      handler
  );

  if (!status.IsOK()) {  // TODO: discuss
    // 如果发起异步请求本身就失败了（例如本地状态错误），立即销毁 handler 并 Reject
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
    return deferred.Promise();
  }

  return deferred.Promise();
}

// ============================================================================
// 2. Close 接口实现
// ============================================================================
Napi::Value XrdNodeFile::Close(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  auto* handler = new FileControlHandler(env, deferred, "Close");

  // 调用 XRootD 异步 Close
  XrdCl::XRootDStatus status = this->file_->Close(handler);

  if (!status.IsOK()) {  // TODO: discuss
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
    return deferred.Promise();
  }

  return deferred.Promise();
}

// ============================================================================
// 3. Clone 接口实现（核心：解包 JS 数组并提取跨实例的底层指针）
// ============================================================================
Napi::Value XrdNodeFile::Clone(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "Clone expects an array of locations").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Array arr = info[0].As<Napi::Array>();
  XrdCl::CloneLocations locs;
  bool lossless;

  // 遍历 JS 传进来的克隆任务配置数组
  for (uint32_t i = 0; i < arr.Length(); i++) {
    Napi::Value itemValue = arr.Get(i);
    if (!itemValue.IsObject()) continue;

    Napi::Object item = itemValue.As<Napi::Object>();
    Napi::Object srcFileObj = item.Get("srcFile").As<Napi::Object>();

    // 核心安全技巧：从传入的 JS File 对象中，反解出它对应的 C++ 包装类实例
    // 从而直接拿到它内部持有的原生 XrdCl::File* 指针
    XrdNodeFile* srcNodeFile = Napi::ObjectWrap<XrdNodeFile>::Unwrap(srcFileObj);
    if (!srcNodeFile || !srcNodeFile->GetInternalFile()) {
      Napi::TypeError::New(env, "Invalid source file object in clone locations")
          .ThrowAsJavaScriptException();
      return env.Null();
    }

    // 提取大整数偏移量，严格校验精度
    int64_t dstOffs = item.Get("dstOffset").As<Napi::BigInt>().Int64Value(&lossless);
    int64_t srcOffs = item.Get("srcOffset").As<Napi::BigInt>().Int64Value(&lossless);
    int64_t srcLen = item.Get("length").As<Napi::BigInt>().Int64Value(&lossless);

    if (!lossless) {
      Napi::RangeError::New(env, "Clone offset or length exceeds 64-bit integer limits")
          .ThrowAsJavaScriptException();
      return env.Null();
    }

    // 将原生指针解引用后，塞入 XRootD 的克隆任务清单中
    locs.Add(*(srcNodeFile->GetInternalFile()), dstOffs, srcOffs, srcLen);
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileControlHandler(env, deferred, "Clone");

  // 发起服务端的异步克隆请求
  XrdCl::XRootDStatus status = this->file_->Clone(locs, handler);

  if (!status.IsOK()) {  // TODO: discuss
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
    return deferred.Promise();
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFile::Stat(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  bool force = false;
  if (info.Length() >= 1 && info[0].IsBoolean()) {
    force = info[0].As<Napi::Boolean>().Value();
  }
  auto* handler = new AsyncStatHandler(env, deferred);
  auto status = this->file_->Stat(force, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::Read(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  // 从 TS 获取参数: read(offset: bigint | number, size: number)
  bool lossless;
  uint64_t offset = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless) {
    Napi::TypeError::New(env, "Offset value is out of bounds for uint64_t")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  uint32_t size = info[1].As<Napi::Number>().Uint32Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  // 实例化 Handler
  auto* handler = new FileReadHandler(env, deferred, size);

  // 瞬间异步调用，将 C++ 分配的 buffer_ 指针传进去
  auto status = this->file_->Read(offset, size, handler->GetBuffer(), handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
    return deferred.Promise();
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFile::Write(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2 || !info[0].IsBigInt() || !info[1].IsBuffer()) {
    Napi::TypeError::New(env, "Write expects offset (bigint) and buffer (Buffer)")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  bool lossless;
  uint64_t offset = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless) {
    Napi::TypeError::New(env, "Offset value is out of bounds for uint64_t")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Buffer<char> jsBuffer = info[1].As<Napi::Buffer<char>>();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileWriteHandler(env, deferred, jsBuffer);
  auto status = this->file_->Write(offset, jsBuffer.Length(), jsBuffer.Data(), handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::WriteFd(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 3 || !info[0].IsBigInt() || !info[1].IsNumber() || !info[2].IsNumber()) {
    Napi::TypeError::New(env, "WriteFd expects offset (bigint), size (number), and fd (number)")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  bool lossless;
  uint64_t offset = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless) {
    Napi::TypeError::New(env, "Offset value is out of bounds for uint64_t")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  uint32_t size = info[1].As<Napi::Number>().Uint32Value();
  int fd = info[2].As<Napi::Number>().Int32Value();

  XrdCl::Optional<uint64_t> fdoff;
  if (info.Length() >= 4 && !info[3].IsUndefined() && !info[3].IsNull()) {
    if (!info[3].IsBigInt()) {
      Napi::TypeError::New(env, "fdoff must be a bigint").ThrowAsJavaScriptException();
      return env.Null();
    }
    bool fdoff_lossless;
    fdoff = info[3].As<Napi::BigInt>().Uint64Value(&fdoff_lossless);
    if (!fdoff_lossless) {
      Napi::TypeError::New(env, "fdoff value is out of bounds for uint64_t")
          .ThrowAsJavaScriptException();
      return env.Null();
    }
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileControlHandler(env, deferred, "WriteFD");
  auto status = this->file_->Write(offset, size, fdoff, fd, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::Sync(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileControlHandler(env, deferred, "Sync");
  XrdCl::XRootDStatus status = this->file_->Sync(handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::Truncate(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  bool lossless;
  uint64_t size = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless) {
    Napi::TypeError::New(env, "Size value is out of bounds for uint64_t")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileControlHandler(env, deferred, "Truncate");
  XrdCl::XRootDStatus status = this->file_->Truncate(size, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::PreRead(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "PreRead expects an array of tracts").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Array arr = info[0].As<Napi::Array>();
  XrdCl::TractList tracts;
  bool lossless;
  for (uint32_t i = 0; i < arr.Length(); i++) {
    Napi::Value itemValue = arr.Get(i);
    if (!itemValue.IsObject()) continue;
    Napi::Object item = itemValue.As<Napi::Object>();
    uint64_t offset = item.Get("offset").As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless) {
      Napi::RangeError::New(env, "Offset value out of bounds").ThrowAsJavaScriptException();
      return env.Null();
    }
    uint32_t size = item.Get("size").As<Napi::Number>().Uint32Value();
    tracts.push_back(XrdCl::TractInfo(offset, size));
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileControlHandler(env, deferred, "PreRead");
  XrdCl::XRootDStatus status = this->file_->PreRead(tracts, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::Visa(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FSBufferHandler(env, deferred, "Visa");
  XrdCl::XRootDStatus status = this->file_->Visa(handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::VectorRead(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "VectorRead expects an array of {offset, size}")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileVectorReadHandler(env, deferred);

  Napi::Array arr = info[0].As<Napi::Array>();
  bool lossless;
  for (uint32_t i = 0; i < arr.Length(); i++) {
    Napi::Value itemValue = arr.Get(i);
    if (!itemValue.IsObject()) continue;
    Napi::Object item = itemValue.As<Napi::Object>();
    uint64_t offset = item.Get("offset").As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless) {
      Napi::RangeError::New(env, "Offset out of bounds").ThrowAsJavaScriptException();
      delete handler;
      return env.Null();
    }
    uint32_t size = item.Get("size").As<Napi::Number>().Uint32Value();
    handler->AddChunk(offset, size);
  }

  XrdCl::XRootDStatus status = this->file_->VectorRead(handler->GetChunkList(), nullptr, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::ReadChunks(const Napi::CallbackInfo& info) {
  return this->VectorRead(info);
}

Napi::Value XrdNodeFile::VectorWrite(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "VectorWrite expects an array of {offset, buffer}")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileVectorWriteHandler(env, deferred);

  Napi::Array arr = info[0].As<Napi::Array>();
  bool lossless;
  for (uint32_t i = 0; i < arr.Length(); i++) {
    Napi::Value itemValue = arr.Get(i);
    if (!itemValue.IsObject()) continue;
    Napi::Object item = itemValue.As<Napi::Object>();
    uint64_t offset = item.Get("offset").As<Napi::BigInt>().Uint64Value(&lossless);
    Napi::Value bufVal = item.Get("buffer");
    if (!lossless || !bufVal.IsBuffer()) {
      Napi::TypeError::New(env, "Invalid offset or buffer").ThrowAsJavaScriptException();
      delete handler;
      return env.Null();
    }
    Napi::Buffer<char> jsBuf = bufVal.As<Napi::Buffer<char>>();
    handler->AddChunk(offset, jsBuf.Length(), jsBuf);
  }

  XrdCl::XRootDStatus status = this->file_->VectorWrite(handler->GetChunkList(), handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::WriteV(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2 || !info[1].IsArray()) {
    Napi::TypeError::New(env, "WriteV expects offset and an array of Buffers")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  bool lossless;
  uint64_t offset = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless) {
    Napi::RangeError::New(env, "Offset out of bounds").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FileVectorWriteHandler(env, deferred);

  Napi::Array arr = info[1].As<Napi::Array>();
  for (uint32_t i = 0; i < arr.Length(); i++) {
    Napi::Value bufVal = arr.Get(i);
    if (!bufVal.IsBuffer()) {
      Napi::TypeError::New(env, "Expected array of Buffers").ThrowAsJavaScriptException();
      delete handler;
      return env.Null();
    }
    handler->AddIovec(bufVal.As<Napi::Buffer<char>>());
  }

  std::vector<struct iovec>& iovs = handler->GetIovecList();
  XrdCl::XRootDStatus status = this->file_->WriteV(offset, iovs.data(), iovs.size(), handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::ReadV(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::TypeError::New(env, "ReadV not implemented, please use VectorRead")
      .ThrowAsJavaScriptException();
  return env.Null();
}

Napi::Value XrdNodeFile::PgRead(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "PgRead expects offset and size").ThrowAsJavaScriptException();
    return env.Null();
  }
  bool lossless;
  uint64_t offset = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  uint32_t size = info[1].As<Napi::Number>().Uint32Value();
  if (!lossless) {
    Napi::RangeError::New(env, "Offset out of bounds").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FilePgReadHandler(env, deferred, size);

  XrdCl::XRootDStatus status = this->file_->PgRead(offset, size, handler->GetBuffer(), handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::PgWrite(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 4 || !info[2].IsBuffer() || !info[3].IsArray()) {
    Napi::TypeError::New(env, "PgWrite expects offset, size, buffer, checksumsArray")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  bool lossless;
  uint64_t offset = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  uint32_t size = info[1].As<Napi::Number>().Uint32Value();
  Napi::Buffer<char> buffer = info[2].As<Napi::Buffer<char>>();
  Napi::Array cksumsArr = info[3].As<Napi::Array>();
  if (!lossless) {
    Napi::RangeError::New(env, "Offset out of bounds").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::vector<uint32_t> cksums;
  for (uint32_t i = 0; i < cksumsArr.Length(); ++i) {
    cksums.push_back(cksumsArr.Get(i).As<Napi::Number>().Uint32Value());
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FilePgWriteHandler(env, deferred, buffer, cksums);

  XrdCl::XRootDStatus status =
      this->file_->PgWrite(offset, size, buffer.Data(), handler->GetCksums(), handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::Fcntl(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsBuffer()) {
    Napi::TypeError::New(env, "Fcntl expects a Buffer").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Buffer<char> buf = info[0].As<Napi::Buffer<char>>();

  XrdCl::Buffer xrdBuf;
  xrdBuf.FromString(std::string(buf.Data(), buf.Length()));

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FSBufferHandler(env, deferred, "Fcntl");
  XrdCl::XRootDStatus status = this->file_->Fcntl(xrdBuf, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::GetProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "GetProperty expects a property name string")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string name = info[0].As<Napi::String>().Utf8Value();
  std::string value;
  bool ok = file_->GetProperty(name, value);
  if (ok) return Napi::String::New(env, value);
  return env.Undefined();
}

Napi::Value XrdNodeFile::SetProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
    Napi::TypeError::New(env, "SetProperty expects name and value strings")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string name = info[0].As<Napi::String>().Utf8Value();
  std::string value = info[1].As<Napi::String>().Utf8Value();
  bool ok = file_->SetProperty(name, value);
  return Napi::Boolean::New(env, ok);
}

Napi::Value XrdNodeFile::SetXAttr(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsObject() || info[0].IsArray()) {
    Napi::TypeError::New(env, "SetXAttr expects an object (Record<string, string>)")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Object obj = info[0].As<Napi::Object>();
  Napi::Array keys = obj.GetPropertyNames();
  std::vector<XrdCl::xattr_t> attrs;

  for (uint32_t i = 0; i < keys.Length(); i++) {
    Napi::Value key = keys.Get(i);
    Napi::Value val = obj.Get(key);
    if (!key.IsString() || !val.IsString()) {
      Napi::TypeError::New(env, "SetXAttr expects string keys and values")
          .ThrowAsJavaScriptException();
      return env.Null();
    }
    attrs.push_back(
        std::make_tuple(key.As<Napi::String>().Utf8Value(), val.As<Napi::String>().Utf8Value())
    );
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FSXAttrStatusHandler(env, deferred, "SetXAttr");
  auto status = this->file_->SetXAttr(attrs, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::GetXAttr(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "GetXAttr expects an array of strings").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Array arr = info[0].As<Napi::Array>();
  std::vector<std::string> attrs;
  for (uint32_t i = 0; i < arr.Length(); i++) {
    Napi::Value val = arr.Get(i);
    if (!val.IsString()) {
      Napi::TypeError::New(env, "GetXAttr expects an array of strings")
          .ThrowAsJavaScriptException();
      return env.Null();
    }
    attrs.push_back(val.As<Napi::String>().Utf8Value());
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FSXAttrDataHandler(env, deferred, "GetXAttr");
  auto status = this->file_->GetXAttr(attrs, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::DelXAttr(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "DelXAttr expects an array of strings").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Array arr = info[0].As<Napi::Array>();
  std::vector<std::string> attrs;
  for (uint32_t i = 0; i < arr.Length(); i++) {
    Napi::Value item = arr.Get(i);
    if (item.IsString()) {
      attrs.push_back(item.As<Napi::String>().Utf8Value());
    }
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FSXAttrStatusHandler(env, deferred, "DelXAttr");
  XrdCl::XRootDStatus status = this->file_->DelXAttr(attrs, handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}

Napi::Value XrdNodeFile::ListXAttr(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto* handler = new FSXAttrDataHandler(env, deferred, "ListXAttr");
  auto status = this->file_->ListXAttr(handler);
  if (!status.IsOK()) {
    Napi::Error err = XrdNode::Utils::StatusToError(env, status);
    deferred.Reject(err.Value());
    delete handler;
  }
  return deferred.Promise();
}
