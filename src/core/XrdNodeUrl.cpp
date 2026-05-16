#include "XrdNodeUrl.h"

Napi::Object XrdNodeUrl::Init(Napi::Env env, Napi::Object exports) {
    // TODO
    return exports;
}

XrdNodeUrl::XrdNodeUrl(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<XrdNodeUrl>(info) {
    // TODO
}

XrdNodeUrl::~XrdNodeUrl() {
    // TODO
}

Napi::Value XrdNodeUrl::IsValid(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::Clear(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::ToString(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::GetHostId(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::GetProtocol(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::GetUserName(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::GetPassword(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::GetHostName(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::GetPort(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::GetPath(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeUrl::GetPathWithParams(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

void XrdNodeUrl::SetProtocol(const Napi::CallbackInfo& info, const Napi::Value& value) {
    // TODO
}

void XrdNodeUrl::SetUserName(const Napi::CallbackInfo& info, const Napi::Value& value) {
    // TODO
}

void XrdNodeUrl::SetPassword(const Napi::CallbackInfo& info, const Napi::Value& value) {
    // TODO
}

void XrdNodeUrl::SetHostName(const Napi::CallbackInfo& info, const Napi::Value& value) {
    // TODO
}

void XrdNodeUrl::SetPort(const Napi::CallbackInfo& info, const Napi::Value& value) {
    // TODO
}

void XrdNodeUrl::SetPath(const Napi::CallbackInfo& info, const Napi::Value& value) {
    // TODO
}
