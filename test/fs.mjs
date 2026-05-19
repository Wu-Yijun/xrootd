/// <reference types="node" />

import assert from "node:assert/strict";
import { test } from 'node:test';
import { copyFileSync } from 'node:fs';
import { AccessMode, FileSystem, MkDirFlags, OpenFlags } from "../dist/index.mjs";
import { PWD, SERVER_URL, testAlive } from "./env.mjs";

// if (!await testAlive()) process.exit(1);

const TMP = PWD + "/tmp";

test("test_filesystem", async () => {
    const fs = new FileSystem(SERVER_URL);
    console.log("fs.locate", await fs.locate(TMP, OpenFlags.Refresh));
    console.log("fs.deeplocate", await fs.deepLocate(TMP, OpenFlags.Refresh));
    // console.log("fs.query", await fs.query(QueryCode.SPACE, '/tmp')); // TODO
    console.log("fs.truncate", await fs.truncate(TMP + "/spam", 200));
    console.log("fs.move", await fs.mv( TMP + "/spam", TMP + "/ham"));
    console.log("fs.chmod", await fs.chmod( TMP + "/ham", AccessMode.UR | AccessMode.UW ));
    console.log("fs.rm", await fs.rm(TMP + "/ham"));
    console.log("fs.mkdir", await fs.mkdir(TMP + "/somedir", MkDirFlags.MakePath));
    console.log("fs.rmdir", await fs.rmdir(TMP + "/somedir"));
    console.log("fs.ping", await fs.ping());
    console.log("fs.stat", await fs.stat(TMP));
    console.log("fs.statVFS", await fs.statVFS(TMP));
    console.log("fs.protocol", await fs.protocol());
    console.log("fs.dirList", await fs.dirList(TMP));
    console.log("fs.sendInfo", await fs.sendInfo("[NOTICE] important info"));
    console.log("fs.prepare", await fs.prepare([TMP + "/bigfile.bin"]));
});