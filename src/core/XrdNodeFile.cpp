#include "XrdNodeFile.h"

Napi::Object XrdNodeFile::Init(Napi::Env env, Napi::Object exports) {
    // 1. 定义 JS 类的名称和它原型链上的所有方法
    Napi::Function func = DefineClass(env, "File", {
        // InstanceMethod 会把 C++ 方法挂载到 JS 的 prototype 上
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
        
        InstanceMethod("Clone", &XrdNodeFile::Clone)
    });

    // (可选) 如果你需要在 C++ 内部 (比如在 Clone 方法中) 实例化这个 JS 对象，
    // 你需要把这个 func (构造函数) 保存到一个静态的 Napi::FunctionReference 中。
    // constructor() = Napi::Persistent(func);
    // constructor().SuppressDestruct();

    // 2. 将创建好的 JS 构造函数挂载到 exports 对象上
    // 等价于 JavaScript 中的：exports.File = function File() { ... }
    exports.Set("File", func);

    return exports;
}

XrdNodeFile::XrdNodeFile(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<XrdNodeFile>(info) {
    // TODO
}

XrdNodeFile::~XrdNodeFile() {
    // TODO
}

Napi::Value XrdNodeFile::IsOpen(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::Open(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::Close(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::Stat(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::Read(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::Write(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::Sync(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::Truncate(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::VectorRead(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::ReadChunks(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::GetProperty(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::SetProperty(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::SetXAttr(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::GetXAttr(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::DelXAttr(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::ListXAttr(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFile::Clone(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}
