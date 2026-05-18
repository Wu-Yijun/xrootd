#pragma once
#include <XrdCl/XrdClFileSystem.hh>
#include <XrdCl/XrdClXRootDResponses.hh>
#include <napi.h>
#include <string>
#include <vector>


namespace XrdNode {
namespace Workers {

#include <XrdCl/XrdClFileSystem.hh>
#include <napi.h>


// 1. 继承 XRootD 的响应处理器
class AsyncStatHandler : public XrdCl::ResponseHandler {
public:
  AsyncStatHandler(Napi::Env env, Napi::Promise::Deferred deferred)
      : deferred_(deferred) {

    // 2. 创建 ThreadSafeFunction (TSFN)
    // 它的作用是允许 XRootD 的后台线程安全地呼叫 V8 主线程
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env,
                            [](const Napi::CallbackInfo &) {}), // Dummy JS 函数
        "XRootD_AsyncStat",                                     // 名称
        0, // 最大队列大小 (0 表示无限制)
        1  // 初始线程数计数
    );
  }

  // 3. 析构函数必须释放 TSFN
  virtual ~AsyncStatHandler() { tsfn_.Release(); }

  // 4. XRootD 网络响应到达时触发 (运行在 XRootD 后台线程！)
  virtual void HandleResponse(XrdCl::XRootDStatus *status,
                              XrdCl::AnyObject *response) override {

    // 警告：这里绝对不能碰任何 Napi::Value，因为这是后台线程！

    // 解析数据并存入局部结构
    XrdCl::StatInfo *statInfo = nullptr;
    if (status->IsOK()) {
      response->Get(statInfo);
    }

    // 5. 使用 TSFN 将数据发送回 JS 主线程
    // BlockingCall 会把 lambda 表达式放入 V8 主线程的事件队列
    tsfn_.BlockingCall([this, statusStr = status->ToString(),
                        isOk = status->IsOK(),
                        statInfo](Napi::Env env, Napi::Function jsCallback) {
      // 这个 Lambda 运行在 V8 主线程！可以安全操作 JS 对象了！

      if (isOk && statInfo) {
        Napi::Object result = Napi::Object::New(env);

        // 数字类型 (64位整数必须用 Napi::BigInt)
        result.Set("id", Napi::String::New(env, statInfo->GetId()));
        result.Set("size", Napi::BigInt::New(env, statInfo->GetSize()));

        // 32位及以下的数字可以直接用 Napi::Number
        result.Set("flags", Napi::Number::New(env, statInfo->GetFlags()));
        result.Set("modTime", Napi::Number::New(env, statInfo->GetModTime()));
        result.Set("accessTime",
                   Napi::Number::New(env, statInfo->GetAccessTime()));
        result.Set("changeTime",
                   Napi::Number::New(env, statInfo->GetChangeTime()));

        // 字符串类型
        result.Set("modTimeAsString",
                   Napi::String::New(env, statInfo->GetModTimeAsString()));
        result.Set("accessTimeAsString",
                   Napi::String::New(env, statInfo->GetAccessTimeAsString()));
        result.Set("changeTimeAsString",
                   Napi::String::New(env, statInfo->GetChangeTimeAsString()));

        result.Set("modeAsString",
                   Napi::String::New(env, statInfo->GetModeAsString()));
        result.Set("modeAsOctString",
                   Napi::String::New(env, statInfo->GetModeAsOctString()));

        result.Set("owner", Napi::String::New(env, statInfo->GetOwner()));
        result.Set("group", Napi::String::New(env, statInfo->GetGroup()));
        result.Set("checksum", Napi::String::New(env, statInfo->GetChecksum()));

        this->deferred_.Resolve(result);
      } else {
        this->deferred_.Reject(Napi::Error::New(env, statusStr).Value());
      }

      delete statInfo; // 清理 XRootD 返回的内存

      // 2. 极其重要：告诉 Node.js 这个线程安全函数已经用完了，可以减去活跃计数
      tsfn_.Release();

      // 3. 极其重要：XRootD 不会自动 delete Handler，必须自杀防泄漏！
      delete this;
    });
  }

private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
};

// // 1. 定位 (Locate / DeepLocate)
// class FSLocateWorker : public Napi::AsyncWorker {
// public:
//     FSLocateWorker(Napi::Env env, Napi::Promise::Deferred deferred,
//                    XrdCl::FileSystem* fs, const std::string& path, uint16_t
//                    flags, bool deep = false);
//     void Execute() override;
//     void OnOK() override;
//     void OnError(const Napi::Error& e) override;
// private:
//     Napi::Promise::Deferred deferred_;
//     XrdCl::FileSystem* fs_;
//     std::string path_;
//     uint16_t flags_;
//     bool deep_;
//     XrdCl::LocationInfo* location_info_; // OnOK 中转换为 Napi::Object
//     XrdCl::XRootDStatus status_;
// };

// // 2. 目录操作与基础文件管理 (复用类以减少模板代码)
// class FSBasicOpWorker : public Napi::AsyncWorker {
// public:
//     enum Action { RM, RMDIR, MKDIR, PING, CHMOD, TRUNCATE };
//     FSBasicOpWorker(Napi::Env env, Napi::Promise::Deferred deferred,
//                     XrdCl::FileSystem* fs, Action action, const std::string&
//                     path, uint16_t mode_or_flags = 0, uint64_t size = 0);
//     void Execute() override;
//     void OnOK() override;
//     void OnError(const Napi::Error& e) override;
// private:
//     Napi::Promise::Deferred deferred_;
//     XrdCl::FileSystem* fs_;
//     Action action_;
//     std::string path_;
//     uint16_t mode_or_flags_;
//     uint64_t size_;
//     XrdCl::XRootDStatus status_;
// };

// // 3. 移动文件 (Mv)
// class FSMvWorker : public Napi::AsyncWorker {
// public:
//     FSMvWorker(Napi::Env env, Napi::Promise::Deferred deferred,
//                XrdCl::FileSystem* fs, const std::string& source, const
//                std::string& dest);
//     void Execute() override;
//     void OnOK() override;
//     void OnError(const Napi::Error& e) override;
// private:
//     Napi::Promise::Deferred deferred_;
//     XrdCl::FileSystem* fs_;
//     std::string source_;
//     std::string dest_;
//     XrdCl::XRootDStatus status_;
// };

// // 4. 目录列表 (DirList)
// class FSDirListWorker : public Napi::AsyncWorker {
// public:
//     FSDirListWorker(Napi::Env env, Napi::Promise::Deferred deferred,
//                     XrdCl::FileSystem* fs, const std::string& path, uint16_t
//                     flags);
//     void Execute() override;
//     void OnOK() override;
//     void OnError(const Napi::Error& e) override;
// private:
//     Napi::Promise::Deferred deferred_;
//     XrdCl::FileSystem* fs_;
//     std::string path_;
//     uint16_t flags_;
//     XrdCl::DirectoryList* dir_list_; // 存放结果，在 OnOK 中转换为 JS
//     Array<Object> XrdCl::XRootDStatus status_;
// };

// // 5. Stat 与 StatVFS
// class FSStatWorker : public Napi::AsyncWorker {
// public:
//     FSStatWorker(Napi::Env env, Napi::Promise::Deferred deferred,
//                  XrdCl::FileSystem* fs, const std::string& path, bool is_vfs
//                  = false);
//     void Execute() override;
//     void OnOK() override;
//     void OnError(const Napi::Error& e) override;
// private:
//     Napi::Promise::Deferred deferred_;
//     XrdCl::FileSystem* fs_;
//     std::string path_;
//     bool is_vfs_;
//     XrdCl::StatInfo* stat_info_;
//     XrdCl::StatInfoVFS* stat_vfs_info_;
//     XrdCl::XRootDStatus status_;
// };

// // 6. Query 请求
// class FSQueryWorker : public Napi::AsyncWorker {
// public:
//     FSQueryWorker(Napi::Env env, Napi::Promise::Deferred deferred,
//                   XrdCl::FileSystem* fs, uint16_t query_code, const
//                   std::string& args);
//     void Execute() override;
//     void OnOK() override;
//     void OnError(const Napi::Error& e) override;
// private:
//     Napi::Promise::Deferred deferred_;
//     XrdCl::FileSystem* fs_;
//     uint16_t query_code_;
//     std::string args_;
//     XrdCl::Buffer* result_buffer_; // OnOK 转换为 JS String
//     XrdCl::XRootDStatus status_;
// };

// // 7. 辅助方法扩展: 整个文件读取 (Cat)
// class FSCatWorker : public Napi::AsyncWorker {
// public:
//     FSCatWorker(Napi::Env env, Napi::Promise::Deferred deferred,
//                 XrdCl::FileSystem* fs, const std::string& path);
//     void Execute() override;
//     void OnOK() override;
//     void OnError(const Napi::Error& e) override;
// private:
//     Napi::Promise::Deferred deferred_;
//     XrdCl::FileSystem* fs_;
//     std::string path_;
//     XrdCl::Buffer* result_buffer_; // FileSystem::Cat 内部并不一定是 XRootD
//     标准接口，可能需要封装 XrdCl::XRootDStatus status_;
// };

// // 8. FileSystem 层级的扩展属性 (XAttr)
// class FSXAttrWorker : public Napi::AsyncWorker {
// public:
//     enum Action { GET, SET, DEL, LIST };
//     FSXAttrWorker(Napi::Env env, Napi::Promise::Deferred deferred,
//     XrdCl::FileSystem* fs,
//                   Action action, const std::string& path, const std::string&
//                   name = "", const std::string& value = "");
//     void Execute() override;
//     void OnOK() override;
//     void OnError(const Napi::Error& e) override;
// private:
//     Napi::Promise::Deferred deferred_;
//     XrdCl::FileSystem* fs_;
//     Action action_;
//     std::string path_;
//     std::string name_;
//     std::string value_;
//     std::string result_str_;
//     XrdCl::XRootDStatus status_;
// };

} // namespace Workers
} // namespace XrdNode