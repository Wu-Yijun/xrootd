// src/handlers/AsyncStatHandler.hpp
#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>

#include "utils/type_conversions.h"

namespace XrdNode {

#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>

/**
 * @brief 文件元数据查询 (Stat) 操作的异步回调处理器 (Handler)。
 * @details 专用于获取单个文件或目录的详细属性及校验和等元数据信息。
 *
 * - **接受内容**：Napi::Env 环境句柄、Napi::Promise::Deferred Promise 延迟对象。
 * - **内部处理**：从底层的 AnyObject 中提取出 XrdCl::StatInfo 指针，通过 TSFN 跳回 V8
 * 主线程后，对底层的 64 位整数字段采用 Napi::BigInt 包装，其他常规数字与字符串采用对应 JS
 * 类型包装，随后释放 StatInfo、释放 TSFN 并 delete this 自销毁。
 * - **返回结果**：向 JS Promise 成功 Resolve 包含 id/size/flags/modTime 等丰富元数据属性的 JS
 * Object，失败 Reject `Napi::Error`。
 */
class AsyncStatHandler : public XrdCl::ResponseHandler {
 public:
  AsyncStatHandler(Napi::Env env, Napi::Promise::Deferred deferred) : deferred_(deferred) {
    // 2. 创建 ThreadSafeFunction (TSFN)
    // 它的作用是允许 XRootD 的后台线程安全地呼叫 V8 主线程
    tsfn_ = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),  // Dummy JS 函数
        "XRootD_AsyncStat",                                          // 名称
        0,  // 最大队列大小 (0 表示无限制)
        1   // 初始线程数计数
    );
  }

  // 3. 析构函数必须释放 TSFN
  virtual ~AsyncStatHandler() { tsfn_.Release(); }

  // 4. XRootD 网络响应到达时触发 (运行在 XRootD 后台线程！)
  virtual void HandleResponse(XrdCl::XRootDStatus* status, XrdCl::AnyObject* response) override {
    // 警告：这里绝对不能碰任何 Napi::Value，因为这是后台线程！

    // 解析数据并存入局部结构
    XrdCl::StatInfo* statInfo = nullptr;
    if (status->IsOK()) {
      response->Get(statInfo);
    }

    // 5. 使用 TSFN 将数据发送回 JS 主线程
    // BlockingCall 会把 lambda 表达式放入 V8 主线程的事件队列
    napi_status callStatus =tsfn_.BlockingCall(
        [this, status = *status, statInfo](Napi::Env env, Napi::Function jsCallback) {
          // 这个 Lambda 运行在 V8 主线程！可以安全操作 JS 对象了！

          if (status.IsOK() && statInfo) {
            auto result = Utils::StatInfoToObject(env, statInfo);

            this->deferred_.Resolve(result);
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          // 4. 【核心修复】只需 delete this。它会自动触发析构函数执行 tsfn_.Release()
          delete this; // [FIXED]
        }
    );

    // 5. 【核心修复】如果 Node.js 事件队列拒绝接收（比如进程正在关闭）
    // 此时 Lambda 永远不会执行，我们必须在这里原地清理，防止内存泄漏！
    if (callStatus != napi_ok) { // [FIXED]
      delete statInfo;
      delete this;
    }
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
};

}  // namespace XrdNode