const fs = require('node:fs');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
const root = path.resolve(__dirname, '..');
// Integration tests may start the production game through LauncherService,
// outside CTest. Apply the same quiet, hidden policy to every child process.
const env = {...process.env, PSYZ_HEADLESS: '1', SDL_AUDIODRIVER: 'dummy',
  SDL_MAC_BACKGROUND_APP: '1', SDL_WINDOW_ACTIVATE_WHEN_SHOWN: '0',
  SDL_WINDOW_ACTIVATE_WHEN_RAISED: '0'};
function run(args) {
  const result = spawnSync(process.execPath, args, {cwd: root, env,
    stdio: 'inherit', shell: false});
  if (result.error) console.error(result.error.message);
  if (result.status !== 0) process.exit(result.status ?? 1);
}
run([path.join(__dirname, 'test-native.cjs')]);
const tests = fs.readdirSync(path.join(root, 'tests'))
  .filter(name => name.endsWith('.test.cjs')).sort()
  .map(name => path.join(root, 'tests', name));
run(['--test', ...tests]);
