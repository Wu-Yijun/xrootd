// lib/types.ts

// ============================================================================
// 1. 公共数据结构 (Public Data Structures)
// ============================================================================

export type PropertyList = Record<string, string | number | boolean>;

export interface IXRootDStatus {
  status: number;
  code: number;
  message: string;
}

export interface StatInfo {
  id: bigint; // C++ 中的 long long 映射为 JS 的 bigint
  size: bigint;
  flags: number;
  modTime: number; // Unix timestamp
}

export interface LocationInfo {
  host: string;
  port: number;
  isWritable: boolean;
  type: "manager" | "server";
}

export interface CopyJob {
  source: string;
  target: string;
  force?: boolean;
}

export interface ReadChunkRequest {
  offset: bigint | number;
  size: number;
}


export interface StatVFSInfo { // TODO
  freeBlocks: bigint;
  totalBlocks: bigint;
  freeInodes: bigint;
  totalInodes: bigint;
  // ... 其他系统状态字段
}

// ============================================================================
// 2. 公共枚举与标志位 (Public Enums & Flags)
// 注意：这些枚举的值必须与 C++ XrdCl::OpenFlags 等头文件中的常量严格保持一致！
// ============================================================================

/**
 * XrdCl::OpenFlags - 文件打开选项
 * @see src/XProtocol/XProtocol.hh:XOpenRequestOption
 * @see src/XrdCl/XrdClFileSystem.hh:OpenFlags::Flags
 */
export enum OpenFlags {
  None = 0x0000,            // 无特殊选项
  Compress = 0x0001,        // 启用压缩数据读取；对于locate，返回唯一的主机
  Delete = 0x0002,          // 打开新文件，删除任何现有文件
  Force = 0x0004,           // 忽略文件使用规则；对于locate表示忽略网络依赖
  New = 0x0008,             // 仅当文件不存在时才打开
  Read = 0x0010,            // 仅以读取方式打开 (kXR_open_read)
  Update = 0x0020,          // 以读写方式打开 (kXR_open_updt)
  Refresh = 0x0080,         // 刷新文件位置的缓存信息，使NoWait失效
  MakePath = 0x0100,        // 如果父目录不存在，自动创建
  IntentDirList = 0x0400,   // 表示locate在dirlist操作的上下文中
  Replica = 0x0800,         // 该文件正在打开以创建副本
  POSC = 0x1000,            // 启用成功关闭时持久处理 (Persist On Successful Close)
  NoWait = 0x2000,          // 仅当不会导致等待时才打开；对于locate，尽快提供位置
  SeqIO = 0x4000,           // 文件将被顺序读取或写入
  Write = 0x8000,           // 仅以写入方式打开 (kXR_open_wrto)
  PrefName = 0x0100,        // 优先返回主机名响应，仅适用于FileSystem::Locate
  Dup = 0x00010000,         // 通过从另一个文件复制内容来打开文件 (kXR_dup<<16)
  Samefs = 0x00020000,      // 在与另一个文件相同的文件系统上打开文件 (kXR_samefs<<16)
}

/**
 * XrdCl::Access - 文件访问权限模式
 * @see src/XProtocol/XProtocol.hh:XOpenRequestMode
 * @see src/XrdCl/XrdClFileSystem.hh:Access::Mode
 */
export enum AccessMode {
  None = 0x000,  // 无权限
  UR = 0x100,    // 所有者可读 (User Read)
  UW = 0x080,    // 所有者可写 (User Write)
  UX = 0x040,    // 所有者可执行/可浏览 (User Execute/Browsable)
  GR = 0x020,    // 组可读 (Group Read)
  GW = 0x010,    // 组可写 (Group Write)
  GX = 0x008,    // 组可执行/可浏览 (Group Execute/Browsable)
  OR = 0x004,    // 其他用户可读 (Other Read)
  OW = 0x002,    // 其他用户可写 (Other Write)
  OX = 0x001,    // 其他用户可执行/可浏览 (Other Execute/Browsable)
}

/**
 * XrdCl::MkDirFlags - 目录创建选项
 * @see src/XrdCl/XrdClFileSystem.hh:MkDirFlags::Flags
 */
export enum MkDirFlags {
  None = 0x00,       // 无特殊选项
  MakePath = 0x01,   // 如果父目录不存在，创建整个目录树（类似于 mkdir -p）
}

// ============================================================================
// 3. 对内原生接口契约 (Internal Native Interfaces)
// 描述 C++ N-API 层暴露出来的原始对象。不向外导出给包使用者。
// ============================================================================

/**
 * 对应 src/core/XrdNodeFile.cc 中 Init 暴露的类
 */
export interface INativeFile {
  Open(url: string, flags: number, mode: number): Promise<void>;
  Close(): Promise<void>;
  Stat(): Promise<StatInfo>;
  Read(offset: bigint, size: number): Promise<Buffer>;
  Write(offset: bigint, buffer: Buffer): Promise<void>;
  Sync(): Promise<void>;
  Truncate(size: bigint): Promise<void>;
  IsOpen(): boolean; // 同步方法
  GetProperty(name: string): Promise<string>;
  SetProperty(name: string, value: string): Promise<void>;
  // 向量读写
  VectorRead(chunks: ReadChunkRequest[]): Promise<Buffer[]>;
  ReadChunks(chunks: ReadChunkRequest[]): Promise<Buffer[]>;

  // 扩展属性
  SetXAttr(name: string, value: string): Promise<void>;
  GetXAttr(name: string): Promise<string>;
  DelXAttr(name: string): Promise<void>;
  ListXAttr(): Promise<string[]>;

  // 克隆
  Clone(): any; // 返回底层的新 N-API 实例
}

/**
 * 对应 src/core/XrdNodeFileSystem.cc 中 Init 暴露的类
 */
export interface INativeFileSystem {
  Locate(path: string, flags: number): Promise<LocationInfo[]>;
  Stat(path: string): Promise<StatInfo>;
  Rm(path: string): Promise<void>;
  MkDir(path: string, mode: number): Promise<void>;
  RmDir(path: string): Promise<void>;
  Mv(source: string, dest: string): Promise<void>;
  Cat(path: string): Promise<Buffer>;
  DirList(path: string, flags: number): Promise<string[]>; // 简化为文件名数组
  DeepLocate(path: string, flags: number): Promise<LocationInfo[]>;
  Query(queryCode: number, args: string): Promise<string>;
  Truncate(path: string, size: bigint): Promise<void>;
  ChMod(path: string, mode: number): Promise<void>;
  Ping(): Promise<void>;
  StatVFS(path: string): Promise<StatVFSInfo>;
  Protocol(): Promise<PropertyList>;
  SendInfo(info: string): Promise<void>;
  Prepare(requests: string[], flags: number): Promise<PropertyList>;

  GetProperty(name: string): Promise<string>;
  SetProperty(name: string, value: string): Promise<void>;

  SetXAttr(path: string, name: string, value: string): Promise<void>;
  GetXAttr(path: string, name: string): Promise<string>;
  DelXAttr(path: string, name: string): Promise<void>;
  ListXAttr(path: string): Promise<string[]>;
}

/**
 * 对应 src/core/XrdNodeCopyProcess.cc 中 Init 暴露的类
 */
export interface INativeCopyProcess {
  AddJob(source: string, target: string): Promise<void>;
  Prepare(): Promise<void>;
  Run(): Promise<PropertyList>;
  CancelJob(jobNum: number): void;
  // 注入 JS 回调给 C++ 的 ThreadSafeFunction 使用
  SetEventListener(
    eventName: string,
    callback: (...args: any[]) => void,
  ): void;
}

/**
 * C++ 原生模块的入口对象总览
 */
export interface XrdNativeBindings {
  File: { new(): INativeFile };
  FileSystem: { new(url: string): INativeFileSystem };
  CopyProcess: { new(): INativeCopyProcess };
  Env: {
    PutString(key: string, value: string): void;
    GetString(key: string): string | null;
  };
}
