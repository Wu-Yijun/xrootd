#include "XrdNodeEnv.h"

Napi::Object XrdNodeEnv::Init(Napi::Env env, Napi::Object exports) {
    // 1. 创建一个空的 JS 对象: {}
    Napi::Object envObj = Napi::Object::New(env);

    // 2. 把 C++ 静态方法包装成 JS 函数，并挂载到 envObj 上
    envObj.Set("PutString", Napi::Function::New(env, XrdNodeEnv::PutString));
    envObj.Set("GetString", Napi::Function::New(env, XrdNodeEnv::GetString));
    envObj.Set("PutInt", Napi::Function::New(env, XrdNodeEnv::PutInt));
    envObj.Set("GetInt", Napi::Function::New(env, XrdNodeEnv::GetInt));

    // 3. 将 envObj 挂载到导出模块上
    // 等价于 JS 中的: exports.Env = { PutString: ..., GetString: ... }
    exports.Set("Env", envObj);

    return exports;
}

Napi::Value XrdNodeEnv::PutString(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeEnv::GetString(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeEnv::PutInt(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}

Napi::Value XrdNodeEnv::GetInt(const Napi::CallbackInfo& info) {
    // TODO
    return Napi::Value();
}
