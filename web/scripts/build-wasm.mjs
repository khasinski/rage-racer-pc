// Builds the WebAssembly modules (web/CMakeLists.txt, via emcmake): rage-web
// into web/public/wasm for Vite to serve, and the race server's rage-server
// into web/server/wasm.
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

run('emcmake', ['cmake', '-S', web, '-B', build, '-DCMAKE_BUILD_TYPE=Release']);
run('cmake', ['--build', build, '--target', 'rage-web', 'rage-server', '-j8']);

for (const [name, out] of [['rage-web', join(web, 'public', 'wasm')], ['rage-server', join(web, 'server', 'wasm')]]) {
  mkdirSync(out, { recursive: true });
  for (const extension of ['mjs', 'wasm']) copyFileSync(join(build, `${name}.${extension}`), join(out, `${name}.${extension}`));
  console.log(`copied ${name}.{mjs,wasm} to ${out}`);
}
