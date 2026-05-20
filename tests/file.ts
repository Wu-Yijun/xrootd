import { Env, File, FileSystem, OpenFlags } from "xrootd";
import { readFile } from "node:fs/promises";
import { test } from 'node:test';
import { strictEqual, deepStrictEqual } from "node:assert";
import { pipeline } from "node:stream/promises"
import { createGzip, gunzipSync, gzipSync } from 'node:zlib';
import { PWD, SERVER_URL } from "./env.mjs";

Env.putString("DebugLevel", "Dump");

test("test_file_operation", async () => {
    for (let i = 0; i < 3; i++) {
        const fs = new FileSystem(SERVER_URL);
        console.log(await fs.ping());

        const SRC = PWD + "/tmp/eggs";
        const DST = PWD + "/tmp/eggs.zip";
        const CPY = PWD + "/tmp/eggs.copy";

        const f = new File();
        await f.open(SRC, OpenFlags.Update);

        if (i > 0) {
            Env.putString("DebugLevel", "Error");
            const size = Number((await f.stat()).size);
            console.log("size", size);
            console.log("f.truncate", await f.truncate(Math.floor(size * 0.95))); // trim file !
        }

        console.log(await f.listXAttrs());
        const r = f.createReadStream();

        if (await fs.exists(DST)) {
            await fs.rm(DST);
            console.log("Delete DST:", !await fs.exists(DST));
        }
        if (await fs.exists(CPY)) {
            await fs.rm(CPY);
            console.log("Delete CPY:", !await fs.exists(CPY));
        }

        const d = new File();
        await d.open(DST, OpenFlags.New | OpenFlags.Write);
        const w = d.createWriteStream();
        try {
            await pipeline(r, createGzip(), w);
            console.log("piped ok");
        } catch (e) {
            console.log("piped fail");
            console.error(e);
        }

        await f.close();
        await d.close();

        await f.open(DST, OpenFlags.Read);
        await d.open(CPY, OpenFlags.New | OpenFlags.Write);

        const stat = await f.stat();
        console.log(stat);

        const buf = await f.read(0, Number(stat.size));
        const buf_raw = gunzipSync(buf);
        await d.write(0, buf_raw);

        console.log("Write Ok");

        await f.close();
        await d.close();

        const egg_0 = await readFile("tests/tmp/eggs", "utf-8");
        const egg_zip_0 = await readFile("tests/tmp/eggs.zip");
        const egg_1 = await readFile("tests/tmp/eggs.copy", "utf-8");
        const egg_zip_1 = gzipSync(egg_0);

        console.log(strictEqual(egg_0, egg_1) ?? "Raw file compare ok");
        console.log(deepStrictEqual(egg_zip_0.buffer, egg_zip_1.buffer) ?? "Zip file compare ok");
    }
});