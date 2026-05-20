### 二、 超时参数 (Timeout) 的优雅管理方案

根据“Thin C++, Thick TS”原则，我们采取以下策略：

#### 1. C++ 层的处理：彻底无视，保持极简

在 N-API 绑定的 C++ 代码中，我们**不要**在入参中解析 timeout。调用底层 API 时，一律省略该参数（使用默认的 0）。让 C++ 层专注于纯粹的 I/O 执行。

#### 2. TS 层的处理：全局配置 + 可选的 Options 对象

Node.js 开发者习惯的 API 风格是 `fs.readFile(path, [options])` 或者使用 `AbortSignal`。我们在 TS 层实现两级超时控制：

**A. 初始化时的全局安全网 (Global Fallback)**
在你的 NPM 包的 `index.ts`（入口文件）中，默认将底层环境的超时时间修改为一个适合 Node.js 的值（例如 30 秒）。

```typescript
// 让默认超时从 30 分钟变成 30 秒，防止小白用户卡死
Env.putInt('RequestTimeout', 30);

```

**B. 接口层面的局部覆盖 (Per-Request Override)**
我们修改上一次设计的 `_run` 拦截器，引入类似于 `axios` 或 `fetch` 的 `options` 参数，通过 `Promise.race` 在 JS 层面实现精准超时拦截。

```typescript
// lib/types.ts
export interface XRootDOptions {
    timeout?: number; // 毫秒级别的超时控制
    // 未来还可以扩展 signal?: AbortSignal 等
}

// lib/file.ts
import { XRootDError } from './error';

export class File {
    // 带有超时控制的执行拦截器
    private async _run<T>(
        operation: () => Promise<T>, 
        syscall: string, 
        options?: XRootDOptions
    ): Promise<T> {
        // 核心：基于 Promise.race 的 JS 层超时控制
        const task = operation();
        
        if (options?.timeout && options.timeout > 0) {
            const timeoutPromise = new Promise<never>((_, reject) => {
                setTimeout(() => {
                    // 抛出标准的 Node.js 超时错误
                    const err: any = new Error(`[XRootD ${syscall}] Operation timed out after ${options.timeout}ms`);
                    err.code = 'ETIMEDOUT';
                    reject(err);
                }, options.timeout);
            });
            
            try {
                return await Promise.race([task, timeoutPromise]);
            } catch (err: any) {
                // 如果是超时，直接抛出；否则包装为 XRootDError
                if (err.code === 'ETIMEDOUT') throw err;
                throw new XRootDError(err, syscall);
            }
        }

        // 没有局部超时限制，直接等待（依赖底层的 30s 兜底）
        try {
            return await task;
        } catch (err: any) {
            throw new XRootDError(err, syscall);
        }
    }

    // 暴露给用户的友好接口，最后一个参数永远是可选的 options
    async stat(path: string, options?: XRootDOptions): Promise<StatInfo> {
        return this._run(() => this._internal.Stat(path), 'stat', options);
    }
}

```

**⚠️ 架构安全性问答：**
*问：JS 层抛出超时错误后，C++ 层的网络请求还在跑吗？会有内存泄漏吗？*
*答：是的，C++ 请求还在后台跑。**但是极其安全。**因为我们之前设计的 Handler 是**按值捕获 (Value Capture)** 且通过 `tsfn_.Release()` 管理生命周期的。当 C++ 请求最终（无论成功失败）返回时，它会去 resolve/reject 那个已经被 `Promise.race` 丢弃的底层 Promise，V8 会直接忽略它，然后 C++ 对象会正常 `delete this`。绝对不会发生内存泄漏或崩溃。*

---
