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

// import { execSync } from "node:child_process";
import { createConnection } from "node:net";

const PORT = 32874;
export const SERVER_URL = `root://localhost:${PORT}/`;
export const PWD = import.meta.dirname;
export const smallfile = SERVER_URL + PWD + '/tmp/spam';
export const smallcopy = SERVER_URL + PWD + '/tmp/eggs';
export const smallbuffer = 'gre\0en\neggs\nand\nham\n';
export const bigfile = SERVER_URL + PWD + '/tmp/bigfile';
export const bigcopy = SERVER_URL + PWD + '/tmp/bigcopy';

export async function testAlive(port = PORT) {
  return new Promise((a, r) => {
    const c = setTimeout(() => {
      console.error("Connection Timeout!");
      a(false);
    }, 15000);
    const socket = createConnection(port, "localhost", () => {
      console.log(`✅ Success: XRootD server is actively hosting on port ${port}`);
      socket.end(); // Close the connection cleanly
      clearTimeout(c);
      a(true);
    });
    let success = true;
    socket.on('error', (err) => {
      console.error(`❌ Failure: Could not connect to port ${port}. Is the server running?`);
      console.error(err);
      clearTimeout(c);
      a(false);
    });
  });
}
