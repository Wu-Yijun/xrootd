import { URL, Env, FileSystem, File, OpenFlags, AccessMode } from "../dist/index.mjs";

// const url = "root://eos01.ihep.ac.cn//eos/lhaaso/decode/km2a";
// const urlFull = "root://username:passowrd@eos01.ihep.ac.cn:5500//eos/lhaaso/decode/km2a/rmout.sh?q=1&k=2#anchor";

// const u = new URL(urlFull);
// console.log(u.hostName);

// Env.putInt('TimeoutResolution', 2);
// // 开启客户端底层 Debug 日志 (对排查连接问题非常有用)
// Env.putString('DebugLevel', 'Dump');

// // 获取配置检查
// const level = Env.getString('DebugLevel'); // 返回 'Dump'
// const notExist = Env.getInt('SomeUnknownKey'); // 返回 null

// console.log({ level, notExist, TimeoutResolution: Env.getInt("TimeoutResolution") });


Env.putString('DebugLevel', 'Dump');
const fs = new FileSystem("root://eos01.ihep.ac.cn/");
console.log(fs);
console.time("fs.stat");
const finfo = await fs.stat("/eos/lhaaso/decode/km2a/rmout.sh");
console.timeEnd("fs.stat");
console.log("fs.stat", finfo);
console.log("fs.stat", await fs.stat("/eos/lhaaso/"));
console.log("Done!");

const f = new File();
await f.open("root://eos01.ihep.ac.cn//eos/lhaaso/decode/km2a/rmout.sh");
console.time("f.read");
const data = await f.read(0, Number(finfo.size));
console.timeEnd("f.read");
console.log(data.length, data);
await f.close();
const txt = new TextDecoder().decode(data);
console.log(txt);


finfo.isOther

// await f.open("root://eos01.ihep.ac.cn//eos/lhaaso/decode/km2a/rmout", OpenFlags.Read, AccessMode.OR/*  */);

const path = "/eos/user/w/wuyijun1/";

// console.log(await fs.dirList("/eos/user/w/"));
// await fs.chmod(path + "test.txt", AccessMode.UR | AccessMode.UW | AccessMode.UX);

// console.log(await fs.deepLocate(path + "test.txt"));

console.log("exist", await fs.stat(path + "test.txt"));
console.log("exist", await fs.exists(path + "temp"));
console.log("ensureDir",await fs.ensureDir(path + "temp/dir"));
console.log(await fs.mkdir(path + "temp/dir/files"));
console.log(await fs.mv(path + "test.txt", path + "temp/dir/files"));
// await fs.rmdir(path + "temp/dir")