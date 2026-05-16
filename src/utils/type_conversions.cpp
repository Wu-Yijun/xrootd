#include "type_conversions.h"

namespace XrdNode {
namespace Utils {

Napi::Object PropertyListToObject(Napi::Env env, const XrdCl::PropertyList* list) {
    // TODO
    return Napi::Object();
}

Napi::Value AnyObjectToValue(Napi::Env env, const XrdCl::AnyObject* obj) {
    // TODO
    return Napi::Value();
}

Napi::Error StatusToError(Napi::Env env, const XrdCl::XRootDStatus& status) {
    // TODO
    return Napi::Error();
}

} // namespace Utils
} // namespace XrdNode
