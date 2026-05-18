#pragma once
#include <napi.h>

#include <XrdCl/XrdClCopyProcess.hh>


class XrdNodeCopyProcess : public Napi::ObjectWrap<XrdNodeCopyProcess> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  XrdNodeCopyProcess(const Napi::CallbackInfo& info);
  ~XrdNodeCopyProcess() override;

  XrdCl::CopyProcess* GetInternalCopyProcess() const { return cp_; }

 private:
  XrdCl::CopyProcess* cp_;

  Napi::Value AddJob(const Napi::CallbackInfo& info);
  Napi::Value Prepare(const Napi::CallbackInfo& info);
  Napi::Value Run(const Napi::CallbackInfo& info);

  // JS 端主动发送取消信号，更新内部标记，Worker 在 ShouldCancel 中读取
  Napi::Value CancelJob(const Napi::CallbackInfo& info);

  // 用于在 TS 层注册事件监听器 (如 on('progress', callback))
  Napi::Value SetEventListener(const Napi::CallbackInfo& info);
};