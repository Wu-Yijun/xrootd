// lib/url.ts
import { URL as NodeURL } from 'node:url';

/**
 * 纯 TypeScript 实现的 XRootD URL 解析器
 * 安全、轻量，避免跨越 C++ N-API 边界
 */
export class XRootDUrl {
  private _url: NodeURL;
  public readonly isValid: boolean;

  constructor(urlString: string) {
    this._url = new NodeURL(urlString);
    this.isValid = true;
    // 强制补全 root:// 协议，方便 Node.js 原生 URL 解析
    if (!["root", "xrootd", "xroot", "file"].includes(this.protocol)) {
      console.error('Invalid XRootD URL protocol.');
      this.isValid = false;
    } else if (this.hostName.length === 0) {
      this.isValid = false;
    }
  }

  // 利用 Node 原生 URL 轻松实现 getters
  get protocol(): string { return this._url.protocol.replace(/:$/, ""); }
  set protocol(protocol: string) { this._url.protocol = protocol; }
  get hostName(): string { return this._url.hostname; }
  set hostName(hostName: string) { this._url.hostname = hostName; }
  get port(): number { return this._url.port ? parseInt(this._url.port, 10) : 1094; } // XRootD 默认端口 1094
  set port(port: number | string) { this._url.port = port.toString(); }
  get userName(): string { return this._url.username; }
  set userName(userName: string) { this._url.username = userName; }
  get password(): string { return this._url.password; }
  set password(password: string) { this._url.password = password; }
  get hostId(): string {
    const u = this._url.username;
    if (u.length === 0) {
      return this._url.host;
    } else if (this._url.password.length > 0) {
      return `${u}:${this._url.password}@${this._url.host}`;
    } else {
      return `${u}@${this._url.host}`;
    }
  }
  set pathWithParams(path: string) {
    const [p, s] = path.split("?", 2);
    this._url.pathname = "/" + p;
    this._url.search = s ?? "";
  } 
  get pathWithParams(): string {
    return this.path + this._url.search;
  }
  set searchParams(params: string) { this._url.search = params; }
  get searchParams(): string { return this._url.search; }
  set anchor(anchor: string) { this._url.hash = anchor; }
  get anchor(): string { return this._url.hash; }
  set path(path: string) {
    this._url.pathname = "/" + path;
  }
  // XRootD 特有的双斜杠路径处理
  get path(): string {
    // root://host//path -> pathname 会被解析为 //path -> 转化为 /path
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
      const u = new XRootDUrl(urlString);
      return u.isValid;
    } catch {
      return false;
    }
  }
}