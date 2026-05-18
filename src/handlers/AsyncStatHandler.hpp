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
        0,                                                           // 最大队列大小 (0 表示无限制)
        1                                                            // 初始线程数计数
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
    tsfn_.BlockingCall(
        [this, status = *status, statInfo](Napi::Env env, Napi::Function jsCallback) {
          // 这个 Lambda 运行在 V8 主线程！可以安全操作 JS 对象了！

          if (status.IsOK() && statInfo) {
            auto result = Utils::StatInfoToObject(env, statInfo);
            // Napi::Object result = Napi::Object::New(env);

            // // 数字类型 (64位整数必须用 Napi::BigInt)
            // result.Set("id", Napi::String::New(env, statInfo->GetId()));
            // result.Set("size", Napi::BigInt::New(env, statInfo->GetSize()));

            // // 32位及以下的数字可以直接用 Napi::Number
            // result.Set("flags", Napi::Number::New(env, statInfo->GetFlags()));
            // result.Set("modTime", Napi::Number::New(env, statInfo->GetModTime()));
            // result.Set("accessTime", Napi::Number::New(env, statInfo->GetAccessTime()));
            // result.Set("changeTime", Napi::Number::New(env, statInfo->GetChangeTime()));

            // // 字符串类型
            // result.Set("modTimeAsString", Napi::String::New(env,
            // statInfo->GetModTimeAsString())); result.Set("accessTimeAsString",
            // Napi::String::New(env, statInfo->GetAccessTimeAsString()));
            // result.Set("changeTimeAsString", Napi::String::New(env,
            // statInfo->GetChangeTimeAsString()));

            // result.Set("modeAsString", Napi::String::New(env, statInfo->GetModeAsString()));
            // result.Set("modeAsOctString", Napi::String::New(env,
            // statInfo->GetModeAsOctString()));

            // result.Set("owner", Napi::String::New(env, statInfo->GetOwner()));
            // result.Set("group", Napi::String::New(env, statInfo->GetGroup()));
            // result.Set("checksum", Napi::String::New(env, statInfo->GetChecksum()));

            this->deferred_.Resolve(result);
          } else {
            Napi::Error err = Utils::StatusToError(env, status);
            this->deferred_.Reject(err.Value());
          }

          delete statInfo;  // 清理 XRootD 返回的内存

          // 2. 极其重要：告诉 Node.js 这个线程安全函数已经用完了，可以减去活跃计数
          tsfn_.Release();

          // 3. 极其重要：XRootD 不会自动 delete Handler，必须自杀防泄漏！
          delete this;
        }
    );
  }

 private:
  Napi::Promise::Deferred deferred_;
  Napi::ThreadSafeFunction tsfn_;
};

}  // namespace XrdNode