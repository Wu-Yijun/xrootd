#pragma once
#include <napi.h>
#include <XrdCl/XrdClFileSystem.hh>

class XrdNodeFileSystem : public Napi::ObjectWrap<XrdNodeFileSystem> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    XrdNodeFileSystem(const Napi::CallbackInfo& info);
    ~XrdNodeFileSystem() override;

    XrdCl::FileSystem* GetInternalFS() const { return fs_; }

private:
    XrdCl::FileSystem* fs_;

    // 异步 I/O
    Napi::Value Locate(const Napi::CallbackInfo& info);
    Napi::Value DeepLocate(const Napi::CallbackInfo& info);
    Napi::Value Mv(const Napi::CallbackInfo& info);
    Napi::Value Query(const Napi::CallbackInfo& info);
    Napi::Value Truncate(const Napi::CallbackInfo& info);
    Napi::Value Rm(const Napi::CallbackInfo& info);
    Napi::Value MkDir(const Napi::CallbackInfo& info);
    Napi::Value RmDir(const Napi::CallbackInfo& info);
    Napi::Value ChMod(const Napi::CallbackInfo& info);
    Napi::Value Ping(const Napi::CallbackInfo& info);
    Napi::Value Stat(const Napi::CallbackInfo& info);
    Napi::Value StatVFS(const Napi::CallbackInfo& info);
    Napi::Value Protocol(const Napi::CallbackInfo& info);
    Napi::Value DirList(const Napi::CallbackInfo& info);
    Napi::Value SendInfo(const Napi::CallbackInfo& info);
    Napi::Value Prepare(const Napi::CallbackInfo& info);
    Napi::Value Cat(const Napi::CallbackInfo& info);

    // 属性与扩展属性
    Napi::Value GetProperty(const Napi::CallbackInfo& info);
    Napi::Value SetProperty(const Napi::CallbackInfo& info);
    Napi::Value SetXAttr(const Napi::CallbackInfo& info);
    Napi::Value GetXAttr(const Napi::CallbackInfo& info);
    Napi::Value DelXAttr(const Napi::CallbackInfo& info);
    Napi::Value ListXAttr(const Napi::CallbackInfo& info);
};