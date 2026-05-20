#pragma once
#include <napi.h>

#include <XrdCl/XrdClFile.hh>


class XrdNodeFile : public Napi::ObjectWrap<XrdNodeFile> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  XrdNodeFile(const Napi::CallbackInfo& info);
  ~XrdNodeFile() override;

  XrdCl::File* GetInternalFile() const { return file_; }

 private:
  XrdCl::File* file_;

  // 状态检查 (可同步)
  Napi::Value IsOpen(const Napi::CallbackInfo& info);
  Napi::Value IsSecure(const Napi::CallbackInfo& info);
  Napi::Value TryOtherServer(const Napi::CallbackInfo& info);

  // 异步 I/O，全数返回 Napi::Value (Promise)
  Napi::Value Open(const Napi::CallbackInfo& info);
  Napi::Value Close(const Napi::CallbackInfo& info);
  Napi::Value Stat(const Napi::CallbackInfo& info);
  Napi::Value Read(const Napi::CallbackInfo& info);

  /**
   * @brief 写入数据到文件 (支持两种重载)
   * 
   * Overload 1: 写入 Buffer 数据
   * @param offset (bigint) 写入的起始偏移量
   * @param buffer (Buffer) 要写入的二进制数据
   * @return Promise<void>
   * 
   * Overload 2: 从本地文件描述符 (fd) 写入数据
   * @param offset (bigint) 写入的起始偏移量
   * @param size (number) 写入的字节数
   * @param fd (number) 本地文件描述符
   * @param fdoff (bigint, optional) 从 fd 读取的偏移量 (如果不提供，则从 fd 当前光标读取)
   * @return Promise<void>
   */
  Napi::Value Write(const Napi::CallbackInfo& info);
  Napi::Value Sync(const Napi::CallbackInfo& info);
  Napi::Value Truncate(const Napi::CallbackInfo& info);
  Napi::Value PreRead(const Napi::CallbackInfo& info);

  // 向量读写与特殊操作
  Napi::Value VectorRead(const Napi::CallbackInfo& info);
  Napi::Value ReadChunks(const Napi::CallbackInfo& info);
  Napi::Value VectorWrite(const Napi::CallbackInfo& info);
  Napi::Value WriteV(const Napi::CallbackInfo& info);
  Napi::Value ReadV(const Napi::CallbackInfo& info);
  Napi::Value PgRead(const Napi::CallbackInfo& info);
  Napi::Value PgWrite(const Napi::CallbackInfo& info);
  Napi::Value Fcntl(const Napi::CallbackInfo& info);
  Napi::Value Visa(const Napi::CallbackInfo& info);

  // 属性与扩展属性
  Napi::Value GetProperty(const Napi::CallbackInfo& info);
  Napi::Value SetProperty(const Napi::CallbackInfo& info);

  /**
   * @brief 设置扩展属性
   * 
   * @param attrs (Record<string, string>) 键值对对象，包含要设置的属性
   * @return Promise<void>
   */
  Napi::Value SetXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 获取扩展属性
   * 
   * @param keys (string[]) 要获取的属性键名数组
   * @return Promise<Record<string, string>> (视 handler 的解析而定)
   */
  Napi::Value GetXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 删除扩展属性
   * 
   * @param keys (string[]) 要删除的属性键名数组
   * @return Promise<void>
   */
  Napi::Value DelXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 列出所有扩展属性
   * 
   * @return Promise<string[]> 包含所有扩展属性名称的数组
   */
  Napi::Value ListXAttr(const Napi::CallbackInfo& info);

  Napi::Value Clone(const Napi::CallbackInfo& info);
};