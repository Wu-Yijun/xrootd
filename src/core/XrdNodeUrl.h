#pragma once
#include <napi.h>
#include <XrdCl/XrdClURL.hh>

class XrdNodeUrl : public Napi::ObjectWrap<XrdNodeUrl> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    XrdNodeUrl(const Napi::CallbackInfo& info);
    ~XrdNodeUrl() override;

    XrdCl::URL* GetInternalUrl() const { return url_; }

private:
    XrdCl::URL* url_;

    // 同步方法
    Napi::Value IsValid(const Napi::CallbackInfo& info);
    Napi::Value Clear(const Napi::CallbackInfo& info);
    Napi::Value ToString(const Napi::CallbackInfo& info);

    // 属性访问器 (Getters)
    Napi::Value GetHostId(const Napi::CallbackInfo& info);
    Napi::Value GetProtocol(const Napi::CallbackInfo& info);
    Napi::Value GetUserName(const Napi::CallbackInfo& info);
    Napi::Value GetPassword(const Napi::CallbackInfo& info);
    Napi::Value GetHostName(const Napi::CallbackInfo& info);
    Napi::Value GetPort(const Napi::CallbackInfo& info);
    Napi::Value GetPath(const Napi::CallbackInfo& info);
    Napi::Value GetPathWithParams(const Napi::CallbackInfo& info);

    // 属性访问器 (Setters)
    void SetProtocol(const Napi::CallbackInfo& info, const Napi::Value& value);
    void SetUserName(const Napi::CallbackInfo& info, const Napi::Value& value);
    void SetPassword(const Napi::CallbackInfo& info, const Napi::Value& value);
    void SetHostName(const Napi::CallbackInfo& info, const Napi::Value& value);
    void SetPort(const Napi::CallbackInfo& info, const Napi::Value& value);
    void SetPath(const Napi::CallbackInfo& info, const Napi::Value& value);
};