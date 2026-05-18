import { URL, Env, FileSystem, File } from "../dist/index.mjs";

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


Env.putString('DebugLevel', 'Dump');
const fs = new FileSystem("root://eos01.ihep.ac.cn/");
console.log(fs);
const finfo = await fs.stat("/eos/lhaaso/decode/km2a/rmout.sh");
console.log("fs.stat", finfo);
console.log("fs.stat", await fs.stat("/eos/lhaaso/"));
console.log("Done!");

const f = new File();
await f.open("root://eos01.ihep.ac.cn//eos/lhaaso/decode/km2a/rmout.sh");
const data = await f.read(0, Number(finfo.size));
console.log(data);
f.close();

const txt = new TextDecoder().decode(data);
console.log(txt);