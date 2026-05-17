// lib/url.ts
import { URL as NodeURL } from 'url';

/**
 * 纯 TypeScript 实现的 XRootD URL 解析器
 * 安全、轻量，避免跨越 C++ N-API 边界
 */
export class XRootDUrl {
  private _url: NodeURL;

  constructor(urlString: string) {
    // 强制补全 root:// 协议，方便 Node.js 原生 URL 解析
    if (!urlString.startsWith('root://') && !urlString.startsWith('xroot://')) {
        throw new Error('Invalid XRootD URL protocol.');
    }
    this._url = new NodeURL(urlString);
  }

  // 利用 Node 原生 URL 轻松实现 getters
  get protocol(): string { return this._url.protocol; }
  get hostName(): string { return this._url.hostname; }
  get port(): number { return this._url.port ? parseInt(this._url.port, 10) : 1094; } // XRootD 默认端口 1094
  get userName(): string { return this._url.username; }
  get password(): string { return this._url.password; }
  
  // XRootD 特有的双斜杠路径处理
  get path(): string {
    // root://host//path -> pathname 会被解析为 //path
    return this._url.pathname.replace(/^\/+/, '/'); 
  }

  // 处理 XRootD 的 CGI 参数 (Opaque Info)
  getParams(): Record<string, string> {
    const params: Record<string, string> = {};
    for (const [key, value] of this._url.searchParams.entries()) {
      params[key] = value;
    }
    return params;
  }

  // 生成最终传给 C++ 的安全字符串
  toString(): string {
    return this._url.toString();
  }

  // 纯静态验证
  static isValid(urlString: string): boolean {
    try {
      new XRootDUrl(urlString);
      return true;
    } catch {
      return false;
    }
  }
}