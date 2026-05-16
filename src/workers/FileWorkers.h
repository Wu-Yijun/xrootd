#pragma once
#include <napi.h>
#include <XrdCl/XrdClFile.hh>
#include <XrdCl/XrdClXRootDResponses.hh>
#include <vector>
#include <string>

namespace XrdNode {
namespace Workers {

// 1. 打开文件
class FileOpenWorker : public Napi::AsyncWorker {
public:
    FileOpenWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                   XrdCl::File* file, const std::string& url, uint16_t flags, uint16_t mode);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    std::string url_;
    uint16_t flags_;
    uint16_t mode_;
    XrdCl::XRootDStatus status_;
};

// 2. 关闭文件
class FileCloseWorker : public Napi::AsyncWorker {
public:
    FileCloseWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    XrdCl::XRootDStatus status_;
};

// 3. 文件 Stat (返回 PropertyList)
class FileStatWorker : public Napi::AsyncWorker {
public:
    FileStatWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file, bool force = false);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    bool force_;
    XrdCl::StatInfo* stat_info_; // 底层申请，OnOK 中转为 JS Object 并释放
    XrdCl::XRootDStatus status_;
};

// 4. 读文件 (Zero-Copy 核心)
class FileReadWorker : public Napi::AsyncWorker {
public:
    FileReadWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                   XrdCl::File* file, uint64_t offset, uint32_t size);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    uint64_t offset_;
    uint32_t size_;
    XrdCl::Buffer* result_buffer_; // 由 C++ 分配，在 OnOK 包装为 Napi::Buffer 移交
    XrdCl::XRootDStatus status_;
};

// 5. 写文件 (注意：需要 Pin 住 JS Buffer 防止 GC)
class FileWriteWorker : public Napi::AsyncWorker {
public:
    FileWriteWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                    XrdCl::File* file, uint64_t offset, Napi::Buffer<char> js_buffer);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    uint64_t offset_;
    Napi::Reference<Napi::Buffer<char>> buffer_ref_; // 强引用 JS Buffer
    const char* data_ptr_;
    uint32_t data_size_;
    XrdCl::XRootDStatus status_;
};

// 6. 同步文件 (Sync)
class FileSyncWorker : public Napi::AsyncWorker {
public:
    FileSyncWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    XrdCl::XRootDStatus status_;
};

// 7. 截断文件 (Truncate)
class FileTruncateWorker : public Napi::AsyncWorker {
public:
    FileTruncateWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file, uint64_t size);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    uint64_t size_;
    XrdCl::XRootDStatus status_;
};

// 8. 向量/多块读取 (VectorRead / ReadChunks)
class FileVectorReadWorker : public Napi::AsyncWorker {
public:
    FileVectorReadWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                         XrdCl::File* file, const XrdCl::ChunkList& chunks);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    XrdCl::ChunkList chunks_;
    XrdCl::VectorReadInfo* result_info_; // 包含多个 Buffer，需要在 OnOK 中转为 JS Array<Buffer>
    XrdCl::XRootDStatus status_;
};

// 9. 扩展属性处理 (复用一个 Worker 通过枚举区分操作)
class FileXAttrWorker : public Napi::AsyncWorker {
public:
    enum Action { GET, SET, DEL, LIST };
    FileXAttrWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::File* file, 
                    Action action, const std::string& name = "", const std::string& value = "");
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::File* file_;
    Action action_;
    std::string name_;
    std::string value_;
    std::string result_str_; // 存放 Get 或 List 的返回值
    XrdCl::XRootDStatus status_;
};

} // namespace Workers
} // namespace XrdNode