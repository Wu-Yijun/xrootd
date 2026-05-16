#pragma once
#include <napi.h>
#include <XrdCl/XrdClXRootDResponses.hh>

namespace XrdNode {
namespace Utils {

    // 将 XrdCl::PropertyList 转化为 JS Object (Record<string, any>)
    Napi::Object PropertyListToObject(Napi::Env env, const XrdCl::PropertyList* list);

    // 将 XrdCl::AnyObject 提取并转化为合适的 Napi::Value
    Napi::Value AnyObjectToValue(Napi::Env env, const XrdCl::AnyObject* obj);

    // 基于 XrdCl::XRootDStatus 生成携带 code/status 的 Napi::Error
    Napi::Error StatusToError(Napi::Env env, const XrdCl::XRootDStatus& status);

} // namespace Utils
} // namespace XrdNode