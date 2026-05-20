import nativeAddon from './native.ts';


type String = string;
type Int = number | BigInt;
type Bool = 'true' | 'false';


/**
 * XRootD 底层客户端环境变量配置
 * 参考自: XrdClConstants.hh
 */
export interface XRootDEnvConfig { // TODO: complete this with clear jsdoc
  // === 网络与超时配置 ===

  /**
   * 每个通道的子流数量。默认: 1
   */
  SubStreamsPerChannel?: Int;

  /**
   * 默认的请求超时时间 (秒)。
   * 注意: XRootD C++ 默认是 1800(30分钟)，本 Node.js 库初始化已重置为 30。
   */
  RequestTimeout?: Int;

  /**
   * 连接重试次数。默认: 5
   */
  ConnectionRetry?: Int;

  // === 性能与并行配置 ===

  /**
   * 内部工作线程数。默认: 3
   */
  WorkerThreads?: Int;

  /**
   * 数据读写恢复策略。默认: 'true'
   */
  ReadRecovery?: Bool;
  WriteRecovery?: Bool;

  // === 安全与插件配置 ===

  /**
   * 安全插件查找目录 (解决 Auth failed 的关键)。
   */
  SecPluginPath?: String;

  /**
   * 强制使用的认证协议，例如 'krb5,unix'
   */
  SecProtocol?: String;

  /**
   * 调试日志级别。例如 'Dump' 可以查看所有底层报文。默认: ''
   */
  DebugLevel?: 'Dump' | 'Debug' | 'Info' | 'Warning' | 'Error' | '';

  // ... 可以根据需要补充常量头文件里的其他字段

  // test_fail?: number | string;
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

class XRootDEnvironment {
  constructor() { }
  putInt(key: XRootDEnvConfigIntKey, intVal: number): void {
    nativeAddon.Env.PutInt(key, intVal);
  }
  putString(key: XRootDEnvConfigStringKey | XRootDEnvConfigBoolKey, strVal: string): void {
    nativeAddon.Env.PutString(key, strVal);
  }
  putBoolean(key: XRootDEnvConfigBoolKey, boolVal: boolean | Bool): void {
    if (typeof boolVal === "boolean") boolVal = boolVal ? "true" : "false";
    nativeAddon.Env.PutString(key, boolVal);
  }
  getInt(key: XRootDEnvConfigIntKey): number | undefined {
    return nativeAddon.Env.GetInt(key) ?? undefined;
  }
  getString(key: XRootDEnvConfigStringKey): string | undefined {
    return nativeAddon.Env.GetString(key) ?? undefined;
  }

  /**
   * 批量安全地设置 XRootD 底层参数
   */
  configure(config: XRootDEnvConfig): void {
    for (const [key, value] of Object.entries(config)) {
      if (value === undefined || value === null) continue;

      if (typeof value === 'number') {
        nativeAddon.Env.PutInt(key, value);
      } else if (typeof value === 'string') {
        nativeAddon.Env.PutString(key, value);
      } else if (typeof value === "boolean") {
        nativeAddon.Env.PutString(key, value ? "true" : "false");
      } else if (typeof value === "bigint") {
        nativeAddon.Env.PutInt(key, Number(value));
      } else {
        console.warn(`[xrootd] Unhandled type ${typeof value} for key ${key}.`);
        nativeAddon.Env.PutString(key, value.toString());
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
    return nativeAddon.Env.GetInt(key) ?? nativeAddon.Env.GetString(key) ?? undefined;
  }
}

export const Env = new XRootDEnvironment();

// --- 在库初始化时设置更符合 Node 习惯的默认值 ---
Env.configure({
  RequestTimeout: 30, // 替换掉底层的 1800 秒
  WorkerThreads: 4    // 适当提升并发处理能力
});