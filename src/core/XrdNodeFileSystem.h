#pragma once
#include <napi.h>

#include <XrdCl/XrdClFileSystem.hh>


class XrdNodeFileSystem : public Napi::ObjectWrap<XrdNodeFileSystem> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  XrdNodeFileSystem(const Napi::CallbackInfo& info);
  ~XrdNodeFileSystem() override;

  XrdCl::FileSystem* GetInternalFS() const { return fs_; }

 private:
  XrdCl::FileSystem* fs_;

  // ============================================================================
  // 异步 I/O (均通过 Napi::ThreadSafeFunction 跳回主线程，由底层 Handler 接管)
  // ============================================================================

  /**
   * @brief 定位文件所在的服务器和访问权限。
   * @param path [String] 目标路径。
   * @param flags [Number] XrdCl::OpenFlags::Flags 枚举值。
   * @return Promise<Array<{ address: string, type: number, accessType: number }>>
   * @note 使用 FSLocateHandler，提取底层的 XrdCl::LocationInfo 结构。
   */
  Napi::Value Locate(const Napi::CallbackInfo& info);

  /**
   * @brief 深度定位文件（触发服务器集群向各数据节点查询实际位置）。
   * @param path [String] 目标路径。
   * @param flags [Number] XrdCl::OpenFlags::Flags 枚举值。
   * @return Promise<Array<{ address: string, type: number, accessType: number }>>
   * @note 使用 FSLocateHandler，提取底层的 XrdCl::LocationInfo 结构。
   */
  Napi::Value DeepLocate(const Napi::CallbackInfo& info);

  /**
   * @brief 重命名或移动文件/目录。
   * @param source [String] 源路径。
   * @param dest [String] 目标路径。
   * @return Promise<undefined>
   * @note 使用 FSControlHandler，通用控制流 Handler，忽略底层响应体数据，仅处理状态码。
   */
  Napi::Value Mv(const Napi::CallbackInfo& info);

  /**
   * @brief 向服务器发送特定查询指令（如查询空间、配置等）。
   * @param queryCode [Number] XrdCl::QueryCode::Code 查询码枚举。
   * @param arg [Buffer] 查询参数。
   * @return Promise<Buffer> 返回服务器响应的二进制数据。
   * @note 使用 FSBufferHandler，负责获取底层 XrdCl::Buffer 并安全拷贝为 Napi::Buffer<char>。
   */
  Napi::Value Query(const Napi::CallbackInfo& info);

  /**
   * @brief 截断或扩展文件至指定大小。
   * @param path [String] 目标文件路径。
   * @param size [BigInt | Number] 目标大小（字节数）。
   * @return Promise<undefined>
   * @note 使用 FSControlHandler。
   */
  Napi::Value Truncate(const Napi::CallbackInfo& info);

  /**
   * @brief 删除单个文件。
   * @param path [String] 待删除的文件路径。
   * @return Promise<undefined>
   * @note 使用 FSControlHandler。
   */
  Napi::Value Rm(const Napi::CallbackInfo& info);

  /**
   * @brief 创建目录。
   * @param path [String] 目录路径。
   * @param flags [Number] XrdCl::MkDirFlags::Flags 创建标志。
   * @param mode [Number] XrdCl::Access::Mode 权限掩码。
   * @return Promise<undefined>
   * @note 使用 FSControlHandler。
   */
  Napi::Value MkDir(const Napi::CallbackInfo& info);

  /**
   * @brief 删除空目录。
   * @param path [String] 待删除的目录路径。
   * @return Promise<undefined>
   * @note 使用 FSControlHandler。
   */
  Napi::Value RmDir(const Napi::CallbackInfo& info);

  /**
   * @brief 修改文件或目录的访问权限。
   * @param path [String] 目标路径。
   * @param mode [Number] XrdCl::Access::Mode 权限掩码。
   * @return Promise<undefined>
   * @note 使用 FSControlHandler。
   */
  Napi::Value ChMod(const Napi::CallbackInfo& info);

  /**
   * @brief 向服务器发送 Ping 以检查连接与服务活性。
   * @param 无参数。
   * @return Promise<undefined>
   * @note 使用 FSControlHandler。
   */
  Napi::Value Ping(const Napi::CallbackInfo& info);

  /**
   * @brief 获取文件或目录的详细元数据信息。
   * @param path [String] 目标路径。
   * @return Promise<Object> 包含 id, size (BigInt), modTime, flags, owner, checksum 等属性。
   * @note 使用 AsyncStatHandler，提取底层的 XrdCl::StatInfo 结构。
   */
  Napi::Value Stat(const Napi::CallbackInfo& info);

  /**
   * @brief 获取虚拟文件系统 (VFS) 的空间利用率与配额统计信息。
   * @param path [String] 目标路径。
   * @return Promise<Object> 包含 nodesRW/freeRW/nodesStaging/freeStaging (BigInt) 及 utilizationRW/utilizationStaging (Number)。
   * @note 使用 FSStatVFSHandler，提取底层的 XrdCl::StatInfoVFS 结构。
   */
  Napi::Value StatVFS(const Napi::CallbackInfo& info);

  /**
   * @brief 获取底层协议信息（当前实现为预留/TODO）。
   * @return undefined
   */
  Napi::Value Protocol(const Napi::CallbackInfo& info);

  /**
   * @brief 列出目录下的所有文件和子目录列表。
   * @param path [String] 目录路径。
   * @param flags [Number] XrdCl::DirListFlags::Flags 标志位（如是否包含 Stat 信息）。
   * @return Promise<Array<{ name: string, hostAddress: string, stat?: Object }>>
   * @note 使用 FSDirListHandler，遍历底层的 XrdCl::DirectoryList 容器。
   */
  Napi::Value DirList(const Napi::CallbackInfo& info);

  /**
   * @brief 向文件系统发送通用信息/控制指令。
   * @param info [String] 指令字符串。
   * @return Promise<Buffer> 返回服务器响应的数据。
   * @note 使用 FSBufferHandler。
   */
  Napi::Value SendInfo(const Napi::CallbackInfo& info);

  /**
   * @brief 向集群发起文件预取/暂存准备请求（例如将磁带文件恢复至磁盘）。
   * @param fileList [Array<String>] 文件路径列表。
   * @param flags [Number] XrdCl::PrepareFlags::Flags 标志位。
   * @param priority [Number] 优先级。
   * @return Promise<Buffer> 响应数据流。
   * @note 使用 FSBufferHandler。
   */
  Napi::Value Prepare(const Napi::CallbackInfo& info);

  /**
   * @brief 向缓存子系统发送特定指令。
   * @param info [String] 缓存控制指令。
   * @return Promise<Buffer> 响应数据。
   * @note 使用 FSBufferHandler。
   */
  Napi::Value SendCache(const Napi::CallbackInfo& info);

  // ============================================================================
  // 属性与扩展属性 (Get/Set Property 为同步操作，XAttr 均为异步操作)
  // ============================================================================

  /**
   * @brief 同步获取客户端/文件系统层面的属性配置。
   * @param name [String] 属性名。
   * @return Object { success: boolean, value: string }
   * @note 无 Handler，直接在 V8 主线程调用底层同步方法。
   */
  Napi::Value GetProperty(const Napi::CallbackInfo& info);

  /**
   * @brief 同步设置客户端/文件系统层面的属性配置。
   * @param name [String] 属性名。
   * @param value [String] 属性值。
   * @return Boolean 设置成功与否。
   * @note 无 Handler，直接在 V8 主线程调用底层同步方法。
   */
  Napi::Value SetProperty(const Napi::CallbackInfo& info);

  /**
   * @brief 设置文件或目录的扩展属性 (Extended Attributes)。
   * @param path [String] 目标路径。
   * @param obj [Object] 键值对 { [key: string]: string }。
   * @return Promise<Array<{ name: string, isOk: boolean, code: number, message: string }>> 每一项设置的状态。
   * @note 使用 FSXAttrStatusHandler，提取 std::vector<XrdCl::XAttrStatus> 并映射。
   */
  Napi::Value SetXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 查询文件或目录指定的扩展属性。
   * @param path [String] 目标路径。
   * @param keys [Array<String>] 属性键列表。
   * @return Promise<Object> 键值对 { [key: string]: string }（仅包含查询成功的属性）。
   * @note 使用 FSXAttrDataHandler，提取 std::vector<XrdCl::XAttr> 并反序列化为 JS Object。
   */
  Napi::Value GetXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 删除文件或目录指定的扩展属性。
   * @param path [String] 目标路径。
   * @param keys [Array<String>] 属性键列表。
   * @return Promise<Array<{ name: string, isOk: boolean, code: number, message: string }>> 每一项删除的状态。
   * @note 使用 FSXAttrStatusHandler。
   */
  Napi::Value DelXAttr(const Napi::CallbackInfo& info);

  /**
   * @brief 列出文件或目录存在的所有扩展属性。
   * @param path [String] 目标路径。
   * @return Promise<Object> 键值对 { [key: string]: string }。
   * @note 使用 FSXAttrDataHandler。
   */
  Napi::Value ListXAttr(const Napi::CallbackInfo& info);
};