#include "FileWorkers.h"

namespace XrdNode {
namespace Workers {

FileOpenWorker::FileOpenWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file, const std::string& url, uint16_t flags, uint16_t mode)
    : Napi::AsyncWorker(env), deferred_(deferred), file_(file), url_(url), flags_(flags), mode_(mode) {
    // TODO
}

void FileOpenWorker::Execute() {
    // TODO
}

void FileOpenWorker::OnOK() {
    // TODO
}

void FileOpenWorker::OnError(const Napi::Error& e) {
    // TODO
}

FileCloseWorker::FileCloseWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file)
    : Napi::AsyncWorker(env), deferred_(deferred), file_(file) {
    // TODO
}

void FileCloseWorker::Execute() {
    // TODO
}

void FileCloseWorker::OnOK() {
    // TODO
}

void FileCloseWorker::OnError(const Napi::Error& e) {
    // TODO
}

FileStatWorker::FileStatWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file, bool force)
    : Napi::AsyncWorker(env), deferred_(deferred), file_(file), force_(force) {
    // TODO
}

void FileStatWorker::Execute() {
    // TODO
}

void FileStatWorker::OnOK() {
    // TODO
}

void FileStatWorker::OnError(const Napi::Error& e) {
    // TODO
}

FileReadWorker::FileReadWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file, uint64_t offset, uint32_t size)
    : Napi::AsyncWorker(env), deferred_(deferred), file_(file), offset_(offset), size_(size) {
    // TODO
}

void FileReadWorker::Execute() {
    // TODO
}

void FileReadWorker::OnOK() {
    // TODO
}

void FileReadWorker::OnError(const Napi::Error& e) {
    // TODO
}

FileWriteWorker::FileWriteWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file, uint64_t offset, Napi::Buffer<char> js_buffer)
    : Napi::AsyncWorker(env), deferred_(deferred), file_(file), offset_(offset) {
    // TODO
}

void FileWriteWorker::Execute() {
    // TODO
}

void FileWriteWorker::OnOK() {
    // TODO
}

void FileWriteWorker::OnError(const Napi::Error& e) {
    // TODO
}

FileSyncWorker::FileSyncWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file)
    : Napi::AsyncWorker(env), deferred_(deferred), file_(file) {
    // TODO
}

void FileSyncWorker::Execute() {
    // TODO
}

void FileSyncWorker::OnOK() {
    // TODO
}

void FileSyncWorker::OnError(const Napi::Error& e) {
    // TODO
}

} // namespace Workers
} // namespace XrdNode
