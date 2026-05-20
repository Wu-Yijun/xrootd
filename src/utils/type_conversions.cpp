#include "type_conversions.h"

#include <string>
#include <vector>

#include "napi.h"

namespace XrdNode {
namespace Utils {

void JsObjectToProps(Napi::Env env, Napi::Object jsObj, XrdCl::PropertyList& props) {
  // 1. 获取 JS 对象的所有 Keys
  Napi::Array keys = jsObj.GetPropertyNames();

  // 2. 动态遍历
  for (uint32_t i = 0; i < keys.Length(); i++) {
    Napi::Value keyVal = keys.Get(i);
    std::string keyStr = keyVal.As<Napi::String>().Utf8Value();
    Napi::Value val = jsObj.Get(keyVal);

    // 3. 动态类型反射与分发
    if (val.IsString()) {
      props.Set(keyStr, val.As<Napi::String>().Utf8Value());
    } else if (val.IsBoolean()) {
      props.Set(keyStr, val.As<Napi::Boolean>().Value());
    } else if (val.IsNumber()) {
      // 根据 CopyProcess 头文件，大部分数字配置如 sourceLimit, chunkSize 都是 uint32_t
      props.Set(keyStr, val.As<Napi::Number>().Uint32Value());
    } else if (val.IsBigInt()) {
      // 对于 timeout 等可能需要 uint64_t/time_t 的字段
      bool lossless;
      props.Set(keyStr, val.As<Napi::BigInt>().Uint64Value(&lossless));
    } else if (val.IsArray()) {
      Napi::Array arr = val.As<Napi::Array>();
      for (uint32_t j = 0; j < arr.Length(); j++) {
        Napi::Value itemVal = arr.Get(j);
        props.Set(keyStr, j, itemVal.As<Napi::String>().Utf8Value());
      }
    } else if (val.IsObject()) {
      Napi::Object obj = val.As<Napi::Object>();
      auto sub_prop = std::make_shared<XrdCl::PropertyList>();
      JsObjectToProps(env, obj, *sub_prop);
      props.Set(keyStr, sub_prop);
    } else {
      // Skip unknown props
      // props.Set(keyStr, "undefined");
    }
  }
}

Napi::Object PropsToJsRecord(Napi::Env env, XrdCl::PropertyList& props) {
  Napi::Object jsObj = Napi::Object::New(env);
  for (auto it = props.begin(); it != props.end(); ++it) {
    const std::string& key = it->first;
    const std::string& value = it->second;
    jsObj.Set(key, Napi::String::New(env, value));
  }
  return jsObj;
}

// // ============================================================================
// // 1. PropertyList 转换 (不安全的, 不合适)
// // 目标: 将 XRootD 的属性列表 (键值对) 转换为 JavaScript 的 Object (Record<string, string>)
// // ============================================================================
Napi::Object PropertyListToObject(Napi::Env env, const XrdCl::PropertyList* list) {
  Napi::Object obj = Napi::Object::New(env);
  if (!list) {
    return obj;  // 传入空指针时返回空对象 {}
  }

  // XrdCl::PropertyList 内部维护的是 std::map<std::string, std::string>
  // 并且对外暴露了 begin() 和 end()。
  // 我们可以直接使用基于范围的 for 循环 (需解引用 list 指针) 或使用迭代器。

  for (auto it = list->begin(); it != list->end(); ++it) {
    // it->first 是 std::string 类型的 key
    // it->second 是 std::string 类型的 value
    const std::string& key = it->first;
    const std::string& value = it->second;

    // 因为底层全都是以字符串形式存储的，直接转换为 Napi::String 即可。
    // 这完美契合 Record<string, string> 的定义。
    obj.Set(key, Napi::String::New(env, value));
  }

  return obj;
}

// // ============================================================================
// // 2. AnyObject 转换
// // 目标: 将 XRootD 的 AnyObject 转换为 Napi::Value
// // ============================================================================
// Napi::Value AnyObjectToValue(Napi::Env env, const XrdCl::AnyObject* obj) {
//   // 深度分析：
//   // 在 Python 的实现中 (template<> struct PyDict<XrdCl::AnyObject>)，
//   // AnyObject 被强行映射为了 Python 的 None。
//   // 原因是 XrdCl::AnyObject 通常被用作 C++ 底层的不透明指针 (Opaque Pointer)、
//   // 闭包上下文 (Closure Context) 或非常底层的内存结构。
//   // 将其强行序列化到 JS 层既不安全也没有实际意义，因此我们直接返回 null。

//   return env.Null();
// }

// ============================================================================
// 3. Status 转换
// 目标: 将底层的 XRootDStatus 封装为带有自定义属性的 JS Error 对象
// 如果状态是 OK，则直接返回空的 Error。
// 只有当状态不是 OK 时，才会在 Error 对象上添加详细的错误信息。
// ============================================================================
Napi::Error StatusToOkError(Napi::Env env, const XrdCl::XRootDStatus& status) {
  // 实例化基础的 Napi::Error
  Napi::Error err = Napi::Error::New(env, status.ToString());
  // 3. 提取底层状态码并挂载到 Error 对象上
  Napi::Object errObj = err.Value();
  bool is_ok = status.IsOK();
  errObj.Set("ok", Napi::Boolean::New(env, is_ok));
  if (is_ok) {
    return err;
  }
  // status.status:  0: Ok, 1: error, 2: fatal
  errObj.Set("xrdStatus", Napi::Number::New(env, static_cast<uint32_t>(status.status)));
  // status.code 对应具体的协议状态码
  errObj.Set("xrdCode", Napi::Number::New(env, static_cast<uint32_t>(status.code)));
  // status.errNo 对应系统级的错误码 (如 POSIX 的 ENOENT)
  errObj.Set("xrdErrNo", Napi::Number::New(env, static_cast<uint32_t>(status.errNo)));
  // 详细的错误信息
  errObj.Set("xrdErrMsg", Napi::String::New(env, status.GetErrorMessage()));
  // 返回错误
  return err;
}

// ============================================================================
// 4. StatInfo 转换
// ============================================================================
Napi::Object StatInfoToObject(Napi::Env env, const XrdCl::StatInfo* statInfo) {
  Napi::Object result = Napi::Object::New(env);
  if (!statInfo) return result;

  result.Set("id", Napi::String::New(env, statInfo->GetId()));
  result.Set("size", Napi::BigInt::New(env, statInfo->GetSize()));
  result.Set("flags", Napi::Number::New(env, statInfo->GetFlags()));
  result.Set("modTime", Napi::Number::New(env, statInfo->GetModTime()));
  result.Set("accessTime", Napi::Number::New(env, statInfo->GetAccessTime()));
  result.Set("changeTime", Napi::Number::New(env, statInfo->GetChangeTime()));

  result.Set("modTimeAsString", Napi::String::New(env, statInfo->GetModTimeAsString()));
  result.Set("accessTimeAsString", Napi::String::New(env, statInfo->GetAccessTimeAsString()));
  result.Set("changeTimeAsString", Napi::String::New(env, statInfo->GetChangeTimeAsString()));

  result.Set("modeAsString", Napi::String::New(env, statInfo->GetModeAsString()));
  result.Set("modeAsOctString", Napi::String::New(env, statInfo->GetModeAsOctString()));

  result.Set("owner", Napi::String::New(env, statInfo->GetOwner()));
  result.Set("group", Napi::String::New(env, statInfo->GetGroup()));
  result.Set("checksum", Napi::String::New(env, statInfo->GetChecksum()));

  return result;
}

// ============================================================================
// 5. XAttr 转换
// ============================================================================
std::vector<XrdCl::xattr_t> ObjectToXAttrVector(Napi::Env env, Napi::Object obj) {
  std::vector<XrdCl::xattr_t> vec;
  Napi::Array keys = obj.GetPropertyNames();
  uint32_t len = keys.Length();

  for (uint32_t i = 0; i < len; ++i) {
    Napi::Value keyVal = keys.Get(i);
    std::string key = keyVal.As<Napi::String>().Utf8Value();
    Napi::Value valVal = obj.Get(keyVal);
    std::string val = valVal.As<Napi::String>().Utf8Value();

    vec.push_back(std::make_tuple(key, val));
  }

  return vec;
}

Napi::Object XAttrVectorToObject(Napi::Env env, const std::vector<XrdCl::XAttr>& attrs) {
  Napi::Object obj = Napi::Object::New(env);

  for (const auto& attr : attrs) {
    if (attr.status.IsOK()) {
      obj.Set(attr.name, Napi::String::New(env, attr.value));
    }
  }

  return obj;
}

}  // namespace Utils
}  // namespace XrdNode