// lib/types.ts

// ============================================================================
// 1. 公共数据结构 (Public Data Structures)
// ============================================================================

export type PropertyList = Record<string, string | number | boolean>;

interface IXRootDError extends Error {
  xrdStatus: number;
  xrdCode: number;
  xrdErrNo: number;
  xrdErrMsg: string;
}

export type XRootDOkError = { ok: true; } | ({ ok: false } & IXRootDError);

export type XAttrStatusResult = {
  ok: true;
  name: string;
} | ({
  ok: false;
  name: string;
} & IXRootDError);

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
  address: string;    // 例如 "host:port"
  type: number;       // XrdCl::LocationInfo::Location::Type 枚举值
  accessType: number; // XrdCl::Access::Type 枚举值
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


export interface StatVFSInfo {
  nodesRW: bigint;
  freeRW: bigint;
  utilizationRW: number;
  nodesStaging: bigint;
  freeStaging: bigint;
  utilizationStaging: number;
}

export interface DirListEntry {
  name: string;
  hostAddress: string;
  stat: StatInfo | null;
}


export class XRootDError extends Error {
  code: number;
  status: number;
  constructor(status: XRootDOkError) {
    if (status.ok) {
      super("throw Unexpected Ok as Errpr");
      this.code = -1;
      this.status = -1;
      return;
    }
    super(status.message);
    // TODO...
    this.code = status.xrdCode;
    this.status = status.xrdStatus;
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
  WriteFd(offset: bigint, size: number, fd: number, fdoff?: bigint): Promise<void>;
  Sync(): Promise<void>;
  Truncate(size: bigint): Promise<void>;
  IsOpen(): boolean; // 同步方法
  GetProperty(name: string): { success: boolean, value: string }; // [FIXED: 与 C++ 同步返回对齐]
  SetProperty(name: string, value: string): boolean; // [FIXED: 与 C++ 同步返回对齐]
  // 向量读写
  VectorRead(chunks: ReadChunkRequest[]): Promise<Buffer[]>;
  ReadChunks(chunks: ReadChunkRequest[]): Promise<Buffer[]>;

  // 扩展属性
  SetXAttr(attrs: Record<string, string>): Promise<XAttrStatusResult[]>; // [FIXED: 与 C++ FSXAttrStatusHandler 对齐]
  GetXAttr(keys: string[]): Promise<Record<string, string>>; // [FIXED: 与 C++ FSXAttrDataHandler 对齐]
  DelXAttr(keys: string[]): Promise<XAttrStatusResult[]>; // [FIXED: 与 C++ FSXAttrStatusHandler 对齐]
  ListXAttr(): Promise<Record<string, string>>; // [FIXED: 与 C++ FSXAttrDataHandler 对齐]

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
  MkDir(path: string, flags: number, mode: number): Promise<void>; // [FIXED: 补全 flags 参数]
  RmDir(path: string): Promise<void>;
  Mv(source: string, dest: string): Promise<void>;
  SendCache(info: string): Promise<Buffer>; // [FIXED: 移除了 C++ 中已废弃的 Cat，替换为 SendCache]
  DirList(path: string, flags: number): Promise<DirListEntry[]>; // [FIXED: 修正返回类型为 DirListEntry[]]
  DeepLocate(path: string, flags: number): Promise<LocationInfo[]>;
  Query(queryCode: number, args: Buffer): Promise<Buffer>; // [FIXED: 修正 args 与返回类型为 Buffer]
  Truncate(path: string, size: bigint | number): Promise<void>; // [FIXED: 增加 number 兼容]
  ChMod(path: string, mode: number): Promise<void>;
  Ping(): Promise<void>;
  StatVFS(path: string): Promise<StatVFSInfo>;
  Protocol(): Promise<PropertyList>;
  SendInfo(info: string): Promise<Buffer>; // [FIXED: 修正返回类型为 Buffer]
  Prepare(requests: string[], flags: number, priority: number): Promise<Buffer>; // [FIXED: 补全 priority 参数，修正返回类型为 Buffer]

  GetProperty(name: string): { success: boolean, value: string }; // [FIXED: 修正为同步返回对象]
  SetProperty(name: string, value: string): boolean; // [FIXED: 修正为同步返回 boolean]

  SetXAttr(path: string, attrs: Record<string, string>): Promise<XAttrStatusResult[]>; // [FIXED: 修正入参与返回结构]
  GetXAttr(path: string, keys: string[]): Promise<Record<string, string>>; // [FIXED: 修正入参与返回结构]
  DelXAttr(path: string, keys: string[]): Promise<XAttrStatusResult[]>; // [FIXED: 修正入参与返回结构]
  ListXAttr(path: string): Promise<Record<string, string>>; // [FIXED: 修正返回结构]
}


export interface CopyJobConfig {
  /** original source URL */
  source: string,
  /** target directory or file */
  target: string,
  /** maximum number sources */
  sourceLimit?: number,
  /** overwrite target if exists */
  force?: boolean,
  /** persistify only on successful close */
  posc?: boolean,
  /** ignore locking semantics on destination */
  coerce?: boolean,
  /** create path to the file if it doesn't exist */
  makeDir?: boolean,
  /** "first" try third party copy, if it fails try normal copy; "only" only try third party copy */
  thirdParty?: string,
  /** "none"    - no checksumming
      "end2end" - end to end checksumming
      "source"  - calculate checksum at source
      "target"  - calculate checksum at target */
  checkSumMode?: "none" | "end2end" | "source" | "target",
  /** type of the checksum to be used */
  checkSumType?: string,
  /** checksum preset */
  checkSumPreset?: string,
  /** size of a copy chunks in bytes */
  chunkSize?: number,
  /** number of chunks that should be requested in parallel
   * 
   * [uint8_t]
   */
  parallelChunks?: number,
  /** time limit for successfull initialization of the copy job
   * [time_t]
   */
  initTimeout?: number,
  /** time limit for the actual copy to finish
   * [time_t]
   */
  tpcTimeout?: number,
  /** support for the case where the size source file may change during reading process */
  dynamicSource?: boolean,
};

export interface CopyJobResult {
  /** checksum at source, if requested */
  sourceCheckSum?: string,
  /** checksum at target, if requested */
  targetCheckSum?: string,
  /** file size */
  size: bigint,
  /** status of the copy operation */
  status?: XRootDOkError,
  /** all sources used */
  sources?: string[],
  /** the actual disk server target */
  realTarget?: string,
};

export type ProgressCallback = (jobNum: number, processed: number, total: number) => void;

/**
 * 对应 src/core/XrdNodeCopyProcess.cc 中 Init 暴露的类
 */
export interface INativeCopyProcess {
  AddJob(config: CopyJobConfig): void;
  Prepare(): Promise<void>;
  Run(): Promise<CopyJobResult[]>;
  CancelJob(): void;
  // 注入 JS 回调给 C++ 的 ThreadSafeFunction 使用
  SetEventListener(eventName: string, callback: ProgressCallback): void;
}

/**
 * C++ 原生模块的入口对象总览
 */
export interface XrdNativeBindings {
  File: { new(): INativeFile };
  FileSystem: { new(url: string): INativeFileSystem };
  CopyProcess: { new(): INativeCopyProcess };
  Env: {
    PutString(key: string, value: string): boolean;
    GetString(key: string): string | null;
    PutInt(key: string, value: number): boolean;
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