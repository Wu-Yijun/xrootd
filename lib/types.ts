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
  id: string;             // GetId()
  size: bigint;           // GetSize()
  flags: number;          // GetFlags()
  modTime: number;        // GetModTime()
  accessTime: number;     // GetAccessTime()
  changeTime: number;     // GetChangeTime()
  
  // 字符串格式的时间和权限，方便 JS 层直接展示，不用自己转 format
  modTimeAsString: string;    // GetModTimeAsString()
  accessTimeAsString: string; // GetAccessTimeAsString()
  changeTimeAsString: string; // GetChangeTimeAsString()
  modeAsString: string;       // GetModeAsString()
  modeAsOctString: string;    // GetModeAsOctString()
  
  owner: string;          // GetOwner()
  group: string;          // GetGroup()
  checksum: string;       // GetChecksum()

  // helper functions
  get modeOctal(): string;  // 映射 C++ 的 GetModeAsString (返回 '0755')
  get modeString(): string; // 映射 C++ 的 GetModeAsOctString (返回 'rwxr-xr-x')
  get isFile(): boolean;
  get isDir(): boolean;
  get isOther(): boolean;
  get isOffline(): boolean;
  get isPOSCPending(): boolean;
  get isReadable(): boolean;
  get isWritable(): boolean;
  get isBackUpExists(): boolean;
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

export class XRootDError extends Error {
  code: number;
  status: number;
  constructor(status: IXRootDStatus) {
    super(status.message);
    this.code = status.code;
    this.status = status.status;
  }
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
  Clone(list: CloneLocationRequest[]): Promise<void>;
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
    PutInt(key: string, value: number): void;
    GetInt(key: string): number | null;
  };
}



export interface ReadStreamOptions {
  start?: bigint;
  end?: bigint;
  /**
   * 每次从底层读取的块大小 (默认 64KB)
   */
  highWaterMark?: number;
}

export interface WriteStreamOptions {
  start?: bigint;
}

//  克隆位置列表
export interface CloneLocationRequest {
  srcFile: File;       // 源文件实例（必须已被打开供读取）
  dstOffset: bigint | number;
  srcOffset: bigint | number;
  length: bigint | number;
}