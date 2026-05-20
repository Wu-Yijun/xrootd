import test from 'node:test';
import assert from 'node:assert';
import { CopyProcess } from 'xrootd';

test('CopyProcess constructor', () => {
    const cp = new CopyProcess();
    assert.ok(cp);
});

test('CopyProcess addJob and prepare', async () => {
    const cp = new CopyProcess();
    cp.addJob({
        source: 'root://localhost//tmp/test1',
        target: 'root://localhost//tmp/test2',
        force: true
    });
    
    // We only call prepare. Since the endpoints might not exist in a pure unit test environment,
    // prepare might fail or pass, but we just verify the method is accessible.
    try {
        await cp.prepare();
    } catch (err) {
        // Ignored in basic tests, as it requires a real XRootD server
        assert.ok(err);
    }
});

test('CopyProcess cancelJob', () => {
    const cp = new CopyProcess();
    cp.cancelJob();
    assert.ok(true);
});
