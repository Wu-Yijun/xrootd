#include "FileSystemWorkers.h"

namespace XrdNode {
namespace Workers {

FSLocateWorker::FSLocateWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::FileSystem* fs, const std::string& path, uint16_t flags, bool deep)
    : Napi::AsyncWorker(env), deferred_(deferred), fs_(fs), path_(path), flags_(flags), deep_(deep) {
    // TODO
}

void FSLocateWorker::Execute() {
    // TODO
}

void FSLocateWorker::OnOK() {
    // TODO
}

void FSLocateWorker::OnError(const Napi::Error& e) {
    // TODO
}

FSBasicOpWorker::FSBasicOpWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::FileSystem* fs, Action action, const std::string& path, uint16_t mode_or_flags, uint64_t size)
    : Napi::AsyncWorker(env), deferred_(deferred), fs_(fs), action_(action), path_(path), mode_or_flags_(mode_or_flags), size_(size) {
    // TODO
}

void FSBasicOpWorker::Execute() {
    // TODO
}

void FSBasicOpWorker::OnOK() {
    // TODO
}

void FSBasicOpWorker::OnError(const Napi::Error& e) {
    // TODO
}

FSMvWorker::FSMvWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::FileSystem* fs, const std::string& source, const std::string& dest)
    : Napi::AsyncWorker(env), deferred_(deferred), fs_(fs), source_(source), dest_(dest) {
    // TODO
}

void FSMvWorker::Execute() {
    // TODO
}

void FSMvWorker::OnOK() {
    // TODO
}

void FSMvWorker::OnError(const Napi::Error& e) {
    // TODO
}

FSDirListWorker::FSDirListWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::FileSystem* fs, const std::string& path, uint16_t flags)
    : Napi::AsyncWorker(env), deferred_(deferred), fs_(fs), path_(path), flags_(flags) {
    // TODO
}

void FSDirListWorker::Execute() {
    // TODO
}

void FSDirListWorker::OnOK() {
    // TODO
}

void FSDirListWorker::OnError(const Napi::Error& e) {
    // TODO
}

FSStatWorker::FSStatWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::FileSystem* fs, const std::string& path, bool is_vfs)
    : Napi::AsyncWorker(env), deferred_(deferred), fs_(fs), path_(path), is_vfs_(is_vfs) {
    // TODO
}

void FSStatWorker::Execute() {
    // TODO
}

void FSStatWorker::OnOK() {
    // TODO
}

void FSStatWorker::OnError(const Napi::Error& e) {
    // TODO
}

} // namespace Workers
} // namespace XrdNode
