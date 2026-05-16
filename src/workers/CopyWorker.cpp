#include "CopyWorker.h"

namespace XrdNode {
namespace Workers {

NodeCopyProgressHandler::NodeCopyProgressHandler(Napi::ThreadSafeFunction tsfn)
    : tsfn_(tsfn) {
    // TODO
}

NodeCopyProgressHandler::~NodeCopyProgressHandler() {
    // TODO
}

void NodeCopyProgressHandler::BeginJob(uint32_t jobNum, uint32_t jobTotal, const XrdCl::URL* source, const XrdCl::URL* target) {
    // TODO
}

void NodeCopyProgressHandler::EndJob(uint32_t jobNum, const XrdCl::PropertyList* result) {
    // TODO
}

void NodeCopyProgressHandler::JobProgress(uint32_t jobNum, uint64_t bytesProcessed, uint64_t bytesTotal) {
    // TODO
}

bool NodeCopyProgressHandler::ShouldCancel(uint32_t jobNum) {
    // TODO
    return false;
}

void NodeCopyProgressHandler::Cancel(uint32_t jobNum) {
    // TODO
}

CopyRunWorker::CopyRunWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::CopyProcess* cp, NodeCopyProgressHandler* handler)
    : Napi::AsyncWorker(env), deferred_(deferred), cp_(cp), handler_(handler) {
    // TODO
}

void CopyRunWorker::Execute() {
    // TODO
}

void CopyRunWorker::OnOK() {
    // TODO
}

void CopyRunWorker::OnError(const Napi::Error& e) {
    // TODO
}

} // namespace Workers
} // namespace XrdNode
