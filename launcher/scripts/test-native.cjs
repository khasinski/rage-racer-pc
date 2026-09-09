const path = require('node:path');
const {spawnSync} = require('node:child_process');
const root = path.resolve(__dirname, '../..');
const build = path.resolve(root, process.env.RAGE_LAUNCHER_BUILD_DIR || 'build/release');
// Built by build:native, alongside the CLI it verifies. Fail if the native
// suite is absent so an unstaged build cannot silently lose format coverage.
const result = spawnSync('ctest', ['--test-dir', build, '-C', 'Release',
  '-R', '^save_format$', '--no-tests=error', '--output-on-failure'],
  {cwd: root, stdio: 'inherit', shell: false});
if (result.error) console.error(result.error.message);
process.exit(result.status ?? 1);
