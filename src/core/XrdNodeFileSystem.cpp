#include "XrdNodeFileSystem.h"

Napi::Object XrdNodeFileSystem::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "FileSystem", {
        InstanceMethod("Locate", &XrdNodeFileSystem::Locate),
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
        InstanceMethod("Cat", &XrdNodeFileSystem::Cat),
        InstanceMethod("GetProperty", &XrdNodeFileSystem::GetProperty),
        InstanceMethod("SetProperty", &XrdNodeFileSystem::SetProperty),
        InstanceMethod("SetXAttr", &XrdNodeFileSystem::SetXAttr),
        InstanceMethod("GetXAttr", &XrdNodeFileSystem::GetXAttr),
        InstanceMethod("DelXAttr", &XrdNodeFileSystem::DelXAttr),
        InstanceMethod("ListXAttr", &XrdNodeFileSystem::ListXAttr)
    });

    exports.Set("FileSystem", func);

    return exports;
}

XrdNodeFileSystem::XrdNodeFileSystem(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<XrdNodeFileSystem>(info) {
    // TODO
}

XrdNodeFileSystem::~XrdNodeFileSystem() {
    // TODO
}

Napi::Value XrdNodeFileSystem::Locate(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::DeepLocate(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Mv(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Query(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Truncate(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Rm(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::MkDir(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::RmDir(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::ChMod(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Ping(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Stat(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::StatVFS(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Protocol(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::DirList(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::SendInfo(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Prepare(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::Cat(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::GetProperty(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::SetProperty(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::SetXAttr(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::GetXAttr(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::DelXAttr(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeFileSystem::ListXAttr(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}
