import { defineConfig } from 'tsdown'

export default defineConfig({
  entry: ['lib/index.ts'],
  outDir: 'dist',
  format: ['cjs', 'esm'],
  dts: true,
  clean: true
})