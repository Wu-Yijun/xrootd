// lib/copy.ts
import nativeAddon from './native.ts';
import type { CopyJobConfig, CopyJobResult, INativeCopyProcess, ProgressCallback } from './types.ts';




export class CopyProcess {
    private nativeCp: INativeCopyProcess;

    constructor() {
        this.nativeCp = new nativeAddon.CopyProcess();
    }

    /**
     * Add a job to the copy process synchronously
     */
    addJob(config: CopyJobConfig): void {
        this.nativeCp.AddJob(config);
    }

    /**
     * Asynchronously prepare the jobs (resolve DNS, check endpoints, etc.)
     */
    async prepare(): Promise<void> {
        return this.nativeCp.Prepare();
    }

    /**
     * Run the copy process.
     * @param onProgress Optional progress callback
     * @returns Array of results corresponding to the jobs added
     */
    async run(onProgress?: ProgressCallback): Promise<CopyJobResult[]> {
        if (onProgress) {
            this.nativeCp.SetEventListener('progress', onProgress);
        }
        return this.nativeCp.Run();
    }

    /**
     * Abort the running copy process.
     */
    abort(): void {
        this.nativeCp.CancelJob();
    }

    /**
     * Abort the running copy process. (alias for abort)
     */
    cancelJob(): void {
        this.nativeCp.CancelJob();
    }
}
