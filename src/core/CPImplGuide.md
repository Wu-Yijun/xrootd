这是一个非常精彩且具有挑战性的分析任务！

如果你仔细对比 `XrdClCopyProcess.hh` 与之前的 `XrdClFile.hh` 或 `XrdClFileSystem.hh`，你会发现一个**极其重大的架构差异**：

之前的 `File` 操作都是通过传递 `ResponseHandler*` 来实现纯底层的异步网络回调；而 `CopyProcess` 的 `Prepare()` 和 `Run()` **没有 `ResponseHandler` 参数，且返回值直接是 `XRootDStatus**`。
这意味着：**在 XRootD 的设计中，`CopyProcess::Run()` 是一个会阻塞当前线程的同步执行方法！** 因此，我们之前的“抛弃 AsyncWorker，纯用 XRootD 异步后台线程”的策略在这里行不通了。我们必须**请回 `Napi::AsyncWorker**`，配合 `Napi::ThreadSafeFunction` (TSFN)，来构建一个混合架构。

以下是 `XrdNodeCopyProcess` 的全盘设计与实现指导。

---

### 一、 宏观架构与 TS 接口设计 (Thick TS)

在 TS 层，`CopyProcess` 应该表现为一个可以添加任务、支持进度回调，并最终返回所有任务结果的类。我们还可以顺手实现**任务取消 (Abort)** 功能。

```typescript
// lib/copy.ts
export interface CopyJobConfig {
    source: string;
    target: string;
    force?: boolean;
    makeDir?: boolean;
    chunkSize?: number;
    parallelChunks?: number;
    // ... 其他属性
}

export interface CopyJobResult {
    size: number;
    sourceCheckSum?: string;
    targetCheckSum?: string;
    realTarget?: string;
}

export type ProgressCallback = (jobNum: number, processed: number, total: number) => void;

export class CopyProcess {
    // 同步配置任务
    addJob(config: CopyJobConfig): void;
    // 异步准备
    async prepare(): Promise<void>;
    // 异步执行，支持进度回调
    async run(onProgress?: ProgressCallback): Promise<CopyJobResult[]>;
    // 中止拷贝
    abort(): void; 
}

```

---

### 二、 C++ 类的核心结构与内存管理

**核心痛点：** `AddJob` 接收的 `results` 参数是一个指针，XRootD 会在 `Run()` 结束时向这块内存写入结果。因此，**C++ 实例必须负责分配并持有所有 jobs 的 `results` 内存，直到它们被转换为 JS 对象后才能释放。**

```cpp
// src/core/XrdNodeCopyProcess.h
#pragma once
#include <napi.h>
#include <XrdCl/XrdClCopyProcess.hh>
#include <vector>
#include <atomic>

namespace XrdNode {

class XrdNodeCopyProcess : public Napi::ObjectWrap<XrdNodeCopyProcess> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    XrdNodeCopyProcess(const Napi::CallbackInfo& info);
    ~XrdNodeCopyProcess();

private:
    Napi::Value AddJob(const Napi::CallbackInfo& info);
    Napi::Value Prepare(const Napi::CallbackInfo& info);
    Napi::Value Run(const Napi::CallbackInfo& info);
    Napi::Value Abort(const Napi::CallbackInfo& info);

    XrdCl::CopyProcess* copyProcess_;
    
    // 内存管理：持有所有任务的返回结果指针，析构时统一释放
    std::vector<XrdCl::PropertyList*> jobResults_;
    
    // 安全机制：用于响应 ShouldCancel 接口
    std::atomic<bool> isCancelled_{false};
};

} // namespace XrdNode

```

---

### 三、 各个接口的 Step-by-Step 实现

#### 1. `AddJob` (同步执行，专注参数转换与内存分配)

这是一个纯内存操作配置，不会阻塞，因此**直接在主线程同步执行**。

* **实现步骤：**
1. 提取 JS 传入的 Object (`config`)。
2. 实例化一个 `XrdCl::PropertyList`。
3. 遍历 JS Object，根据值的类型（String, Number, Boolean），调用 `PropertyList::Set()`。
4. **内存管理关键：** `new XrdCl::PropertyList()` 作为 `results` 容器，将其推入 `jobResults_` 数组妥善保管。
5. 调用 `copyProcess_->AddJob(props, resultProps)`。



```cpp
Napi::Value XrdNodeCopyProcess::AddJob(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected job config object").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object config = info[0].As<Napi::Object>();
    XrdCl::PropertyList props;
    
    // 示例：提取 string (实际工程中应当遍历 keys 并动态转换)
    if (config.Has("source")) props.Set("source", config.Get("source").As<Napi::String>().Utf8Value());
    if (config.Has("target")) props.Set("target", config.Get("target").As<Napi::String>().Utf8Value());
    if (config.Has("force")) props.Set("force", config.Get("force").As<Napi::Boolean>().Value());
    
    // 内存分配：创建用于存放结果的容器
    XrdCl::PropertyList* resultProps = new XrdCl::PropertyList();
    this->jobResults_.push_back(resultProps); // 纳入生命周期管理

    XrdCl::XRootDStatus status = this->copyProcess_->AddJob(props, resultProps);
    if (!status.IsOK()) {
        Napi::Error::New(env, status.ToString()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

```

#### 2. `Prepare` (异步执行，使用 AsyncWorker)

`Prepare` 可能会进行域名解析和集群探测，必须放到后台。

* **实现步骤：**
1. 创建一个简单的 `Napi::AsyncWorker`。
2. 在 `Execute` (跑在 libuv 线程) 中调用 `copyProcess_->Prepare()`。
3. 在 `OnOK` / `OnError` (跳回主线程) 中 Resolve/Reject Promise。



#### 3. 终极挑战：`Run` 与 `CopyProgressHandler`

这是整个模块最复杂的函数。因为我们需要：

1. 在 libuv 线程中**阻塞**调用 `copyProcess_->Run()`。
2. 在 `CopyProgressHandler` 的 C++ 回调中，通过 N-API 的 **ThreadSafeFunction (TSFN)** 突破线程壁垒，将进度事件发给 V8 主线程。
3. `Run` 结束后，提取 `jobResults_` 返回给 JS。

**A. 定义进度处理器 (内部类)**

```cpp
class NodeProgressHandler : public XrdCl::CopyProgressHandler {
public:
    NodeProgressHandler(Napi::ThreadSafeFunction tsfn, std::atomic<bool>& cancelFlag) 
        : tsfn_(tsfn), cancelFlag_(cancelFlag) {}

    // 突破线程限制发送进度
    virtual void JobProgress(uint32_t jobNum, uint64_t bytesProcessed, uint64_t bytesTotal) override {
        // 封存数据，按值捕获
        tsfn_.BlockingCall([jobNum, bytesProcessed, bytesTotal](Napi::Env env, Napi::Function jsCallback) {
            if (!jsCallback.IsEmpty()) {
                // 回调 JS: onProgress(jobNum, processed, total)
                jsCallback.Call({
                    Napi::Number::New(env, jobNum),
                    Napi::Number::New(env, bytesProcessed),
                    Napi::Number::New(env, bytesTotal)
                });
            }
        });
    }

    // 巧妙利用 shared atomic 实现优雅中断！
    virtual bool ShouldCancel(uint32_t jobNum) override {
        return cancelFlag_.load();
    }

private:
    Napi::ThreadSafeFunction tsfn_;
    std::atomic<bool>& cancelFlag_;
};

```

**B. 组装 RunWorker**

```cpp
class CopyRunWorker : public Napi::AsyncWorker {
public:
    CopyRunWorker(Napi::Env env, Napi::Promise::Deferred deferred, XrdCl::CopyProcess* cp, 
                  std::vector<XrdCl::PropertyList*>& results, std::atomic<bool>& cancelFlag, Napi::Function progressCb)
        : Napi::AsyncWorker(env, "CopyRunWorker"), deferred_(deferred), cp_(cp), results_(results) {
        
        // 如果用户传了进度回调，就初始化 TSFN
        if (!progressCb.IsEmpty()) {
            tsfn_ = Napi::ThreadSafeFunction::New(env, progressCb, "XrdCopyProgress", 0, 1);
        }
        // 实例化 XRootD 的 handler
        handler_ = new NodeProgressHandler(tsfn_, cancelFlag);
    }

    ~CopyRunWorker() {
        if (tsfn_) tsfn_.Release();
        delete handler_;
    }

    // 运行在后台 libuv 线程！绝对不能碰 JS 对象！
    void Execute() override {
        // 阻塞执行，期间 handler_ 会不断通过 tsfn_ 呼叫主线程
        status_ = cp_->Run(handler_);
        if (!status_.IsOK()) {
            SetError(status_.ToString());
        }
    }

    // 运行在主线程！
    void OnOK() override {
        Napi::Env env = Env();
        Napi::Array jsResults = Napi::Array::New(env, results_.size());

        // 提取 C++ 内存中的结果，转化为 JS 数组
        for (size_t i = 0; i < results_.size(); i++) {
            Napi::Object resObj = Napi::Object::New(env);
            XrdCl::PropertyList* p = results_[i];
            
            uint64_t size = 0;
            if (p->Get("size", size)) resObj.Set("size", Napi::BigInt::New(env, size));
            
            std::string srcCksm;
            if (p->Get("sourceCheckSum", srcCksm)) resObj.Set("sourceCheckSum", Napi::String::New(env, srcCksm));
            
            // ... 提取 targetCheckSum, realTarget 等
            jsResults.Set(i, resObj);
        }

        deferred_.Resolve(jsResults);
    }

    void OnError(const Napi::Error& e) override {
        deferred_.Reject(e.Value());
    }

private:
    Napi::Promise::Deferred deferred_;
    XrdCl::CopyProcess* cp_;
    std::vector<XrdCl::PropertyList*>& results_;
    XrdCl::XRootDStatus status_;
    NodeProgressHandler* handler_;
    Napi::ThreadSafeFunction tsfn_;
};

```

---

### 四、 极致的内存与安全保障

#### 1. 析构防漏 (The Destructor)

在 `AddJob` 中，我们一直往 `jobResults_` 塞裸指针。无论 `Run` 是成功、失败还是抛出异常，或者用户只 `AddJob` 没 `Run` 就销毁了对象，**析构函数是最终的垃圾站**：

```cpp
XrdNodeCopyProcess::~XrdNodeCopyProcess() {
    if (this->copyProcess_) {
        delete this->copyProcess_;
    }
    // 释放所有存放结果的 PropertyList
    for (auto* prop : this->jobResults_) {
        delete prop;
    }
    this->jobResults_.clear();
}

```

#### 2. 优雅的 Abort 机制 (The Safe Cancellation)

还记得我们在 `XrdNodeCopyProcess` 定义的 `std::atomic<bool> isCancelled_` 吗？
如果用户在 JS 里调用 `process.abort()`：

```cpp
Napi::Value XrdNodeCopyProcess::Abort(const Napi::CallbackInfo& info) {
    // 仅仅改变一个原子变量，完全线程安全
    this->isCancelled_.store(true);
    return info.Env().Undefined();
}

```

此时，后台正在疯狂 `Run` 的 XRootD 引擎每拷贝一块数据，就会调用一次 `ShouldCancel()`。它读取到 `true` 后，会自动停止拷贝，并在 `Run` 的返回值中抛出中止错误，一切收尾工作完美流转回 `OnError` 并触发 Promise Reject。

### 总结

这套 `CopyProcess` 架构展示了 N-API 开发的高级形态：**利用 `AsyncWorker` 封装阻塞调用，利用 `ThreadSafeFunction` 建立双向进度通信，利用 `std::atomic` 实现跨线程优雅中断。** 它的稳健程度将足以支撑 LHAASO PB 级别数据的稳定传输！