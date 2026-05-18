// lib/filesystem.ts
import { posix } from 'node:path';
import nativeAddon from './native.ts';
import type {
  INativeFileSystem,
  LocationInfo,
  StatInfo,
  PropertyList,
  StatVFSInfo,
  DirListEntry,
  XAttrStatusResult
} from './types.ts';
import {
  MkDirFlags,
  AccessMode,
  StatFlags,
} from './enums.ts';
import { reverseStr } from './utils.ts';

/**
 * XRootD FileSystem 客户端
 * 提供对远程服务器目录和文件的通用管理操作。
 */
export class FileSystem {
  private _internal: INativeFileSystem;
  public readonly serverUrl: string;

  /**
   * 实例化一个 FileSystem 客户端
   * @param url 服务器地址 (例如: 'root://eospublic.cern.ch')
   */
  constructor(url: string) {
    if (!nativeAddon || !nativeAddon.FileSystem) {
      throw new Error('Native addon not loaded properly. Cannot instantiate FileSystem.');
    }
    this.serverUrl = url;
    this._internal = new nativeAddon.FileSystem(url);
  }

  /**
   * 内部辅助方法：确保路径是标准的 Unix 绝对路径
   */
  private _normalize(targetPath: string): string {
    // XRootD 期望的通常是以 '/' 开头的绝对路径
    const posixPath = posix.normalize(targetPath);
    return posixPath.startsWith('/') ? posixPath : '/' + posixPath;
  }

  // ==========================================================================
  // 核心 API (Promise 代理)
  // ==========================================================================

  /**
   * 定位文件在集群中的具体数据节点
   * @param filePath 目标文件路径
   * @param flags 定位标志位 (默认 0)
   * @returns 包含主机、端口等信息的数组
   */
  async locate(filePath: string, flags: number = 0): Promise<LocationInfo[]> {
    return this._internal.Locate(this._normalize(filePath), flags);
  }

  /**
   * 获取文件或目录的状态信息
   * @param targetPath 目标路径
   */
  async stat(targetPath: string): Promise<StatInfo> {
    const rawStat = await this._internal.Stat(this._normalize(targetPath));
    return {
      ...rawStat,
      get modeOctal() { return reverseStr(rawStat.modeAsOctString); },
      get modeString() { return reverseStr(rawStat.modeAsString); },
      get isFile() { return (rawStat.flags & StatFlags.IsDir) === 0 && (rawStat.flags & StatFlags.Other) === 0 },
      get isDir() { return (rawStat.flags & StatFlags.IsDir) !== 0 },
      get isOther() { return (rawStat.flags & StatFlags.Other) !== 0 },
      get isOffline() { return (rawStat.flags & StatFlags.Offline) !== 0 },
      get isPOSCPending() { return (rawStat.flags & StatFlags.POSCPending) !== 0 },
      get isReadable() { return (rawStat.flags & StatFlags.IsReadable) !== 0 },
      get isWritable() { return (rawStat.flags & StatFlags.IsWritable) !== 0 },
      get isBackUpExists() { return (rawStat.flags & StatFlags.BackUpExists) !== 0 },
    };
  }

  /**
   * 删除远程文件
   * @param filePath 要删除的文件路径
   */
  async rm(filePath: string): Promise<void> {
    return this._internal.Rm(this._normalize(filePath));
  }

  /**
   * 创建远程目录
   * @param dirPath 目录路径
   * @param flags 标志位 (例如 MakePath，允许创建多级父目录)
   * @param mode 访问权限模式 (默认 0755 对应的 AccessMode)
   */
  async mkdir(
    dirPath: string,
    flags: MkDirFlags = MkDirFlags.None,
    mode: AccessMode = AccessMode.UR | AccessMode.UW | AccessMode.UX | AccessMode.GR | AccessMode.GX | AccessMode.OR | AccessMode.OX
  ): Promise<void> {
    // 对应底层 C++ XrdCl::FileSystem::MkDir，接受 path, flags, mode
    return this._internal.MkDir(this._normalize(dirPath), flags, mode);
  }

  /**
   * 删除远程目录 (目录必须为空)
   * @param dirPath 要删除的目录路径
   */
  async rmdir(dirPath: string): Promise<void> {
    return this._internal.RmDir(this._normalize(dirPath));
  }

  /**
   * 移动或重命名文件/目录
   * @param source 源路径
   * @param dest 目标路径
   */
  async mv(source: string, dest: string): Promise<void> {
    return this._internal.Mv(this._normalize(source), this._normalize(dest));
  }

  /**
   * 列出目录下的所有文件和子目录名
   * @param dirPath 目录路径
   * @param flags 标志位 (例如是否显示隐藏文件)
   */
  async dirList(dirPath: string, flags: number = 0): Promise<DirListEntry[]> {
    return this._internal.DirList(this._normalize(dirPath), flags);
  }

  // ==========================================================================
  // 高级 Node.js 风格辅助方法 (糖)
  // ==========================================================================

  /**
   * 检查文件或目录是否存在
   * (通过捕获 stat 的错误来实现，类似于老版本 Node 的 fs.exists)
   * @param targetPath 目标路径
   */
  async exists(targetPath: string): Promise<boolean> {
    try {
      await this.stat(targetPath);
      return true;
    } catch (err: any) {
      // 假设底层 C++ 在找不到文件时会抛出包含错误码的 error
      // 具体的 code 取决于 error_handler.cc 中的实现 (如 ENOENT)
      if (err.code === 'ENOENT' || err.status === 3011) { // 3011 是 XRootD 常见的 kXR_NotFound
        return false;
      }
      // 其他网络或权限错误则继续向上抛出
      throw err;
    }
  }

  /**
   * 确保目录存在。如果目录不存在，则会自动创建它及其所有父目录。
   * (类似于 fs-extra 的 ensureDir 或 mkdir -p)
   * @param dirPath 目标目录
   */
  async ensureDir(dirPath: string): Promise<void> {
    const isExist = await this.exists(dirPath);
    if (!isExist) {
      // 传入 MakePath flag (通常对应 XRootD 的 MkDirFlags.MakePath)
      await this.mkdir(dirPath, MkDirFlags.MakePath);
    }
  }

  // ==========================================================================
  // 高级集群操作与元数据管理 (补全接口)
  // ==========================================================================

  /**
   * 深度定位：返回包含所有数据节点副本的详细物理位置信息
   */
  async deepLocate(filePath: string, flags: number = 0): Promise<LocationInfo[]> {
    return this._internal.DeepLocate(this._normalize(filePath), flags);
  }

  /**
   * 在不打开文件的情况下，直接截断目标文件
   */
  async truncate(filePath: string, size: bigint | number): Promise<void> {
    return this._internal.Truncate(this._normalize(filePath), size);
  }

  /**
   * 更改远程文件或目录的访问权限
   */
  async chmod(targetPath: string, mode: AccessMode): Promise<void> {
    return this._internal.ChMod(this._normalize(targetPath), mode);
  }

  /**
   * 探活：检查远端文件系统服务是否响应
   */
  async ping(): Promise<void> {
    return this._internal.Ping();
  }

  /**
   * 获取虚拟文件系统(VFS)的状态（如磁盘总容量、剩余可用空间）
   */
  async statVFS(targetPath: string): Promise<StatVFSInfo> {
    return this._internal.StatVFS(this._normalize(targetPath));
  }

  /**
   * 获取当前连接协议的详细属性
   */
  async protocol(): Promise<PropertyList> {
    return this._internal.Protocol();
  }

  /**
   * 发送带外查询指令到数据节点 (通常用于 XRootD 的高级自定义插件)
   */
  async query(queryCode: number, args: Buffer): Promise<Buffer> {
    return this._internal.Query(queryCode, args);
  }

  /**
   * 发送通用信息到服务器
   */
  async sendInfo(info: string): Promise<Buffer> {
    return this._internal.SendInfo(info);
  }

  /**
   * 发送缓存操作信息给集群
   */
  async sendCache(info: string): Promise<Buffer> {
    return this._internal.SendCache(info);
  }

  /**
   * 数据预热/暂存 (Staging)：
   * 在处理海量物理数据时，通知存储集群将特定的冷数据（如磁带上的文件）提前拉取到磁盘缓存。
   * @param targetPaths 需要预热的路径数组
   * @param flags 预热策略标志
   * @param priority 优先级
   */
  async prepare(targetPaths: string[], flags: number = 0, priority: number = 0): Promise<Buffer> {
    // 对所有的请求路径进行安全清理
    const normalizedPaths = targetPaths.map(p => this._normalize(p));
    return this._internal.Prepare(normalizedPaths, flags, priority);
  }

  // ==========================================================================
  // 属性与扩展属性 (XAttr) (补全接口)
  // ==========================================================================

  /**
   * 获取文件系统属性
   */
  getProperty(name: string): { success: boolean; value: string } {
    return this._internal.GetProperty(name);
  }

  /**
   * 设置文件系统属性
   */
  setProperty(name: string, value: string): boolean {
    return this._internal.SetProperty(name, value);
  }

  /**
   * 设置扩展属性
   * @param targetPath 目标路径
   * @param attrs 扩展属性键值对记录
   */
  async setXAttr(targetPath: string, attrs: Record<string, string>): Promise<XAttrStatusResult[]> {
    return this._internal.SetXAttr(this._normalize(targetPath), attrs);
  }

  /**
   * 获取扩展属性
   * @param targetPath 目标路径
   * @param keys 需要获取的属性名数组
   */
  async getXAttr(targetPath: string, keys: string[]): Promise<Record<string, string>> {
    return this._internal.GetXAttr(this._normalize(targetPath), keys);
  }

  /**
   * 删除指定的扩展属性
   * @param targetPath 目标路径
   * @param keys 需要删除的属性名数组
   */
  async delXAttr(targetPath: string, keys: string[]): Promise<XAttrStatusResult[]> {
    return this._internal.DelXAttr(this._normalize(targetPath), keys);
  }

  /**
   * 列出目标文件或目录的所有扩展属性
   */
  async listXAttr(targetPath: string): Promise<Record<string, string>> {
    return this._internal.ListXAttr(this._normalize(targetPath));
  }

}