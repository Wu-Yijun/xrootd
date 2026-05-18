这是一个极其浩大且精密的工程规划。基于 `XrdClFileSystem.hh` 的头文件设计，结合我们之前敲定的 **Native Async (ThreadSafeFunction)** 架构，我们需要对所有的接口进行分类、抽象，并制定严格的内存和回调规范。

为了实现 C++ 层的“极简与复用（Thin C++）”，我们**绝对不能**为每个方法写一个独立的 Handler。根据底层 `HandleResponse` 中 `XrdCl::AnyObject *response` 携带的数据类型，我们可以将所有方法划分为 **4 大类 Handler** 和 **1 类同步操作**。

以下是 `XrdNodeFileSystem.cpp` 接口与 Handler 的完整实现蓝图（Blueprint）：

---

### ⚠️ 全局内存与生命周期铁律 (The Iron Rules)

在编写以下任何一个 Handler 时，必须严格遵守以下三点，否则极易导致 Node 进程崩溃或内存泄漏：

1. **状态拷贝法则：** 在 `BlockingCall` 的 Lambda 捕获列表中，必须**按值深拷贝**状态对象：`statusCopy = *status`。
2. **Payload 释放法则：** 如果 `status->IsOK()` 为真且 `response` 不为空，必须通过 `response->Get(ptr)` 取出数据，**在 Lambda 将其转化为 JS 对象后，必须手动 `delete ptr;**`（XRootD 引擎把这块内存的生命周期交给了你）。
3. **Handler 自毁法则：** 在 Lambda 的最后，必须调用 `tsfn_.Release();` 和 `delete this;`。

---

### 第一类：通用控制流 Handler (`FSControlHandler`)

**适用接口：** `Rm`, `MkDir`, `RmDir`, `ChMod`, `Mv`, `Truncate`, `Ping`

* **特征：** 这类操作的共性是：成功时不返回任何数据（`response` 通常为 nullptr 或没有意义），失败时返回错误码。
* **JS 期望返回：** `Promise<void>` (成功则 Resolve `undefined`)。
* **实现指导：**
* 复用我们上一节写的 `FileControlHandler`。
* 对于 `MkDir` 和 `ChMod`，需要将 JS 传进来的 `Number` 强制转换为底层的 `XrdCl::MkDirFlags::Flags` 和 `XrdCl::Access::Mode`。
* **额外注意：** `Ping` 方法不需要传 `path`，只传 `handler`。



---

### 第二类：字符串/二进制缓冲 Handler (`FSBufferHandler`)

**适用接口：** `Query`, `Prepare`, `SendInfo`

* **特征：** 底层的 `response->Get(XrdCl::Buffer* ptr)` 会取出一个 `XrdCl::Buffer` 对象。
* **JS 期望返回：** `Promise<string>` 或 `Promise<Buffer>`。对于 `Query` 和 `Prepare`，返回字符串或 JSON 更加 Node Native。
* **实现指导：**
```cpp
// 在 HandleResponse 中
XrdCl::Buffer* xrdBuffer = nullptr;
if (isOk && response) { response->Get(xrdBuffer); }

tsfn_.BlockingCall([..., xrdBuffer](Napi::Env env, ...) {
    if (isOk) {
        if (xrdBuffer) {
            // 注意：XRootD Buffer 不保证有 \0 结尾，必须使用 GetSize()
            std::string str(xrdBuffer->GetBuffer(), xrdBuffer->GetSize());
            deferred_.Resolve(Napi::String::New(env, str));
            delete xrdBuffer; // 必须释放！
        } else {
            deferred_.Resolve(env.Null());
        }
    } else {
        deferred_.Reject(Utils::StatusToError(env, statusCopy).Value());
    }
    // ... release & delete this
});

```


* **额外注意：** `Prepare` 接口在 JS 层接收的是 `string[]`。在 C++ 接口入口处，你需要遍历 `Napi::Array`，将其转化为 `std::vector<std::string>`，然后再传给 `fs_->Prepare(...)`。这个 vector 作为局部变量，在发起异步请求后会被 XRootD 引擎拷贝或接管。

---

### 第三类：复杂结构体定制 Handlers (Must Be Independent)

由于返回的数据结构差异巨大，以下每个接口需要一个**独立的 Handler 类**。

#### 1. `FSStatHandler` (对应 `Stat`)

* **处理逻辑：** `response->Get(XrdCl::StatInfo* statInfo);`
* **返回组装：** 参考我们之前讨论的，将 `size`、`id` 映射为 `Napi::BigInt`，时间戳映射为 `Napi::Number`，并补充上各种 `AsString` 属性。
* **清理：** `delete statInfo;`

#### 2. `FSStatVFSHandler` (对应 `StatVFS`)

* **处理逻辑：** `response->Get(XrdCl::StatInfoVFS* vfsInfo);`
* **返回组装：** 这是虚拟文件系统状态。你需要将其包装成类似 Node.js `fs.statfs` 的对象，包含 `f_bsize` (块大小), `f_blocks` (总块数), `f_bfree` (可用块数) 等，方便前端做容量监控。
* **清理：** `delete vfsInfo;`

#### 3. `FSDirListHandler` (对应 `DirList`)

* **处理逻辑：** `response->Get(XrdCl::DirectoryList* dirList);`
* **返回组装：**
* JS 层期望得到 `Promise<Array<StatInfo | string>>`（根据 flag 是否要求带 stat）。
* 在 Lambda 中，创建一个 `Napi::Array`。
* 遍历 `dirList->GetSize()`，通过 `dirList->At(i)` 获取每个 `XrdCl::DirectoryList::Entry*`。
* 提取 `entry->GetName()` 放入数组。


* **清理：** `delete dirList;`

#### 4. `FSLocateHandler` (对应 `Locate` / `DeepLocate`)

* **处理逻辑：** `response->Get(XrdCl::LocationInfo* locInfo);` (注意区分，返回的是封装对象而不是直接的 `std::vector`)。
* **返回组装：**
* 返回 `Promise<Array<Location>>`。
* 遍历 `locInfo->GetSize()`，通过 `locInfo->At(i)` 获取每个 `XrdCl::LocationInfo::Location`。
* 将每个节点的地址 (`GetAddress()`)、类型 (`GetType()`) 等包装成 JavaScript 对象。


* **清理：** `delete locInfo;`

---

### 第四类：扩展属性 Handlers (`XAttr`)

XRootD 的 `XAttr`（Extended Attributes）接口设计比较特殊，它分两种返回：

#### 1. `FSXAttrStatusHandler` (对应 `SetXAttr`, `DelXAttr`)

* **处理逻辑：** `response->Get(std::vector<XrdCl::XAttrStatus>* statusList);`
* **返回：** 这些操作是批量执行的。你需要返回一个 JS 数组，告诉用户哪个属性成功了，哪个失败了。
* **JS 层封装注意：** 传入的参数是对象 `{ "user.tag": "value" }`，C++ 层需要先解析并构建 `std::vector<xattr_t>`，然后再发请求。

#### 2. `FSXAttrDataHandler` (对应 `GetXAttr`, `ListXAttr`)

* **处理逻辑：** `response->Get(std::vector<XrdCl::XAttr>* attrList);`
* **返回：** 将其重新组装为 JS 的 Record 对象 `{ [key: string]: string }` Resolve 掉。

---

### 第五类：例外 —— 纯同步属性操作

**适用接口：** `GetProperty`, `SetProperty`

头文件中的签名是：

```cpp
bool SetProperty( const std::string &name, const std::string &value );
bool GetProperty( const std::string &name, std::string &value ) const;

```

* **特征：** 它们**没有** `ResponseHandler` 参数！它们是纯粹的本地客户端内存配置项。
* **实现指导：** **绝对不要用 Worker 或 Handler**。直接在主线程同步执行，并立刻返回结果。

```cpp
Napi::Value XrdNodeFileSystem::SetProperty(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string name = info[0].As<Napi::String>().Utf8Value();
    std::string value = info[1].As<Napi::String>().Utf8Value();

    bool success = this->fs_->SetProperty(name, value);
    return Napi::Boolean::New(env, success);
}

```

---

### 架构总结与落地建议

1. **文件结构划分：**
* 不要把所有代码堆在 `XrdNodeFileSystem.cpp` 里。
* 创建 `handlers/FSControlHandler.h`
* 创建 `handlers/FSBufferHandler.h`
* 创建 `handlers/FSComplexHandlers.h` (里面装 Stat, DirList, Locate)
这会让你的工程极其干净。


2. **错误处理归一：**
在所有的 Handler 里面，`else` 分支统一调用：
`this->deferred_.Reject(Utils::StatusToError(env, statusCopy).Value());`
不要做任何针对特定 Handler 的错误魔改，把映射任务全部留给 `lib/error.ts`。
3. **从哪里动手？**
建议你先实现 `DirList`。因为 LHAASO 的分析脚本第一步通常是扫描目录结构。当你能通过 `const files = await fs.dirList('/eos/lhaaso')` 拿到一个庞大的文件名数组时，这个文件系统模块就算彻底站住脚了。