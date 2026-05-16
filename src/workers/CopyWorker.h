#pragma once
#include <napi.h>
#include <XrdCl/XrdClCopyProcess.hh>
#include <mutex>

namespace XrdNode {
namespace Workers {

// 继承底层的进度处理器，内部持有 ThreadSafeFunction 以安全呼叫 JS
class NodeCopyProgressHandler : public XrdCl::CopyProgressHandler {
public:
    // tsfn 由 XrdNodeCopyProcess::SetEventListener 传入的回调创建
    NodeCopyProgressHandler(Napi::ThreadSafeFunction tsfn);
    ~NodeCopyProgressHandler() override;

    void BeginJob(uint32_t jobNum, uint32_t jobTotal, const XrdCl::URL* source, const XrdCl::URL* target) override;
    void EndJob(uint32_t jobNum, const XrdCl::PropertyList* result) override;
    void JobProgress(uint32_t jobNum, uint64_t bytesProcessed, uint64_t bytesTotal) override;
    bool ShouldCancel(uint32_t jobNum) override;

    // 允许主线程修改取消状态
    void Cancel(uint32_t jobNum);

private:
    Napi::ThreadSafeFunction tsfn_;
    std::mutex cancel_mutex_;
    std::vector<uint32_t> canceled_jobs_;
};

// 包装 Run() 阻塞操作的 Worker
class CopyRunWorker : public Napi::AsyncWorker {
public:
    CopyRunWorker(Napi::Env env, Napi::Promise::Deferred deferred, 
                  XrdCl::CopyProcess* cp, NodeCopyProgressHandler* handler);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error& e) override;

private:
    Napi::Promise::Deferred deferred_;
    XrdCl::CopyProcess* cp_;
    NodeCopyProgressHandler* handler_;
    XrdCl::PropertyList* result_list_;
    XrdCl::XRootDStatus status_;
};

} // namespace Workers
} // namespace XrdNode