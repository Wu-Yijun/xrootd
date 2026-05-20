import nativeAddon from './native.ts';

type String = string;
type Int = number | bigint;
type Bool = 'true' | 'false';

/**
 * XRootD 底层客户端环境变量配置
 * 参考自: XrdClConstants.hh 与 XrdClDefaultEnv.cc
 */
export interface XRootDEnvConfig {
  // === 基础与连接控制 (Basic & Connection Control) ===

  /**
   * 每个通道的子流数量，用于多路复用并行传输。默认值: 1
   */
  SubStreamsPerChannel?: Int;

  /**
   * 默认的请求超时时间 (秒)。
   * 注意: XRootD C++ 默认是 1800（30 分钟），本 Node.js 库初始化时重置为 30。
   */
  RequestTimeout?: Int;

  /**
   * 连接重试次数。默认值: 5
   */
  ConnectionRetry?: Int;

  /**
   * 连接窗口时间 (秒)。如果在该窗口内没有成功建立连接，则报错。默认值: 120
   */
  ConnectionWindow?: Int;

  /**
   * 流超时时间 (秒)。默认值: 60
   */
  StreamTimeout?: Int;

  /**
   * 超时检测的时间精度/分辨率 (秒)。默认值: 15
   */
  TimeoutResolution?: Int;

  /**
   * 流错误统计的时间窗口长度 (秒)。默认值: 1800
   */
  StreamErrorWindow?: Int;

  /**
   * 是否启用多进程 fork 处理器，用于管理多线程 fork 同步。默认值: 1
   */
  RunForkHandler?: Int;

  /**
   * 重定向的最大限制次数。默认值: 16
   */
  RedirectLimit?: Int;

  // === 性能与线程配置 (Performance & Threading) ===

  /**
   * 内部工作线程数。默认值: 3。注意：本 Node.js 库初始化时重置为 4。
   */
  WorkerThreads?: Int;

  /**
   * 并发运行的事件循环数量。默认值: 10
   */
  ParallelEvtLoop?: Int;

  /**
   * 是否启用 TCP_NODELAY (禁用 Nagle 算法)。默认值: Linux 下为 1，OSX 下为 0
   */
  NoDelay?: Int;

  /**
   * 异步 I/O 通知使用的信号（0 表示不使用）。默认值: 0
   */
  AioSignal?: Int;

  // === 复制传输配置 (Copy Process Settings) ===

  /**
   * Copy 复制过程所使用的基本分块大小 (字节)。默认值: 8388608 (8MB)
   */
  CPChunkSize?: Int;

  /**
   * Copy 过程中同时并发传输的分块数量。默认值: 4
   */
  CPParallelChunks?: Int;

  /**
   * Copy 任务的初始化阶段超时时间 (秒)。默认值: 600
   */
  CPInitTimeout?: Int;

  /**
   * 第三方复制 (Third Party Copy, TPC) 过程超时时间 (秒)。默认值: 1800
   */
  CPTPCTimeout?: Int;

  /**
   * 整个 Copy 复制任务的总体超时时间 (秒，0 表示不限制)。默认值: 0
   */
  CPTimeout?: Int;

  /**
   * 扩展 Copy 的单次写入最大块大小 (字节)。默认值: 134217728 (128MB)
   */
  XCpBlockSize?: Int;

  /**
   * 写入被引导至负载均衡器时的重试次数上限。默认值: 3
   */
  RetryWrtAtLBLimit?: Int;

  /**
   * Copy 任务的重试次数。默认值: 0
   */
  CpRetry?: Int;

  /**
   * 在 Copy 复制传输时，是否启用渐进式读写。默认值: 1
   */
  CpUsePgWrtRd?: Int;

  /**
   * 复制文件时是否保留扩展属性 (XAttrs)。默认值: 0
   */
  PreserveXAttrs?: Int;

  /**
   * Copy 任务失败后的重试策略。默认值: "force"
   */
  CpRetryPolicy?: String;

  /**
   * Copy 操作的目标类型。默认值: ""
   */
  CpTarget?: String;

  // === TCP Keep-Alive 配置 ===

  /**
   * 是否启用 TCP Keep-Alive。默认值: 0
   */
  TCPKeepAlive?: Int;

  /**
   * TCP 空闲多长时间后发送首次 Keep-Alive 探测包 (秒)。默认值: 7200
   */
  TCPKeepAliveTime?: Int;

  /**
   * 两次 TCP Keep-Alive 探测包 of 发送间隔时间 (秒)。默认值: 75
   */
  TCPKeepAliveInterval?: Int;

  /**
   * 连接断开前，最大允许的 TCP Keep-Alive 探测未响应次数。默认值: 9
   */
  TCPKeepAliveProbes?: Int;

  // === TTL 与负载均衡配置 ===

  /**
   * 数据服务器连接的生存时间 (TTL，秒)。默认值: 300
   */
  DataServerTTL?: Int;

  /**
   * 负载均衡器连接的生存时间 (TTL，秒)。默认值: 1200
   */
  LoadBalancerTTL?: Int;

  /**
   * 解析 IP 后是否禁止打乱顺序 (打乱顺序有利于负载均衡)。默认值: 0
   */
  IPNoShuffle?: Int;

  /**
   * 定位端点时是否记录并排除已尝试失败的地址。默认值: 1
   */
  PreserveLocateTried?: Int;

  // === 网络协议栈与域名解析 ===

  /**
   * DNS 解析时是否优先选择 IPv4 而非 IPv6。默认值: 0
   */
  PreferIPv4?: Int;

  /**
   * 首选的 Poller 轮询器实现。默认值: "built-in"
   */
  PollerPreference?: String;

  /**
   * 使用的网络协议栈 (如 "IPAuto", "IPv4", "IPv6")。默认值: "IPAuto"
   */
  NetworkStack?: String;

  // === 错误恢复策略 (Recovery Settings) ===

  /**
   * 读数据失败时的恢复策略。默认值: "true"
   */
  ReadRecovery?: Bool;

  /**
   * 写数据失败时的恢复策略。默认值: "true"
   */
  WriteRecovery?: Bool;

  /**
   * 打开文件失败时的恢复策略。默认值: "true"
   */
  OpenRecovery?: Bool;

  // === Metalink 相关配置 ===

  /**
   * 是否启用 Metalink 解析和处理。默认值: 1
   */
  MetalinkProcessing?: Int;

  /**
   * 是否允许解析本地存储 of Metalink 文件。默认值: 0
   */
  LocalMetalinkFile?: Int;

  /**
   * 等待 Metalink 下载的最大时间 (秒)。默认值: 60
   */
  MaxMetalinkWait?: Int;

  /**
   * 是否对 Zip 文件的 Metalink 执行校验和检查。默认值: 0
   */
  ZipMtlnCksum?: Int;

  // === 协议与安全控制 (Security & TLS) ===

  /**
   * 强制使用的安全协议，例如 "unix" 或 "krb5,unix"（底层通过系统环境变量设置）。
   */
  SecProtocol?: String; // TODO: List All options

  /**
   * 未授权 (Permission Denied) 时，触发自动刷新证书并重试的次数上限。默认值: 3
   */
  NotAuthorizedRetryLimit?: Int;

  /**
   * 如果 TLS 握手失败，是否允许降级为非 TLS 明文传输。默认值: 0
   */
  NoTlsOK?: Int;

  /**
   * 是否只对控制通道加密（控制通道加密，数据通道明文）。默认值: 0
   */
  TlsNoData?: Int;

  /**
   * 获取 Metalink 配置文件时是否启用 TLS。默认值: 0
   */
  TlsMetalink?: Int;

  /**
   * 是否在访问非渐进式文件时要求 TLS。默认值: 0
   */
  WantTlsOnNoPgrw?: Int;

  /**
   * TLS 调试级别（如 "OFF", "INFO", "DEBUG"）。默认值: "OFF"
   */
  TlsDbgLvl?: String;

  /**
   * 是否启用多协议通道支持。默认值: 0
   */
  MultiProtocol?: Int;

  // === 日志与调试配置 ===

  /**
   * 日志文件路径。底层拦截设置，直接对 C++ logger 生效。默认值: ""
   */
  LogFile?: String;

  /**
   * 调试日志级别（如 "Dump", "Debug", "Info", "Warning", "Error", ""）。底层拦截设置。
   */
  DebugLevel?: 'Dump' | 'Debug' | 'Info' | 'Warning' | 'Error' | '';

  /**
   * 日志掩码，指定哪些子系统输出日志。底层拦截设置。
   */
  LogMask?: String;

  // === 插件与应用级配置 ===

  /**
   * 客户端配置文件加载目录。默认值: ""
   */
  ClConfDir?: String;

  /**
   * 默认客户端配置文件的绝对路径。默认值: ""
   */
  DefaultClConfFile?: String;

  /**
   * 安全或传输插件配置文件的加载目录。默认值: ""
   */
  PlugInConfDir?: String;

  /**
   * 指定动态加载的插件名称。默认值: ""
   */
  PlugIn?: String;

  /**
   * 外部客户端监控插件动态链接库的加载路径。默认值: ""
   */
  ClientMonitor?: String;

  /**
   * 传递给外部客户端监控插件的参数字符串。默认值: ""
   */
  ClientMonitorParam?: String;

  /**
   * 应用的标识名称。默认值: 当前可执行文件名
   */
  AppName?: String;

  /**
   * 监控上报信息内容。默认值: ""
   */
  MonInfo?: String;
}

type KeysOfValue<T, TValue> = {
  [K in keyof T]: T[K] extends TValue ? K : never;
}[keyof T];

export type XRootDEnvConfigIntKey = KeysOfValue<Required<XRootDEnvConfig>, Int>;
export type XRootDEnvConfigBoolKey = KeysOfValue<Required<XRootDEnvConfig>, Bool>;
export type XRootDEnvConfigStringKey = KeysOfValue<Required<XRootDEnvConfig>, String>;
export type XRootDEnvConfigOtherKey = Exclude<keyof XRootDEnvConfig, XRootDEnvConfigIntKey | XRootDEnvConfigBoolKey | XRootDEnvConfigStringKey>;

type _AssertNever<T extends never> = T;
type _Check = _AssertNever<XRootDEnvConfigOtherKey>;

function checkIntRange(key: string, value: number | bigint): void {
  const num = Number(value);
  if (num < -2147483648 || num > 2147483647) {
    console.warn(`[xrootd] Warning: value ${value} for key "${key}" is out of 32-bit signed integer range [-2147483648, 2147483647]. It will be truncated.`);
  }
}

class XRootDEnvironment {
  constructor() { }

  /**
   * 设置整数配置项，返回是否设置成功。
   */
  putInt(key: XRootDEnvConfigIntKey, intVal: number | bigint): boolean {
    checkIntRange(key, intVal);
    const success = nativeAddon.Env.PutInt(key, Number(intVal));
    if (!success) {
      console.warn(`[xrootd] Warning: Failed to set integer configuration "${key}"=${intVal}. It might have been overridden by a system environment variable.`);
    }
    return success;
  }

  /**
   * 设置字符串配置项，返回是否设置成功。
   */
  putString(key: XRootDEnvConfigStringKey | XRootDEnvConfigBoolKey, strVal: string): boolean {
    if (key === 'SecProtocol') {
      process.env.XrdSecPROTOCOL = strVal;
      process.env.XRD_SECPROTOCOL = strVal;
      return true;
    }
    const success = nativeAddon.Env.PutString(key, strVal);
    if (!success) {
      console.warn(`[xrootd] Warning: Failed to set string configuration "${key}"="${strVal}". It might have been overridden by a system environment variable.`);
    }
    return success;
  }

  /**
   * 设置布尔配置项，返回是否设置成功。
   */
  putBoolean(key: XRootDEnvConfigBoolKey, boolVal: boolean | Bool): boolean {
    if (typeof boolVal === "boolean") boolVal = boolVal ? "true" : "false";
    return this.putString(key, boolVal);
  }

  /**
   * 获取整数配置项。
   */
  getInt(key: XRootDEnvConfigIntKey): number | undefined {
    return nativeAddon.Env.GetInt(key) ?? undefined;
  }

  /**
   * 获取字符串配置项。
   */
  getString(key: XRootDEnvConfigStringKey): string | undefined {
    if (key === 'SecProtocol') {
      return process.env.XrdSecPROTOCOL ?? process.env.XRD_SECPROTOCOL ?? undefined;
    }
    return nativeAddon.Env.GetString(key) ?? undefined;
  }

  /**
   * 批量安全地设置 XRootD 底层参数
   */
  configure(config: XRootDEnvConfig): void {
    for (const [key, value] of Object.entries(config)) {
      if (value === undefined || value === null) continue;

      let success = true;
      if (key === 'SecProtocol') {
        process.env.XrdSecPROTOCOL = String(value);
        process.env.XRD_SECPROTOCOL = String(value);
        continue;
      }

      if (typeof value === 'number' || typeof value === 'bigint') {
        checkIntRange(key, value);
        success = nativeAddon.Env.PutInt(key, Number(value));
      } else if (typeof value === 'string') {
        success = nativeAddon.Env.PutString(key, value);
      } else if (typeof value === 'boolean') {
        success = nativeAddon.Env.PutString(key, value ? "true" : "false");
      } else {
        console.warn(`[xrootd] Unhandled type ${typeof value} for key ${key}.`);
        success = nativeAddon.Env.PutString(key, value.toString());
      }

      if (!success) {
        console.warn(`[xrootd] Warning: Failed to set configuration "${key}"=${value}. It might have been overridden by a system environment variable.`);
      }
    }
  }

  /**
   * 获取指定配置
   */
  get(key: XRootDEnvConfigBoolKey): string | undefined;
  get(key: XRootDEnvConfigStringKey): string | undefined;
  get(key: XRootDEnvConfigIntKey): number | undefined;
  get(key: keyof XRootDEnvConfig): string | number | undefined {
    if (key === 'SecProtocol') {
      return process.env.XrdSecPROTOCOL ?? process.env.XRD_SECPROTOCOL ?? undefined;
    }
    return nativeAddon.Env.GetInt(key as any) ?? nativeAddon.Env.GetString(key as any) ?? undefined;
  }
}

export const Env = new XRootDEnvironment();

// --- 在库初始化时设置更符合 Node 习惯的默认值 ---
Env.configure({
  RequestTimeout: 30, // 替换掉底层的 1800 秒
  WorkerThreads: 4    // 适当提升并发处理能力
});