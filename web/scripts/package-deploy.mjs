// Assembles the CapRover deployment bundle:
//   node scripts/package-deploy.mjs <Track 01 BIN> [out.tar]
// Run `npm run wasm` and `npm run build` first. The bundle holds the built
// client, the server with its WebAssembly simulation, the image recipe
// (deploy/) and the server operator's disc data track.
import { spawnSync } from 'node:child_process';
import { cpSync, existsSync, mkdirSync, mkdtempSync, rmSync } from 'node:fs';
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
  for (const file of ['db.ts', 'main.ts', 'rooms.ts', 'seed.ts', 'sim.ts']) cpSync(join(web, 'server', file), join(stage, 'server', file));
  cpSync(join(web, 'server', 'wasm'), join(stage, 'server', 'wasm'), { recursive: true });
  mkdirSync(join(stage, 'disc'));
  cpSync(resolve(discArg), join(stage, 'disc', 'track01.bin'), { dereference: true });
  const tar = spawnSync('tar', ['-cf', out, '-C', stage, '.'], { stdio: 'inherit' });
  if (tar.status !== 0) process.exit(tar.status ?? 1);
  console.log(`bundle: ${out}`);
} finally {
  rmSync(stage, { recursive: true, force: true });
}
