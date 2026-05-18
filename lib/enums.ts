
// ============================================================================
// 2. 公共枚举与标志位 (Public Enums & Flags)
// 注意：这些枚举的值必须与 C++ XrdCl::OpenFlags 等头文件中的常量严格保持一致！
// ============================================================================

/**
 * XrdCl::OpenFlags - 文件打开选项
 * @see src/XProtocol/XProtocol.hh:XOpenRequestOption
 * @see src/XrdCl/XrdClFileSystem.hh:OpenFlags::Flags
 */
export const OpenFlags = {
    None: 0x0000,            // 无特殊选项
    Compress: 0x0001,        // 启用压缩数据读取；对于locate，返回唯一的主机
    Delete: 0x0002,          // 打开新文件，删除任何现有文件
    Force: 0x0004,           // 忽略文件使用规则；对于locate表示忽略网络依赖
    New: 0x0008,             // 仅当文件不存在时才打开
    Read: 0x0010,            // 仅以读取方式打开 (kXR_open_read)
    Update: 0x0020,          // 以读写方式打开 (kXR_open_updt)
    Refresh: 0x0080,         // 刷新文件位置的缓存信息，使NoWait失效
    MakePath: 0x0100,        // 如果父目录不存在，自动创建
    IntentDirList: 0x0400,   // 表示locate在dirlist操作的上下文中
    Replica: 0x0800,         // 该文件正在打开以创建副本
    POSC: 0x1000,            // 启用成功关闭时持久处理 (Persist On Successful Close)
    NoWait: 0x2000,          // 仅当不会导致等待时才打开；对于locate，尽快提供位置
    SeqIO: 0x4000,           // 文件将被顺序读取或写入
    Write: 0x8000,           // 仅以写入方式打开 (kXR_open_wrto)
    PrefName: 0x0100,        // 优先返回主机名响应，仅适用于FileSystem::Locate
    Dup: 0x00010000,         // 通过从另一个文件复制内容来打开文件 (kXR_dup<<16)
    Samefs: 0x00020000,      // 在与另一个文件相同的文件系统上打开文件 (kXR_samefs<<16)
} as const;
export type OpenFlags = typeof OpenFlags[keyof typeof OpenFlags];

/**
 * XrdCl::Access - 文件访问权限模式
 * @see src/XProtocol/XProtocol.hh:XOpenRequestMode
 * @see src/XrdCl/XrdClFileSystem.hh:Access::Mode
 */
export const AccessMode = {
    None: 0x000,  // 无权限
    UR: 0x100,    // 所有者可读 (User Read)
    UW: 0x080,    // 所有者可写 (User Write)
    UX: 0x040,    // 所有者可执行/可浏览 (User Execute/Browsable)
    GR: 0x020,    // 组可读 (Group Read)
    GW: 0x010,    // 组可写 (Group Write)
    GX: 0x008,    // 组可执行/可浏览 (Group Execute/Browsable)
    OR: 0x004,    // 其他用户可读 (World Read)
    OW: 0x002,    // 其他用户可写 (World Write)
    OX: 0x001,    // 其他用户可执行/可浏览 (World Execute/Browsable)
} as const;
export type AccessMode = number;

/**
 * XrdCl::MkDirFlags - 目录创建选项
 * @see src/XrdCl/XrdClFileSystem.hh:MkDirFlags::Flags
 */
export const MkDirFlags = {
    None: 0x00,       // 无特殊选项
    MakePath: 0x01,   // 如果父目录不存在，创建整个目录树（类似于 mkdir -p）
} as const;
export type MkDirFlags = typeof MkDirFlags[keyof typeof MkDirFlags];

// 在 lib/types.ts 导出枚举
export const StatFlags = {
    XBitSet: 1,   //!< Executable/searchable bit set
    IsDir: 2,   //!< This is a directory
    Other: 4,   //!< Neither a file nor a directory
    Offline: 8,   //!< File is not online (ie. on disk)
    POSCPending: 64,  //!< File opened with POST flag, not yet successfully closed
    IsReadable: 16,  //!< Read access is allowed
    IsWritable: 32,  //!< Write access is allowed
    BackUpExists: 128  //!< Back up copy exists
} as const;
export type StatFlags = typeof StatFlags[keyof typeof StatFlags];
