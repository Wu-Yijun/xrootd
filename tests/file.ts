import { File } from "../dist/index.mjs";

const f = new File();
f.open("");

(await f.listXAttr()).forEach((name) => {
    console.log(`Attr ${name}:`, f.getXAttr(name));
});

