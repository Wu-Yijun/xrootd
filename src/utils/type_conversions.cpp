#include "type_conversions.h"
#include <string>
#include <vector>

namespace XrdNode {
namespace Utils {

// ============================================================================
// 1. PropertyList 转换
// 目标: 将 XRootD 的属性列表 (键值对) 转换为 JavaScript 的 Object (Record<string, string>)
// ============================================================================
Napi::Object PropertyListToObject(Napi::Env env, const XrdCl::PropertyList* list) {
    Napi::Object obj = Napi::Object::New(env);
    if (!list) {
        return obj; // 传入空指针时返回空对象 {}
    }

    // 注意：XrdCl::PropertyList 在不同版本的 XRootD 中 API 可能略有差异。
    // 通常，它并不支持直接的基于范围的 for 循环 (range-based for)，
    // 而是通过获取键名列表来遍历。
    
    // 假设使用标准的 XRootD API (获取 Keys 并提取为 String)
    std::vector<std::string> keys = list->GetKeys();
    
    for (const std::string& key : keys) {
        std::string stringValue;
        
        // PropertyList 可能会存储不同类型的数据。
        // 为了安全起见并配合 TS 层的 Record<string, any>，我们优先尝试提取为字符串。
        // 在实际的物理数据处理中，XRootD 绝大部分的 property 都是以 string 形式传递的。
        if (list->GetString(key, stringValue)) {
            obj.Set(key, Napi::String::New(env, stringValue));
        } else {
            int intValue;
            if (list->GetInt(key, intValue)) {
                obj.Set(key, Napi::Number::New(env, intValue));
            } else {
                // 如果既不是字符串也不是整型，可以根据需要补充 GetBool 等，
                // 或者降级写入一个空字符串/提示信息。
                obj.Set(key, env.Null());
            }
        }
    }

    return obj;
}

// ============================================================================
// 2. AnyObject 转换
// 目标: 将 XRootD 的 AnyObject 转换为 Napi::Value
// ============================================================================
Napi::Value AnyObjectToValue(Napi::Env env, const XrdCl::AnyObject* obj) {
    // 深度分析：
    // 在 Python 的实现中 (template<> struct PyDict<XrdCl::AnyObject>)，
    // AnyObject 被强行映射为了 Python 的 None。
    // 原因是 XrdCl::AnyObject 通常被用作 C++ 底层的不透明指针 (Opaque Pointer)、
    // 闭包上下文 (Closure Context) 或非常底层的内存结构。
    // 将其强行序列化到 JS 层既不安全也没有实际意义，因此我们直接返回 null。
    
    return env.Null();
}

// ============================================================================
// 3. Status 转换
// 目标: 将底层的 XRootDStatus 封装为带有自定义属性的 JS Error 对象
// ============================================================================
Napi::Error StatusToError(Napi::Env env, const XrdCl::XRootDStatus& status) {
    // 1. 获取基础错误信息 (通常包含 code 和 message)
    std::string errMsg = status.ToString();
    
    // 2. 实例化基础的 Napi::Error
    Napi::Error err = Napi::Error::New(env, errMsg);
    
    // 3. 提取底层状态码并挂载到 Error 对象上
    // 使得在 TS 层可以通过 err.code, err.status, err.errNo 捕获并进行逻辑判断
    // 对应 types.ts 中的 XRootDError
    
    // err.Value() 返回 Error 在 V8 引擎中对应的 JS Object
    Napi::Object errObj = err.Value();
    
    // status.status 对应 XrdCl::errDataError, XrdCl::errErrorResponse 等枚举
    errObj.Set("status", Napi::Number::New(env, static_cast<uint32_t>(status.status)));
    
    // status.code 对应具体的协议状态码
    errObj.Set("code", Napi::Number::New(env, static_cast<uint32_t>(status.code)));
    
    // status.errNo 对应系统级的错误码 (如 POSIX 的 ENOENT)
    errObj.Set("errNo", Napi::Number::New(env, static_cast<uint32_t>(status.errNo)));
    
    return err;
}

} // namespace Utils
} // namespace XrdNode