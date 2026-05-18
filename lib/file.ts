// lib/file.ts

import { Readable, Writable } from "stream";
import nativeAddon from "./native.ts";
import type {
  INativeFile,
  ReadChunkRequest,
  StatInfo,
  ReadStreamOptions,
  WriteStreamOptions,
  CloneLocationRequest
} from "./types.ts";
import {
  OpenFlags,
  AccessMode,
} from './types.ts';

/**
 * XRootD File 客户端
 * 提供对远程文件的异步读写操作及 Node.js 风格的流式接口。
 */
export class File {
  private _internal: INativeFile;

  // 允许通过内部实例进行构造（专为 Clone 设计）
  constructor(internalInstance?: INativeFile) {
    if (internalInstance) {
      this._internal = internalInstance;
    } else {
      if (!nativeAddon || !nativeAddon.File) {
        throw new Error('Native addon not loaded properly.');
      }
      this._internal = new nativeAddon.File();
    }
  }

  // ==========================================================================
  // 核心 I/O 接口 (Promise 封装)
  // ==========================================================================

  /**
   * 打开远程文件
   * @param url 目标地址 (如 root://server//path/to/file)
   * @param flags 打开标志位
   * @param mode 访问权限模式
   */
  async open(
    url: string,
    flags: OpenFlags = OpenFlags.None,
    mode: AccessMode = AccessMode.None,
  ): Promise<void> {
    return this._internal.Open(url, flags, mode);
  }

  /**
   * 关闭文件
   */
  async close(): Promise<void> {
    return this._internal.Close();
  }

  /**
   * 获取文件状态
   */
  async stat(): Promise<StatInfo> {
    return this._internal.Stat();
  }

  /**
   * 读取文件块 (Zero-Copy from C++)
   * @param offset 偏移量 (支持 >2GB)
   * @param size 读取字节数
   * @returns 包含数据的 Node.js Buffer
   */
  async read(offset: bigint | number, size: number): Promise<Buffer> {
    return this._internal.Read(BigInt(offset), size);
  }

  /**
   * 写入文件块
   * @param offset 偏移量
   * @param buffer 要写入的数据
   */
  async write(offset: bigint | number, buffer: Buffer): Promise<void> {
    return this._internal.Write(BigInt(offset), buffer);
  }

  /**
   * 同步文件缓冲区到磁盘
   */
  async sync(): Promise<void> {
    return this._internal.Sync();
  }

  /**
   * 截断文件
   * @param size 目标大小
   */
  async truncate(size: bigint | number): Promise<void> {
    return this._internal.Truncate(BigInt(size));
  }

  /**
   * 检查本地实例状态 (同步操作)
   */
  isOpen(): boolean {
    return this._internal.IsOpen();
  }

  // ==========================================================================
  // 扩展属性接口 (XAttr & Properties)
  // ==========================================================================

  async getProperty(name: string): Promise<string> {
    return this._internal.GetProperty(name);
  }

  async setProperty(name: string, value: string): Promise<void> {
    return this._internal.SetProperty(name, value);
  }
  // ==========================================================================
  // 向量化/块读取 (高性能 I/O)
  // ==========================================================================

  /**
   * 向量化读取 (Vector Read)
   * 在单个请求中从文件的多个非连续区域读取数据，极大地减少网络往返开销。
   * @param chunks 包含 offset 和 size 的读取请求数组
   * @returns 与请求数组顺序对应的 Buffer 数组
   */
  async vectorRead(chunks: ReadChunkRequest[]): Promise<Buffer[]> {
    // 确保传递给底层的 offset 是 bigint，防止精度丢失
    const normalizedChunks = chunks.map(c => ({
      offset: BigInt(c.offset),
      size: c.size
    }));
    return this._internal.VectorRead(normalizedChunks);
  }

  /**
   * 读取块 (Read Chunks)
   * 类似于 VectorRead，但底层实现可能利用更高级的预读或多路复用策略。
   */
  async readChunks(chunks: ReadChunkRequest[]): Promise<Buffer[]> {
    const normalizedChunks = chunks.map(c => ({
      offset: BigInt(c.offset),
      size: c.size
    }));
    return this._internal.ReadChunks(normalizedChunks);
  }

  // ==========================================================================
  // 扩展属性管理 (Extended Attributes)
  // ==========================================================================

  /**
   * 设置文件的扩展属性
   */
  async setXAttr(name: string, value: string): Promise<void> {
    return this._internal.SetXAttr(name, value);
  }

  /**
   * 获取文件的扩展属性
   */
  async getXAttr(name: string): Promise<string> {
    return this._internal.GetXAttr(name);
  }

  /**
   * 删除文件的扩展属性
   */
  async delXAttr(name: string): Promise<void> {
    return this._internal.DelXAttr(name);
  }

  /**
   * 列出文件所有的扩展属性名称
   */
  async listXAttr(): Promise<string[]> {
    return this._internal.ListXAttr();
  }

  // ==========================================================================
  // 实例控制
  // ==========================================================================

  /**
   * 将其他文件的指定区间在服务器端克隆到当前文件
   * 当前文件必须以写入/更新模式打开
   */
  clone(locations: CloneLocationRequest[]): Promise<void>{
    return this._internal.Clone(locations);
  }

  // ==========================================================================
  // 高级 Node.js 生态适配: Streams
  // ==========================================================================

  /**
   * 创建一个可读流 (Readable Stream)
   * 使得 XRootD 文件可以无缝 pipe 到其他 Node.js 流 (如本地 fs, HTTP response)
   */
  createReadStream(options: ReadStreamOptions = {}): Readable {
    let currentOffset = options.start ?? 0n;
    const endOffset = options.end;
    const chunkSize = options.highWaterMark ?? 64 * 1024; // 默认 64KB

    // 保存 this 引用，以在 Readable 的闭包中使用
    const self = this;

    return new Readable({
      highWaterMark: chunkSize,
      async read(size: number) {
        try {
          // 如果指定了结束位置，计算本次最多能读多少
          let bytesToRead = size;
          if (endOffset !== undefined) {
            const remaining = endOffset - currentOffset + 1n;
            if (remaining <= 0n) {
              this.push(null); // EOF
              return;
            }
            if (remaining < BigInt(bytesToRead)) {
              bytesToRead = Number(remaining);
            }
          }

          // 调用底层的 Promise 读取
          const buffer = await self.read(currentOffset, bytesToRead);

          if (buffer.length === 0) {
            // 读到文件末尾
            this.push(null);
          } else {
            currentOffset += BigInt(buffer.length);
            this.push(buffer);
          }
        } catch (err) {
          this.destroy(err instanceof Error ? err : new Error(String(err)));
        }
      },
    });
  }

  /**
   * 创建一个可写流 (Writable Stream)
   * 支持通过 pipe 将大量数据流式写入 XRootD 服务器
   */
  createWriteStream(options: WriteStreamOptions = {}): Writable {
    let currentOffset = options.start ?? 0n;
    const self = this;

    return new Writable({
      async write(
        chunk: Buffer,
        encoding: BufferEncoding,
        callback: (error?: Error | null) => void,
      ) {
        try {
          // 如果传入的是 string，需要将其转为 Buffer
          const buffer = Buffer.isBuffer(chunk)
            ? chunk
            : Buffer.from(chunk, encoding);

          await self.write(currentOffset, buffer);
          currentOffset += BigInt(buffer.length);

          callback(); // 成功完成当前 chunk
        } catch (err) {
          callback(err instanceof Error ? err : new Error(String(err)));
        }
      },

      // 支持批量写入优化 (如果 Node.js 底层缓冲了多个 chunk)
      async writev(
        chunks: Array<{ chunk: any; encoding: BufferEncoding }>,
        callback: (error?: Error | null) => void,
      ) {
        try {
          // 合并 chunk 以减少网络和 C++ 跨层调用的开销
          const buffers = chunks.map((c) =>
            Buffer.isBuffer(c.chunk)
              ? c.chunk
              : Buffer.from(c.chunk, c.encoding)
          );
          const masterBuffer = Buffer.concat(buffers);

          await self.write(currentOffset, masterBuffer);
          currentOffset += BigInt(masterBuffer.length);

          callback();
        } catch (err) {
          callback(err instanceof Error ? err : new Error(String(err)));
        }
      },
    });
  }
}
