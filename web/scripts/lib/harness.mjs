// Shared by the server and browser checks: a server on a scratch database
// with the development accounts (admin/admin, rage/racer), headless players
// who log in and load the disc, and pass/fail reporting.
import { spawn, spawnSync } from 'node:child_process';
import { rmSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

export const web = join(dirname(fileURLToPath(import.meta.url)), '..', '..');

export const wait = (ms) => new Promise((r) => setTimeout(r, ms));

/** Polls until a condition holds (messages arrive asynchronously). */
export async function until(condition, ms = 10_000) {
  const t0 = Date.now();
  while (!condition() && Date.now() - t0 < ms) await wait(20);
  return Boolean(condition());
}

/** Collects failures; `check` also prints each result. */
export function checks() {
  const failures = [];
  return {
    failures,
    check(ok, what) {
      console.log(`${ok ? '✓' : '✗'} ${what}`);
      if (!ok) failures.push(what);
    },
    /** Prints the failures and sets the exit code; true when all passed. */
    report(success = 'ok') {
      if (failures.length) {
        console.error(`${failures.length} failed:\n${failures.join('\n')}`);
        process.exitCode = 1;
        return false;
      }
      console.log(success);
      return true;
    },
  };
}

/** Seeds a fresh database and starts the server on it; resolves once it
 *  listens, rejects if it exits first. */
export async function startServer({ disc, port, db, env = {} }) {
  for (const suffix of ['', '-wal', '-shm']) rmSync(db + suffix, { force: true });
  const seed = spawnSync(process.execPath, [join(web, 'server/seed.ts'), '--db', db,
    '--admin-password', 'admin', '--rage-password', 'racer'], { encoding: 'utf8' });
  if (seed.status !== 0) throw new Error(`seeding failed: ${seed.stderr}`);
  const server = spawn(process.execPath, [join(web, 'server/main.ts'), '--disc', disc, '--port', String(port), '--db', db],
    { cwd: web, stdio: ['ignore', 'pipe', 'inherit'], env: { ...process.env, ...env } });
  await new Promise((ready, fail) => {
    server.stdout.on('data', (d) => String(d).includes('server on') && ready());
    server.on('exit', (code) => fail(new Error(`the server exited with ${code}`)));
  });
  return { server, base: `http://localhost:${port}`, stop: () => server.kill() };
}

export async function launchBrowser() {
  const { chromium } = await import('playwright');
  return chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
}

/** Opens the app in a new browser context, logs in (as a guest without a
 *  name) and chooses the disc files; resolves on the lobby. Page errors (and
 *  console errors with `consoleErrors`) go to `onError`. */
export async function openPlayer(browser, { base, name, password, discFiles, onError, consoleErrors = false, init }) {
  const context = await browser.newContext({ viewport: { width: 1280, height: 720 } });
  const page = await context.newPage();
  const who = name ?? 'guest';
  page.on('pageerror', (error) => onError(`${who}: page error: ${error.message}`));
  if (consoleErrors) page.on('console', (m) => { if (m.type() === 'error') onError(`${who}: console: ${m.text()}`); });
  if (init) await page.addInitScript(init);
  await page.goto(`${base}/#e2e`);
  await page.waitForSelector('#auth:not([hidden])');
  if (name === undefined) await page.click('#auth button[value=guest]');
  else {
    await page.fill('#auth input[name=name]', name);
    await page.fill('#auth input[name=password]', password);
    await page.click('#auth button[value=login]');
  }
  await page.waitForSelector('#disc:not([hidden])');
  await page.setInputFiles('#disc-input', discFiles);
  await page.waitForSelector('#lobby:not([hidden])', { timeout: 180_000 });
  return page;
}
