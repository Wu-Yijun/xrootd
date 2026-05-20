import { File } from "xrootd";

const f = new File();
f.open("");

(await f.listXAttr()).forEach((name) => {
    console.log(`Attr ${name}:`, f.getXAttr(name));
});

