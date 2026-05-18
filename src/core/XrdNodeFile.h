#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>


class XrdNodeFile : public Napi::ObjectWrap<XrdNodeFile> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  XrdNodeFile(const Napi::CallbackInfo& info);
  ~XrdNodeFile() override;

  XrdCl::File* GetInternalFile() const { return file_; }

 private:
  XrdCl::File* file_;

  // 状态检查 (可同步)
  Napi::Value IsOpen(const Napi::CallbackInfo& info);

  // 异步 I/O，全数返回 Napi::Value (Promise)
  Napi::Value Open(const Napi::CallbackInfo& info);
  Napi::Value Close(const Napi::CallbackInfo& info);
  Napi::Value Stat(const Napi::CallbackInfo& info);
  Napi::Value Read(const Napi::CallbackInfo& info);
  Napi::Value Write(const Napi::CallbackInfo& info);
  Napi::Value Sync(const Napi::CallbackInfo& info);
  Napi::Value Truncate(const Napi::CallbackInfo& info);

  // 向量读写与特殊操作
  Napi::Value VectorRead(const Napi::CallbackInfo& info);
  Napi::Value ReadChunks(const Napi::CallbackInfo& info);

  // 属性与扩展属性
  Napi::Value GetProperty(const Napi::CallbackInfo& info);
  Napi::Value SetProperty(const Napi::CallbackInfo& info);
  Napi::Value SetXAttr(const Napi::CallbackInfo& info);
  Napi::Value GetXAttr(const Napi::CallbackInfo& info);
  Napi::Value DelXAttr(const Napi::CallbackInfo& info);
  Napi::Value ListXAttr(const Napi::CallbackInfo& info);

  Napi::Value Clone(const Napi::CallbackInfo& info);
};