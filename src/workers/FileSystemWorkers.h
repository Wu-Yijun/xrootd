#pragma once
#include <napi.h>
#include <XrdCl/XrdClFileSystem.hh>
#include <XrdCl/XrdClXRootDResponses.hh>
#include <vector>
#include <string>

namespace XrdNode {
namespace Workers {

// 1. 定位 (Locate / DeepLocate)
class FSLocateWorker : public Napi::AsyncWorker {
public:
    FSLocateWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                   XrdCl::FileSystem* fs, const std::string& path, uint16_t flags, bool deep = false);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::FileSystem* fs_;
    std::string path_;
    uint16_t flags_;
    bool deep_;
    XrdCl::LocationInfo* location_info_; // OnOK 中转换为 Napi::Object
    XrdCl::XRootDStatus status_;
};

// 2. 目录操作与基础文件管理 (复用类以减少模板代码)
class FSBasicOpWorker : public Napi::AsyncWorker {
public:
    enum Action { RM, RMDIR, MKDIR, PING, CHMOD, TRUNCATE };
    FSBasicOpWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                    XrdCl::FileSystem* fs, Action action, const std::string& path, 
                    uint16_t mode_or_flags = 0, uint64_t size = 0);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::FileSystem* fs_;
    Action action_;
    std::string path_;
    uint16_t mode_or_flags_;
    uint64_t size_;
    XrdCl::XRootDStatus status_;
};

// 3. 移动文件 (Mv)
class FSMvWorker : public Napi::AsyncWorker {
public:
    FSMvWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
               XrdCl::FileSystem* fs, const std::string& source, const std::string& dest);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::FileSystem* fs_;
    std::string source_;
    std::string dest_;
    XrdCl::XRootDStatus status_;
};

// 4. 目录列表 (DirList)
class FSDirListWorker : public Napi::AsyncWorker {
public:
    FSDirListWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                    XrdCl::FileSystem* fs, const std::string& path, uint16_t flags);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::FileSystem* fs_;
    std::string path_;
    uint16_t flags_;
    XrdCl::DirectoryList* dir_list_; // 存放结果，在 OnOK 中转换为 JS Array<Object>
    XrdCl::XRootDStatus status_;
};

// 5. Stat 与 StatVFS
class FSStatWorker : public Napi::AsyncWorker {
public:
    FSStatWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                 XrdCl::FileSystem* fs, const std::string& path, bool is_vfs = false);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::FileSystem* fs_;
    std::string path_;
    bool is_vfs_;
    XrdCl::StatInfo* stat_info_;
    XrdCl::StatInfoVFS* stat_vfs_info_;
    XrdCl::XRootDStatus status_;
};

// 6. Query 请求
class FSQueryWorker : public Napi::AsyncWorker {
public:
    FSQueryWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                  XrdCl::FileSystem* fs, uint16_t query_code, const std::string& args);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::FileSystem* fs_;
    uint16_t query_code_;
    std::string args_;
    XrdCl::Buffer* result_buffer_; // OnOK 转换为 JS String
    XrdCl::XRootDStatus status_;
};

// 7. 辅助方法扩展: 整个文件读取 (Cat)
class FSCatWorker : public Napi::AsyncWorker {
public:
    FSCatWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                XrdCl::FileSystem* fs, const std::string& path);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::FileSystem* fs_;
    std::string path_;
    XrdCl::Buffer* result_buffer_; // FileSystem::Cat 内部并不一定是 XRootD 标准接口，可能需要封装
    XrdCl::XRootDStatus status_;
};

// 8. FileSystem 层级的扩展属性 (XAttr)
class FSXAttrWorker : public Napi::AsyncWorker {
public:
    enum Action { GET, SET, DEL, LIST };
    FSXAttrWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::FileSystem* fs, 
                  Action action, const std::string& path, const std::string& name = "", const std::string& value = "");
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;
private:
    Napi::Promise::Deferred deferred_;
    XrdCl::FileSystem* fs_;
    Action action_;
    std::string path_;
    std::string name_;
    std::string value_;
    std::string result_str_; 
    XrdCl::XRootDStatus status_;
};

} // namespace Workers
} // namespace XrdNode