// Builds the WebAssembly module (CMake target rage-web, via emcmake) and
// copies it into web/public/wasm for Vite to serve.
//   npm run wasm
import { copyFileSync, mkdirSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const web = join(dirname(fileURLToPath(import.meta.url)), '..');
const root = join(web, '..');
const build = join(root, 'build', 'web');
// Emscripten runs its own JS with this Node, not whatever `node` is on PATH.
const env = { ...process.env, EM_NODE_JS: process.execPath };

const run = (command, args) => {
  const result = spawnSync(command, args, { cwd: root, env, stdio: 'inherit' });
  if (result.status !== 0) process.exit(result.status ?? 1);
};

run('emcmake', ['cmake', '-S', '.', '-B', build, '-DRAGE_BUILD_PORT=OFF', '-DBUILD_TESTING=OFF',
                '-DRAGE_EMBED_AUTHORED_CARS=OFF', '-DCMAKE_BUILD_TYPE=Release']);
run('cmake', ['--build', build, '--target', 'rage-web', '-j8']);

const out = join(web, 'public', 'wasm');
mkdirSync(out, { recursive: true });
for (const file of ['rage-web.mjs', 'rage-web.wasm']) copyFileSync(join(build, file), join(out, file));
console.log(`copied rage-web.{mjs,wasm} to ${out}`);
