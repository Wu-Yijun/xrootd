#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>

class XrdNodeFile : public Napi::ObjectWrap<XrdNodeFile> {
 public:
  /**
   * @brief 初始化并将 XrdNodeFile 类及其方法注册到 JS 模块导出。
   * 
   * @param env N-API 环境。
   * @param exports JS 模块导出对象。
   * @return Napi::Object 注册后的导出对象。
   */
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  /**
   * @brief XrdNodeFile 构造函数。实例化底层的 XrdCl::File 对象。
   * 
   * JS 签名：`new File()`
   */
  XrdNodeFile(const Napi::CallbackInfo& info);

  /**
   * @brief 析构函数。释放底层的 XrdCl::File 对象。
   */
  ~XrdNodeFile() override;

  /**
   * @brief 获取内部底层的 XrdCl::File 指针。
   * 
   * @return XrdCl::File* 底层文件客户端指针。
   */
  XrdCl::File* GetInternalFile() const { return file_; }

 private:
  XrdCl::File* file_;

  // ==========================================================================
  // 状态检查 (同步方法)
  // ==========================================================================

  /**
   * @brief 检查文件是否处于打开状态。
   * 
   * JS 签名: `isOpen(): boolean`
   * @return Napi::Value 包含布尔值的 Napi::Value。
   */
  Napi::Value IsOpen(const Napi::CallbackInfo& info);

  /**
   * @brief 检查当前文件连接是否是安全的。
   * 
   * JS 签名: `isSecure(): boolean`
   * @return Napi::Value 包含布尔值的 Napi::Value。
   */
  Napi::Value IsSecure(const Napi::CallbackInfo& info);

  /**
   * @brief 检查当前连接失败时是否可尝试备用服务器。
   * 
   * JS 签名: `tryOtherServer(): boolean`
   * @return Napi::Value 包含布尔值的 Napi::Value。
   */
  Napi::Value TryOtherServer(const Napi::CallbackInfo& info);

  // ==========================================================================
  // 异步 I/O (Promise 返回)
  // ==========================================================================

  /**
   * @brief 异步打开远程 XRootD 文件。
   * 
   * JS 签名: `open(url: string, flags: number, mode: number): Promise<void>`
   * @param info 
   *   - info[0] (string): 远程文件的 URL 地址 (例如 "root://server:port//path")。
   *   - info[1] (number): 打开标志位 (如 OpenFlags.Read, OpenFlags.Write 等)。
   *   - info[2] (number): 创建权限模式 (如 AccessMode.UR, AccessMode.UW 等)。
   * @return Promise<void>
   */
  Napi::Value Open(const Napi::CallbackInfo& info);

  /**
   * @brief 异步关闭文件。
   * 
   * JS 签名: `close(): Promise<void>`
   * @return Promise<void>
   */
  Napi::Value Close(const Napi::CallbackInfo& info);

  /**
   * @brief 异步获取文件状态信息。
   * 
   * JS 签名: `stat(force?: boolean): Promise<StatInfo>`
   * @param info
   *   - info[0] (boolean, 可选): 是否强制从服务器拉取，为 true 时不使用缓存。
   * @return Promise<StatInfo>
   */
  Napi::Value Stat(const Napi::CallbackInfo& info);

  /**
   * @brief 异步读取文件片段。
   * 
   * JS 签名: `read(offset: bigint, size: number): Promise<Buffer>`
   * @param info
   *   - info[0] (bigint): 起始读取的字节偏移量。
   *   - info[1] (number): 要读取的字节数。
   * @return Promise<Buffer>
   */
  Napi::Value Read(const Napi::CallbackInfo& info);

  /**
   * @brief 异步写入数据到文件。
   * 
   * Overload 1 (Buffer 写入):
   * JS 签名: `write(offset: bigint, buffer: Buffer): Promise<void>`
   *   - info[0] (bigint): 写入的起始字节偏移量。
   *   - info[1] (Buffer): 要写入的二进制数据。
   * 
   * Overload 2 (从本地 fd 写入):
   * JS 签名: `write(offset: bigint, size: number, fd: number, fdoff?: bigint): Promise<void>`
   *   - info[0] (bigint): 写入的起始字节偏移量。
   *   - info[1] (number): 写入大小。
   *   - info[2] (number): 本地文件描述符 (fd)。
   *   - info[3] (bigint, 可选): 从本地 fd 中读取的起始偏移。如果不传，则从 fd 的当前指针位置读取。
   * @return Promise<void>
   */
  Napi::Value Write(const Napi::CallbackInfo& info);

  /**
   * @brief 异步同步刷盘 (将客户端缓存的所有修改持久化到服务器上)。
   * 
   * JS 签名: `sync(): Promise<void>`
   * @return Promise<void>
   */
  Napi::Value Sync(const Napi::CallbackInfo& info);

  /**
   * @brief 异步截断文件到指定大小。
   * 
   * JS 签名: `truncate(size: bigint): Promise<void>`
   * @param info
   *   - info[0] (bigint): 要截断到的目标文件大小。
   * @return Promise<void>
   */
  Napi::Value Truncate(const Napi::CallbackInfo& info);

  /**
   * @brief 异步预读取。向服务器发出预读请求以优化缓存性能。
   * 
   * JS 签名: `preRead(chunks: {offset: bigint, size: number}[]): Promise<void>`
   * @param info
   *   - info[0] (Array): 包含要预读的区块范围数组。
   * @return Promise<void>
   */
  Napi::Value PreRead(const Napi::CallbackInfo& info);

  // ==========================================================================
  // 向量读写与特殊操作 (Promise 返回)
  // ==========================================================================

  /**
   * @brief 异步多片段并行读取 (向量读 / Scatter Read)。
   * 
   * JS 签名: `vectorRead(chunks: {offset: bigint, size: number}[]): Promise<Buffer[]>`
   * @param info
   *   - info[0] (Array): 包含读取偏移和大小的对象数组。
   * @return Promise<Buffer[]> 每一个元素是一个对应请求大小的 Buffer 数组。
   */
  Napi::Value VectorRead(const Napi::CallbackInfo& info);

  /**
   * @brief VectorRead 的别名接口。
   * 
   * JS 签名: `readChunks(chunks: {offset: bigint, size: number}[]): Promise<Buffer[]>`
   */
  Napi::Value ReadChunks(const Napi::CallbackInfo& info);

  /**
   * @brief 异步多片段并行写入 (向量写 / Gather Write)。
   * 
   * JS 签名: `vectorWrite(chunks: {offset: bigint, buffer: Buffer}[]): Promise<void>`
   * @param info
   *   - info[0] (Array): 包含写入偏移与 Buffer 数据的对象数组。
   * @return Promise<void>
   */
  Napi::Value VectorWrite(const Napi::CallbackInfo& info);

  /**
   * @brief 异步分聚式写入。在特定偏移量处并行写入多个 Buffer 数据。
   * 
   * JS 签名: `writeV(offset: bigint, buffers: Buffer[]): Promise<void>`
   * @param info
   *   - info[0] (bigint): 写入的起始字节偏移量。
   *   - info[1] (Array): 要写入的 Buffer 数组。
   * @return Promise<void>
   */
  Napi::Value WriteV(const Napi::CallbackInfo& info);

  /**
   * @brief 分聚式读取 (目前未实现，抛出异常)。
   * 
   * JS 签名: `readV(): Promise<Buffer[]>`
   * @return Promise<Buffer[]>
   */
  Napi::Value ReadV(const Napi::CallbackInfo& info);

  /**
   * @brief 异步分页式读取数据。支持以页为单位的高性能对齐读。
   * 
   * JS 签名: `pgRead(offset: bigint, size: number): Promise<Buffer>`
   * @param info
   *   - info[0] (bigint): 起始页对齐偏移量（必须是 4096 的整数倍）。
   *   - info[1] (number): 读取的数据大小，至少为 1 页大小（4KB）。
   * @return Promise<Buffer>
   */
  Napi::Value PgRead(const Napi::CallbackInfo& info);

  /**
   * @brief 异步分页式写入数据并校验。
   * 
   * JS 签名: `pgWrite(offset: bigint, size: number, buffer: Buffer, cksums: number[]): Promise<void>`
   * @param info
   *   - info[0] (bigint): 目标页对齐偏移量。
   *   - info[1] (number): 写入数据的大小。
   *   - info[2] (Buffer): 包含数据的 Buffer。
   *   - info[3] (number[]): 每个 4KB 页对应的 CRC32C 校验码数组。
   * @return Promise<void>
   */
  Napi::Value PgWrite(const Napi::CallbackInfo& info);

  /**
   * @brief 在打开的文件上异步执行自定义的控制和管理操作（具体取决于服务端实现）。
   * 
   * JS 签名: `fcntl(arg: Buffer): Promise<Buffer>`
   * @param info
   *   - info[0] (Buffer): 包含操作和参数的 Buffer。
   * @return Promise<Buffer> 服务器返回的响应数据 Buffer。
   */
  Napi::Value Fcntl(const Napi::CallbackInfo& info);

  /**
   * @brief 异步查询文件的 visa (凭证及内部特有属性) 信息。
   * 
   * JS 签名: `visa(): Promise<Buffer>`
   * @return Promise<Buffer> 返回的 visa 数据 Buffer。
   */
  Napi::Value Visa(const Napi::CallbackInfo& info);

  // ==========================================================================
  // 属性与扩展属性 (部分同步，部分 Promise)
  // ==========================================================================

  /**
   * @brief 同步获取文件客户端本地属性值。
   * 
   * JS 签名: `getProperty(name: string): { success: boolean, value: string }`
   * @param info
   *   - info[0] (string): 属性名称。
   * @return Napi::Value 包含 { success, value } 的 JS 对象。
   */
  Napi::Value GetProperty(const Napi::CallbackInfo& info);

  /**
   * @brief 同步设置文件客户端本地属性值。
   * 
   * JS 签名: `setProperty(name: string, value: string): boolean`
   * @param info
   *   - info[0] (string): 属性名称。
   *   - info[1] (string): 要设置的属性值。
   * @return Napi::Value 包含设置成功与否布尔值的 JS 对象。
   */
  Napi::Value SetProperty(const Napi::CallbackInfo& info);

  /**
   * @brief 异步批量设置文件的扩展属性 (XAttr)。
   * 
   * JS 签名: `setXAttr(attrs: Record<string, string>): Promise<XAttrStatusResult[]>`
   * @param info
   *   - info[0] (Record<string, string>): 需要设置的扩展属性键值对对象。
   * @return Promise<XAttrStatusResult[]> 各个属性设置状态结果数组。
   */
  Napi::Value SetXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 异步批量查询文件的指定扩展属性 (XAttr)。
   * 
   * JS 签名: `getXAttr(keys: string[]): Promise<Record<string, string>>`
   * @param info
   *   - info[0] (string[]): 要查询的扩展属性键名数组。
   * @return Promise<Record<string, string>> 查询到的属性键值对结果。
   */
  Napi::Value GetXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 异步批量删除文件的指定扩展属性 (XAttr)。
   * 
   * JS 签名: `delXAttr(keys: string[]): Promise<XAttrStatusResult[]>`
   * @param info
   *   - info[0] (string[]): 要删除的属性键名数组。
   * @return Promise<XAttrStatusResult[]> 各个属性删除状态结果数组。
   */
  Napi::Value DelXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 异步批量获取文件的所有已设置扩展属性 (XAttr)。
   * 
   * JS 签名: `listXAttr(): Promise<Record<string, string>>`
   * @return Promise<Record<string, string>> 所有已设置的扩展属性键值对结果。
   */
  Napi::Value ListXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 异步在 EOS 层面克隆指定的数据区块位置（用于不经过客户端中转的服务端直接数据复制）。
   * 
   * JS 签名: `clone(list: CloneLocationRequest[]): Promise<void>`
   * @param info
   *   - info[0] (Array): 包含要克隆的数据范围详情数组 (每个项包括 srcFile, dstOffset, srcOffset, length)。
   * @return Promise<void>
   */
  Napi::Value Clone(const Napi::CallbackInfo& info);
};