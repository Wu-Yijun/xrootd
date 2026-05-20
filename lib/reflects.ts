
interface TypeMap {
    "number": number;
    "bigint": bigint;
    "string": string;
    "boolean": boolean;
}

type TypeKey = keyof TypeMap | `${keyof TypeMap}?`;

export interface TypeDef {
    [key: string]: TypeKey;
}

export type ToType<T extends TypeDef> = {
    [K in keyof T as T[K] extends `${string}?` ? K : K]:
    T[K] extends `${infer Base}?`
    ? TypeMap[Base & keyof TypeMap] | undefined
    : TypeMap[T[K] & keyof TypeMap];
} & {};

export function parseStringRecord<T extends TypeDef>(type: T, obj: Record<keyof T, string>): ToType<T> {
    const result: any = {};
    for (const key in type) {
        if (!(key in obj)) continue;
        const value = obj[key];
        const typeName = type[key];
        switch (typeName) {
            case "number":
            case "number?":
                result[key] = Number(value);
                break;
            case "bigint":
            case "bigint?":
                result[key] = BigInt(value);
                break;
            case "string":
            case "string?":
                result[key] = value;
                break;
            case "boolean":
            case "boolean?":
                result[key] = Boolean(value);
                break;
            default:
            // cannot parsed
        }
    }
    return result;
}
