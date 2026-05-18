#pragma once
#include <napi.h>

class XrdNodeEnv {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

 private:
  static Napi::Value PutString(const Napi::CallbackInfo& info);
  static Napi::Value GetString(const Napi::CallbackInfo& info);
  static Napi::Value PutInt(const Napi::CallbackInfo& info);
  static Napi::Value GetInt(const Napi::CallbackInfo& info);
};