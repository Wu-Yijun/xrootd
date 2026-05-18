import { URL, Env, FileSystem } from "../dist/index.mjs";

const url = "root://eos01.ihep.ac.cn//eos/lhaaso/decode/km2a";
const urlFull = "root://username:passowrd@eos01.ihep.ac.cn:5500//eos/lhaaso/decode/km2a/rmout.sh?q=1&k=2#anchor";

const u = new URL(urlFull);
console.log(u.hostName);

Env.putInt('TimeoutResolution', 2);
// 开启客户端底层 Debug 日志 (对排查连接问题非常有用)
Env.putString('DebugLevel', 'Dump');

// 获取配置检查
const level = Env.getString('DebugLevel'); // 返回 'Dump'
const notExist = Env.getInt('SomeUnknownKey'); // 返回 null

console.log({ level, notExist, TimeoutResolution: Env.getInt("TimeoutResolution") });



const fs = new FileSystem("root://eos01.ihep.ac.cn/");
console.log(fs);
console.log(await fs.dirList("/eos/lhaaso/decode/km2a"));