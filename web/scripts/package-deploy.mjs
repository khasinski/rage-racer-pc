// Assembles the CapRover deployment bundle:
//   node scripts/package-deploy.mjs <Track 01 BIN> [out.tar]
// Run `npm run wasm` and `npm run build` first. The bundle holds the built
// client, the server with its WebAssembly simulation, the image recipe
// (deploy/) and the server operator's disc data track.
import { spawnSync } from 'node:child_process';
import { cpSync, existsSync, mkdirSync, mkdtempSync, readdirSync, rmSync, symlinkSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const web = join(dirname(fileURLToPath(import.meta.url)), '..');
const [discArg, outArg] = process.argv.slice(2);
if (!discArg) { console.error('usage: package-deploy.mjs <Track 01 BIN> [out.tar]'); process.exit(2); }
const out = resolve(outArg ?? join(web, 'deploy.tar'));
for (const needed of ['dist/index.html', 'server/wasm/rage-server.wasm']) {
  if (!existsSync(join(web, needed))) { console.error(`missing ${needed}: run npm run wasm && npm run build`); process.exit(1); }
}
const stage = mkdtempSync(join(tmpdir(), 'rage-deploy-'));
try {
  for (const file of ['package.json', 'package-lock.json']) cpSync(join(web, file), join(stage, file));
  for (const file of ['Dockerfile', 'captain-definition']) cpSync(join(web, 'deploy', file), join(stage, file));
  cpSync(join(web, 'shared'), join(stage, 'shared'), { recursive: true });
  cpSync(join(web, 'dist'), join(stage, 'dist'), { recursive: true });
  mkdirSync(join(stage, 'server'));
  // Every server module (type-stripped TypeScript, run as is).
  for (const file of readdirSync(join(web, 'server')).filter((f) => f.endsWith('.ts'))) cpSync(join(web, 'server', file), join(stage, 'server', file));
  cpSync(join(web, 'server', 'wasm'), join(stage, 'server', 'wasm'), { recursive: true });
  mkdirSync(join(stage, 'disc'));
  cpSync(resolve(discArg), join(stage, 'disc', 'track01.bin'), { dereference: true });
  // Start the bundled server once (with this checkout's node_modules), so a
  // missing module or file fails here rather than on the live server.
  symlinkSync(join(web, 'node_modules'), join(stage, 'node_modules'));
  const check = spawnSync(process.execPath, ['-e', `
    const { spawn } = await import('node:child_process');
    const server = spawn(process.execPath, ['server/main.ts', '--disc', 'disc/track01.bin', '--port', '4189', '--db', 'check.db'], { stdio: ['ignore', 'pipe', 'pipe'] });
    let log = '';
    const started = await new Promise((resolve) => {
      server.stdout.on('data', (d) => { log += d; if (String(d).includes('server on')) resolve(true); });
      server.stderr.on('data', (d) => { log += d; });
      server.on('exit', () => resolve(false));
      setTimeout(() => resolve(false), 60000);
    });
    server.kill();
    if (!started) { console.error(log); process.exit(1); }`], { cwd: stage, stdio: 'inherit', input: '' });
  if (check.status !== 0) { console.error('the bundled server does not start'); process.exit(1); }
  for (const file of ['node_modules', 'check.db', 'check.db-wal', 'check.db-shm']) rmSync(join(stage, file), { force: true, recursive: false });
  console.log('bundled server starts');
  const tar = spawnSync('tar', ['-cf', out, '-C', stage, '.'], { stdio: 'inherit' });
  if (tar.status !== 0) process.exit(tar.status ?? 1);
  console.log(`bundle: ${out}`);
} finally {
  rmSync(stage, { recursive: true, force: true });
}
