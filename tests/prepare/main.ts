import { mkdirSync, writeFileSync } from "node:fs";
import { generateRandomFile } from "./dump.ts";
const PWD = import.meta.dirname + "/..";


mkdirSync(PWD + "/tmp", { recursive: true });
writeFileSync(PWD + "/tmp/hello.txt", "Hello, world!");

generateRandomFile({
    size: 2e6,
    outputPath: PWD + "/tmp/eggs",
    binary: false,
    chunkSize: 256000,
});
generateRandomFile({
    size: 2e7,
    outputPath: PWD + "/tmp/spam",
    binary: false,
    chunkSize: 1024 * 1024,
});
generateRandomFile({
    size: 2e8,
    outputPath: PWD + "/tmp/bigfile.bin",
    binary: false,
    chunkSize: 4 * 1024 * 1024,
});