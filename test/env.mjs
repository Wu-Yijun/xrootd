/// <reference types="node" />

// =====================================
//        xrootd python test
// =====================================
// SERVER_URL  = 'root://localhost/'
// smallfile   = SERVER_URL + '/tmp/spam'
// smallcopy   = SERVER_URL + '/tmp/eggs'
// smallbuffer = 'gre\0en\neggs\nand\nham\n'
// bigfile     = SERVER_URL + '/tmp/bigfile'
// bigcopy     = SERVER_URL + '/tmp/bigcopy'

import { execSync } from "node:child_process";

const PORT = 32874;
export const SERVER_URL = `root://localhost:${PORT}/`;
export const PWD = import.meta.dirname;
export const smallfile = SERVER_URL + PWD + '/tmp/spam';
export const smallcopy = SERVER_URL + PWD + '/tmp/eggs';
export const smallbuffer = 'gre\0en\neggs\nand\nham\n';
export const bigfile = SERVER_URL + PWD + '/tmp/bigfile';
export const bigcopy = SERVER_URL + PWD + '/tmp/bigcopy';

export function testAlive(port = PORT) {
    try {
        execSync(`nc -zv localhost 32874`)
        return true;
    } catch (e) {
        console.log(`[ERROR] Could Not connect to localhost:${port}!`);
        console.error(e.message);
    }
    return false;
}
