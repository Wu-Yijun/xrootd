# XRootD TypeScript Support

A high-performance, enterprise-grade Node.js binding for the [XRootD](https://github.com/xrootd/xrootd) client library. Designed to bring seamless, extreme-throughput data access to the TypeScript ecosystem.

Github Repo: [**Wu-Yijun/xrootd**](https://github.com/Wu-Yijun/xrootd); NPM package: [**XRootD**](https://www.npmjs.com/package/xrootd)

> [!NOTE]
> 🚀 Exciting News: A pure Node.js implementation is coming!
> We are rewriting xrootd in pure native Node.js to eliminate C++ compilation (node-gyp) issues. You can try the beta version now:
> ```bash
> npm install xrootd@next
> ```
> Feedback is highly appreciated!

## Supported Capabilities

* **`FileSystem`**: Cluster-level operations (`stat`, `dirList`, `rm`, `mkdir`, `locate`).
* **`File`**: High-performance I/O (`open`, `read`, `write`, `vectorRead`, server-side `clone`).
* **`CopyProcess`**: Asynchronous, highly-parallel data transfers with real-time progress callbacks.
* **`Env`**: Type-safe configuration management and OS-level auth protocol routing.
* **`Url`**: XRootD-specific URI scheme parsing and validation.


## Prerequisites

To ensure compatibility with the pre-compiled native bindings, your environment must meet the following requirements:

* **Node.js**: Version 20.x or higher.
* **System Dependencies (Linux)**: Requires `libstdc++` providing `GLIBCXX_3.4.26` or newer (typically GCC 9+, e.g., CentOS 9 / Ubuntu 20.04+).
* **System Dependencies (Macos)**: Requires macos 14 or later for arm64 architecture, macos 15 or later for intel architecture.
* **Authentication**: Ensure your host machine has valid tickets (e.g., `kinit` for Kerberos) if accessing a secured cluster.


## Installation

```bash
npm install xrootd
# or
yarn add xrootd

```

---

## Usage Examples

### 1. Configuration & Basic FileSystem

```typescript
import { Env, FileSystem } from 'xrootd';

// Safely configure underlying XRootD environment
Env.configure({
    RequestTimeout: 30,       // Prevent 30-min infinite hangs (Node.js friendly)
    WorkerThreads: 4,         // Boost underlying multiplexing
    SecProtocol: 'krb5,unix'  // Force Kerberos or Unix socket auth
});

const fs = new FileSystem('root://eos01.ihep.ac.cn/');

async function checkCluster() {
    try {
        const stat = await fs.stat('/eos/lhaaso/data');
        console.log(`Directory size: ${stat.size} bytes`);
        console.log(`Modified at: ${stat.modTimeAsString}`);
    } catch (err: any) {
        if (err.code === 'ENOENT') {
            console.error('File not found in cluster.');
        } else {
            console.error(err.message);
        }
    }
}

```

### 2. Zero-Copy File Reading

```typescript
import { File, OpenFlags } from 'xrootd';

async function readHeader() {
    const file = new File();
    
    // Auto-translates and handles C++ lifecycle
    await file.open('root://eos01.ihep.ac.cn//eos/test.dat', OpenFlags.Read);
    
    // The returned buffer is a zero-copy mapping of C++ allocated memory
    const buffer = await file.read(0n, 1024);
    console.log(buffer.toString('utf-8'));
    
    await file.close();
}

```

### 3. High-Performance CopyProcess with Progress

```typescript
import { CopyProcess } from 'xrootd';

async function syncData() {
    const cp = new CopyProcess();
    
    cp.addJob({
        source: 'root://eos01.ihep.ac.cn//eos/data.raw',
        target: '/local/disk/data.raw',
        force: true,
        parallelChunks: 4
    });

    await cp.prepare();
    
    const results = await cp.run((jobNum, processed, total) => {
        const percent = ((processed / total) * 100).toFixed(2);
        console.log(`Job ${jobNum} Progress: ${percent}%`);
    });

    console.log('Copy completed:', results);
}

```

## ✨ Why `xrootd`? (Core Architecture)

Unlike traditional Node.js C++ addons, `xrootd` is architected for **High Energy Physics (HEP)** and **Big Data** workloads:

*  **True Native Async**: Bypasses the notoriously limited Node.js `libuv` thread pool (default 4 threads). It hooks directly into XRootD's underlying C++ event loop via N-API `ThreadSafeFunction`, allowing thousands of concurrent requests without blocking the V8 engine.
*  **Absolute Zero-Copy I/O**: Implements direct memory handoffs. Data read from the EOS cluster is mounted directly as V8 `Buffer` objects without a single byte of internal memory copying, completely eliminating GC (Garbage Collection) pauses during heavy I/O.
*  **Idiomatic Node.js Experience**: "Thin C++, Thick TS". Complex XRootD protocol errors (e.g., `3011`) are smartly translated into standard Node.js exceptions (e.g., `ENOENT`, `EACCES`), complete with system call contexts and retryable flags.
*  **Zero-Config Authentication**: Bundles essential security plugins (Kerberos, SSS, Unix, Token) natively. Path injection is handled automatically under the hood—no more `[FATAL] Auth failed` or missing `.so` nightmares.

---

##  Licensing

This project is released under a **Dual License** strategy to balance open-source compatibility and developer freedom:

1. **[GNU GPLv3](LICENSE-GPLv3)**: The native binding codebase adheres to the GPLv3 license to remain fully compatible with the upstream C++ XRootD project.
2. **[MIT](LICENSE-MIT)**: The core TypeScript APIs, interface definitions, and glue layers are provided under the MIT license, allowing you to integrate the TS components into your own software architectures without viral restrictive requirements.

*Disclaimer: This project is a third-party community initiative built for modern web ecosystems and is not affiliated with, officially endorsed by, or sponsored by the core XRootD project.*

---

## Contributing

We welcome contributions from the high-energy physics, astrophysics, and Node.js communities! Whether it's reporting bugs, improving documentation, or adding support for advanced XRootD features.

*Contribution guidelines coming soon.*