// lib/index.ts

// 1. 导出核心功能类 (使用者将通过 new File() 来调用)
export { File } from './file';
export { FileSystem } from './filesystem';
// export { CopyProcess } from './copy-process';
// export { URL } from './url';
// export { Env } from './env';

// 2. 导出公共枚举和接口 (使用者在传参或处理返回数据时需要用到)
// 注意：不要使用 export *，以防止把内部的 INativeFile 等接口泄露出去
export {
  OpenFlags,
  AccessMode,
  MkDirFlags,
  XRootDError,
} from './types';

export type {
  StatInfo,
  LocationInfo,
  StatVFSInfo,
  PropertyList,
  ReadChunkRequest,
  ReadStreamOptions,
  WriteStreamOptions
} from './types';