// lib/native.ts
import nodeGypBuild from 'node-gyp-build';
import path from 'path';
import { XrdNativeBindings } from './types';

// __dirname 在编译后的 dist/ 目录下
// 所以 path.resolve(__dirname, '..') 指向项目的根目录
// nodeGypBuild 会自动去根目录下的 prebuilds/ 寻找匹配的文件
// 例如：prebuilds/linux-x64/node.napi.node
const nativeAddon: XrdNativeBindings = nodeGypBuild(path.resolve(__dirname, '..'));

export default nativeAddon;