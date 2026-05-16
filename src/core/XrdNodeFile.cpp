#include "XrdNodeFile.h"

Napi::Object XrdNodeFile::Init(Napi::Env env, Napi::Object exports) {
    // TODO
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
