// lib/index.ts

// 1. 导出核心功能类 (使用者将通过 new File() 来调用)
export { File } from './file.ts';
export { FileSystem } from './filesystem.ts';
export { CopyProcess } from './copy.ts';
export { XRootDUrl as URL } from './url.ts';
export { Env } from './env.ts';

// 2. 导出公共枚举和接口 (使用者在传参或处理返回数据时需要用到)
// 注意：不要使用 export *，以防止把内部的 INativeFile 等接口泄露出去
export {
  OpenFlags,
  AccessMode,
  MkDirFlags,
} from './enums.ts';

export type {
  StatInfo,
  LocationInfo,
  StatVFSInfo,
  PropertyList,
  ReadChunkRequest,
  ReadStreamOptions,
  WriteStreamOptions,
  XRootDError,
} from './types.ts';