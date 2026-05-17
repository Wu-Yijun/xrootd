#include "XrdNodeEnv.h"
#include <XrdCl/XrdClDefaultEnv.hh>

// 辅助宏：用于快速抛出类型错误，保持代码整洁
#define THROW_TYPE_ERROR_RETURN_NULL(env, msg) \
    do { \
        Napi::TypeError::New(env, msg).ThrowAsJavaScriptException(); \
        return env.Null(); \
    } while (0)

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
    Napi::Env env = info.Env();

    // 1. 严格的参数校验：需要 2 个参数，且都必须是字符串
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        THROW_TYPE_ERROR_RETURN_NULL(env, "PutString expects 2 arguments: (key: string, value: string)");
    }

    // 2. 提取 C++ std::string
    std::string key = info[0].As<Napi::String>().Utf8Value();
    std::string value = info[1].As<Napi::String>().Utf8Value();

    // 3. 调用 XRootD 底层 API (设置全局环境变量)
    XrdCl::DefaultEnv::GetEnv()->PutString(key, value);

    // 4. 返回 undefined (在 C++ 中用 env.Undefined() 表示)
    return env.Undefined();
}

Napi::Value XrdNodeEnv::GetString(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        THROW_TYPE_ERROR_RETURN_NULL(env, "GetString expects 1 argument: (key: string)");
    }

    std::string key = info[0].As<Napi::String>().Utf8Value();
    std::string value;

    // XRootD 的 GetString 通常返回一个 bool，指示该 key 是否存在
    if (XrdCl::DefaultEnv::GetEnv()->GetString(key, value)) {
        return Napi::String::New(env, value);
    } else {
        // 如果环境变量不存在，返回 JS 的 null
        return env.Null();
    }
}

Napi::Value XrdNodeEnv::PutInt(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // 校验：键必须是字符串，值必须是数字
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        THROW_TYPE_ERROR_RETURN_NULL(env, "PutInt expects 2 arguments: (key: string, value: number)");
    }

    std::string key = info[0].As<Napi::String>().Utf8Value();
    // JS 的 Number 是双精度浮点，底层通常需要 int，这里做安全的强制转换
    int value = info[1].As<Napi::Number>().Int32Value();

    XrdCl::DefaultEnv::GetEnv()->PutInt(key, value);

    return env.Undefined();
}

Napi::Value XrdNodeEnv::GetInt(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        THROW_TYPE_ERROR_RETURN_NULL(env, "GetInt expects 1 argument: (key: string)");
    }

    std::string key = info[0].As<Napi::String>().Utf8Value();
    int value;

    if (XrdCl::DefaultEnv::GetEnv()->GetInt(key, value)) {
        return Napi::Number::New(env, value);
    } else {
        return env.Null();
    }
}