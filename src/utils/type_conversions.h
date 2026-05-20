#pragma once
#include <napi.h>

#include <XrdCl/XrdClPropertyList.hh>
#include <XrdCl/XrdClXRootDResponses.hh>

namespace XrdNode {
namespace Utils {

/**
 * 将 JS object 安全的转化为 XrdCl::PropertyList
 *
 * 支持的类型:
 * - string
 * - number
 * - bigint
 * - boolean
 * - array
 * - object
 */
void JsObjectToProps(Napi::Env env, Napi::Object jsObj, XrdCl::PropertyList& props);

/**
 * 将非嵌套的PropertyList 直接写入为 js Record<string, string> (可能不安全)
 */
Napi::Object PropsToJsRecord(Napi::Env env, const XrdCl::PropertyList& props);

// // 将 XrdCl::PropertyList 转化为 JS Object (Record<string, any>)
// Napi::Object PropertyListToObject(Napi::Env env, const XrdCl::PropertyList* list);

// // 将 XrdCl::AnyObject 提取并转化为合适的 Napi::Value
// Napi::Value AnyObjectToValue(Napi::Env env, const XrdCl::AnyObject* obj);

// 基于 XrdCl::XRootDStatus 生成携带 code/status 的 Napi::Error
Napi::Error StatusToOkError(Napi::Env env, const XrdCl::XRootDStatus& status);

// 将 XrdCl::StatInfo 转化为 JS Object
Napi::Object StatInfoToObject(Napi::Env env, const XrdCl::StatInfo* statInfo);

// 将 JS Object 转化为 XrdCl::xattr_t 向量
std::vector<XrdCl::xattr_t> ObjectToXAttrVector(Napi::Env env, Napi::Object obj);

// 将 XrdCl::XAttr 向量转化为 JS Object
Napi::Object XAttrVectorToObject(Napi::Env env, const std::vector<XrdCl::XAttr>& attrs);

}  // namespace Utils
}  // namespace XrdNode