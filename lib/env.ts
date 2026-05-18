import nativeAddon from './native.ts';

export const Env = {
  putInt: nativeAddon.Env.PutInt,
  putString: nativeAddon.Env.PutString,
  getInt: nativeAddon.Env.GetInt,
  getString: nativeAddon.Env.GetString,
} as const;