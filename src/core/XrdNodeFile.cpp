#include "XrdNodeFile.h"

#include "handlers/FileControlHandler.h"
#include "handlers/FileReadHandler.h"
#include "handlers/FileWriteHandler.h"


using XrdNode::FileReadHandler, XrdNode::FileWriteHandler,
    XrdNode::FileControlHandler;

Napi::Object XrdNodeFile::Init(Napi::Env env, Napi::Object exports) {
  // 1. 定义 JS 类的名称和它原型链上的所有方法
  Napi::Function func =
      DefineClass(env, "File",
                  {// InstanceMethod 会把 C++ 方法挂载到 JS 的 prototype 上
                   InstanceMethod("Open", &XrdNodeFile::Open),
                   InstanceMethod("Close", &XrdNodeFile::Close),
                   InstanceMethod("Stat", &XrdNodeFile::Stat),
                   InstanceMethod("Read", &XrdNodeFile::Read),
                   InstanceMethod("Write", &XrdNodeFile::Write),
                   InstanceMethod("Sync", &XrdNodeFile::Sync),
                   InstanceMethod("Truncate", &XrdNodeFile::Truncate),

                   // 向量读取
                   InstanceMethod("VectorRead", &XrdNodeFile::VectorRead),
                   InstanceMethod("ReadChunks", &XrdNodeFile::ReadChunks),

                   // 同步方法
                   InstanceMethod("IsOpen", &XrdNodeFile::IsOpen),

                   // 扩展属性
                   InstanceMethod("GetProperty", &XrdNodeFile::GetProperty),
                   InstanceMethod("SetProperty", &XrdNodeFile::SetProperty),
                   InstanceMethod("SetXAttr", &XrdNodeFile::SetXAttr),
                   InstanceMethod("GetXAttr", &XrdNodeFile::GetXAttr),
                   InstanceMethod("DelXAttr", &XrdNodeFile::DelXAttr),
                   InstanceMethod("ListXAttr", &XrdNodeFile::ListXAttr),

                   InstanceMethod("Clone", &XrdNodeFile::Clone)});

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
XrdNodeFile::XrdNodeFile(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<XrdNodeFile>(info) {

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

Napi::Value XrdNodeFile::IsOpen(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

// ============================================================================
// 1. Open 接口实现
// ============================================================================
Napi::Value XrdNodeFile::Open(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // 参数提取与校验
  if (info.Length() < 3 || !info[0].IsString() || !info[1].IsNumber() ||
      !info[2].IsNumber()) {
    Napi::TypeError::New(env, "Invalid arguments for Open")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string url = info[0].As<Napi::String>().Utf8Value();
  uint32_t flags = info[1].As<Napi::Number>().Uint32Value();
  uint32_t mode = info[2].As<Napi::Number>().Uint32Value();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  // 实例化复用的控制 Handler
  auto *handler = new FileControlHandler(env, deferred, "Open");

  // 调用 XRootD 异步 Open，瞬间返回
  XrdCl::XRootDStatus status =
      this->file_->Open(url, static_cast<XrdCl::OpenFlags::Flags>(flags),
                        static_cast<XrdCl::Access::Mode>(mode), handler);

  if (!status.IsOK()) {
    // 如果发起异步请求本身就失败了（例如本地状态错误），立即销毁 handler 并
    // Reject
    handler->HandleResponse(&status, nullptr);
  }

  return deferred.Promise();
}

// ============================================================================
// 2. Close 接口实现
// ============================================================================
Napi::Value XrdNodeFile::Close(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  auto *handler = new FileControlHandler(env, deferred, "Close");

  // 调用 XRootD 异步 Close
  XrdCl::XRootDStatus status = this->file_->Close(handler);

  if (!status.IsOK()) {
    handler->HandleResponse(&status, nullptr);
  }

  return deferred.Promise();
}

// ============================================================================
// 3. Clone 接口实现（核心：解包 JS 数组并提取跨实例的底层指针）
// ============================================================================
Napi::Value XrdNodeFile::Clone(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "Clone expects an array of locations")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Array arr = info[0].As<Napi::Array>();
  XrdCl::CloneLocations locs;
  bool lossless;

  // 遍历 JS 传进来的克隆任务配置数组
  for (uint32_t i = 0; i < arr.Length(); i++) {
    Napi::Value itemValue = arr.Get(i);
    if (!itemValue.IsObject())
      continue;

    Napi::Object item = itemValue.As<Napi::Object>();
    Napi::Object srcFileObj = item.Get("srcFile").As<Napi::Object>();

    // 核心安全技巧：从传入的 JS File 对象中，反解出它对应的 C++ 包装类实例
    // 从而直接拿到它内部持有的原生 XrdCl::File* 指针
    XrdNodeFile *srcNodeFile =
        Napi::ObjectWrap<XrdNodeFile>::Unwrap(srcFileObj);
    if (!srcNodeFile || !srcNodeFile->GetInternalFile()) {
      Napi::TypeError::New(env, "Invalid source file object in clone locations")
          .ThrowAsJavaScriptException();
      return env.Null();
    }

    // 提取大整数偏移量，严格校验精度
    int64_t dstOffs =
        item.Get("dstOffset").As<Napi::BigInt>().Int64Value(&lossless);
    int64_t srcOffs =
        item.Get("srcOffset").As<Napi::BigInt>().Int64Value(&lossless);
    int64_t srcLen =
        item.Get("length").As<Napi::BigInt>().Int64Value(&lossless);

    if (!lossless) {
      Napi::RangeError::New(
          env, "Clone offset or length exceeds 64-bit integer limits")
          .ThrowAsJavaScriptException();
      return env.Null();
    }

    // 将原生指针解引用后，塞入 XRootD 的克隆任务清单中
    locs.Add(*(srcNodeFile->GetInternalFile()), dstOffs, srcOffs, srcLen);
  }

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
  auto *handler = new FileControlHandler(env, deferred, "Clone");

  // 发起服务端的异步克隆请求
  XrdCl::XRootDStatus status = this->file_->Clone(locs, handler);

  if (!status.IsOK()) {
    handler->HandleResponse(&status, nullptr);
  }

  return deferred.Promise();
}

Napi::Value XrdNodeFile::Stat(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::Read(const Napi::CallbackInfo &info) {
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
  auto *handler = new FileReadHandler(env, deferred, size);

  // 瞬间异步调用，将 C++ 分配的 buffer_ 指针传进去
  this->file_->Read(offset, size, handler->GetBuffer(), handler);

  return deferred.Promise();
}

Napi::Value XrdNodeFile::Write(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // 1. 提前声明一个布尔变量
  bool lossless;
  uint64_t offset = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
  // 3. (可选但推荐的防御性编程) 检查是否丢失了精度
  if (!lossless) {
    Napi::TypeError::New(env, "Offset value is out of bounds for uint64_t")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  // 从 TS 接收 Buffer
  Napi::Buffer<char> jsBuffer = info[1].As<Napi::Buffer<char>>();

  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  auto *handler = new FileWriteHandler(env, deferred, jsBuffer);

  // 瞬间异步调用，直接传入 JS Buffer 的底层指针和长度
  this->file_->Write(offset, jsBuffer.Length(), jsBuffer.Data(), handler);

  return deferred.Promise();
}

Napi::Value XrdNodeFile::Sync(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::Truncate(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::VectorRead(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::ReadChunks(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::GetProperty(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::SetProperty(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::SetXAttr(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::GetXAttr(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::DelXAttr(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}

Napi::Value XrdNodeFile::ListXAttr(const Napi::CallbackInfo &info) {
  // TODO
  return Napi::Value();
}
