import type { IXRootDError } from "./types.ts";

export function reverseStr(str: string) {
    let reversed = '';
    // Loop backward and append characters
    for (let i = str.length - 1; i >= 0; i--) {
        reversed += str.charAt(i); // Or str[i]
    }
    return reversed;
}

export function isXrdError(e: unknown): e is IXRootDError {
    return e instanceof Error && "xrdStatus" in e && typeof e.xrdStatus === "number" && e.xrdStatus > 0;
}