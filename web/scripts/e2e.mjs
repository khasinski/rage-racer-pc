// End-to-end check of the whole app in real (headless) browsers:
//   node scripts/e2e.mjs <disc file ...> [--out dir]
// Starts the multiplayer server with a scratch database and the development
// accounts, then two players (admin and rage) log in, choose the disc through
// the page's file picker, meet in a room, race each other on the server, leave
// and read the results; one of them then drives an offline practice race.
// Fails on any page error, a blank frame or a step that does not happen.
import { spawn, spawnSync } from 'node:child_process';
import { mkdirSync, rmSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { chromium } from 'playwright';

const web = join(dirname(fileURLToPath(import.meta.url)), '..');
const args = process.argv.slice(2);
const outIndex = args.indexOf('--out');
const out = resolve(outIndex >= 0 ? args.splice(outIndex, 2)[1] : join(web, 'e2e-output'));
const discFiles = args.map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: e2e.mjs <disc file ...> [--out dir]'); process.exit(2); }
mkdirSync(out, { recursive: true });

// The server reads the Track 01 BIN (or the CUE) the players will choose.
const serverDisc = discFiles.find((f) => /\.cue$/i.test(f)) ?? discFiles[0];
const db = join(out, 'e2e.db');
for (const suffix of ['', '-wal', '-shm']) rmSync(db + suffix, { force: true });
spawnSync(process.execPath, [join(web, 'server/seed.ts'), '--db', db], { stdio: 'inherit' });
const port = 4180;
const server = spawn(process.execPath, [join(web, 'server/main.ts'), '--disc', serverDisc, '--port', String(port), '--db', db],
  { cwd: web, stdio: ['ignore', 'pipe', 'inherit'] });
await new Promise((ready, fail) => {
  server.stdout.on('data', (d) => String(d).includes('server on') && ready());
  server.on('exit', (code) => fail(new Error(`server exited with ${code}`)));
});

const failures = [];
const step = (text) => console.log(`· ${text}`);
const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });

async function player(name, password) {
  const context = await browser.newContext({ viewport: { width: 1280, height: 720 } });
  const page = await context.newPage();
  page.on('pageerror', (error) => failures.push(`${name}: page error: ${error.message}`));
  page.on('console', (message) => {
    if (message.type() === 'error') failures.push(`${name}: console: ${message.text()}`);
  });
  await page.goto(`http://localhost:${port}/#e2e`);
  await page.waitForSelector('#auth:not([hidden])');
  await page.fill('#auth input[name=name]', name);
  await page.fill('#auth input[name=password]', password);
  await page.click('#auth button[value=login]');
  await page.waitForSelector('#disc:not([hidden])');
  await page.setInputFiles('#disc-input', discFiles);
  await page.waitForSelector('#lobby:not([hidden])', { timeout: 180_000 });
  return page;
}

const blank = (page) => page.evaluate(() => {
  const canvas = document.getElementById('view');
  const probe = document.createElement('canvas');
  probe.width = 64; probe.height = 36;
  const context = probe.getContext('2d');
  context.drawImage(canvas, 0, 0, 64, 36);
  const data = context.getImageData(0, 0, 64, 36).data;
  const colours = new Set();
  for (let i = 0; i < data.length; i += 4) colours.add((data[i] << 16) | (data[i + 1] << 8) | data[i + 2]);
  return colours.size < 8;
});
const hud = (page) => page.evaluate(() => Object.fromEntries(['place', 'lap', 'time', 'speed', 'gear']
  .map((id) => [id, document.getElementById(`hud-${id}`)?.textContent])));

try {
  const admin = await player('admin', 'admin');
  const rage = await player('rage', 'racer');
  step('both players logged in and loaded the disc');
  await admin.screenshot({ path: join(out, '01-lobby.png') });

  // The host creates a room: class 3, Mythical Coast, one lap, rivals.
  await admin.click('[data-action=create-room]');
  await admin.fill('#settings-form input[name=name]', 'E2E Grand Prix');
  await admin.selectOption('#settings-form select[name=class]', '2');
  await admin.selectOption('#settings-form select[name=laps]', '1');
  await admin.selectOption('#settings-form select[name=maxPlayers]', '4');
  await admin.screenshot({ path: join(out, '02-create-room.png') });
  await admin.click('#settings-submit');
  await admin.waitForSelector('#room:not([hidden])');
  step('admin created a room');

  await rage.waitForSelector('#room-list button:has-text("Join")');
  await rage.click('#room-list button:has-text("Join")');
  await rage.waitForSelector('#room:not([hidden])');
  const cars = await rage.$$eval('#car-model option', (options) => options.map((o) => o.textContent));
  step(`rage joined; class 3 offers ${cars.join(', ')}`);
  await rage.selectOption('#car-model', { index: Math.min(4, cars.length - 1) });
  await rage.fill('#chat-form input', 'good luck!');
  await rage.press('#chat-form input', 'Enter');
  await rage.click('#ready-button');
  await admin.waitForSelector('#chat-log li:has-text("good luck!")');
  await admin.waitForSelector('#start-button:not([disabled])');
  await admin.screenshot({ path: join(out, '03-room.png') });
  await admin.click('#start-button');
  step('host started the race');

  await Promise.all([admin, rage].map((page) => page.waitForSelector('#view:not([hidden])', { timeout: 120_000 })));
  await Promise.all([admin, rage].map((page) =>
    page.waitForFunction(() => document.getElementById('hud-banner')?.textContent !== '', null, { timeout: 120_000 })));
  await admin.screenshot({ path: join(out, '04-countdown-admin.png') });
  // Once racing, rage switches to the chase view: both players start together.
  await rage.waitForFunction(() => document.getElementById('hud-banner')?.textContent === '', null, { timeout: 60_000 });
  await rage.keyboard.press('KeyS');
  await rage.waitForTimeout(400);
  await rage.screenshot({ path: join(out, '05-grid-rage.png') });
  await Promise.all([admin, rage].map((page) => page.keyboard.down('KeyX')));
  await admin.waitForTimeout(8000);
  await admin.screenshot({ path: join(out, '06-race-admin.png') });
  await rage.screenshot({ path: join(out, '07-race-rage.png') });
  const hudAdmin = await hud(admin);
  const hudRage = await hud(rage);
  console.log('hud admin', JSON.stringify(hudAdmin), 'rage', JSON.stringify(hudRage));
  for (const [name, h] of [['admin', hudAdmin], ['rage', hudRage]]) {
    if (!(Number(h.speed) > 0)) failures.push(`${name}'s car did not move`);
    if (!h.place?.endsWith('/12')) failures.push(`${name} does not see the full field: ${h.place}`);
  }
  if (hudAdmin.time === '0:00.00') failures.push('the race clock did not run');
  for (const [name, page] of [['admin', admin], ['rage', rage]]) if (await blank(page)) failures.push(`${name}'s race view is blank`);
  await Promise.all([admin, rage].map((page) => page.keyboard.up('KeyX')));

  // Both leave the race; the server closes it and sends the results.
  await admin.keyboard.press('Escape');
  await rage.keyboard.press('Escape');
  await Promise.all([admin, rage].map((page) => page.waitForSelector('#results:not([hidden])', { timeout: 30_000 })));
  const rows = await admin.$$eval('#results-table tr', (trs) => trs.length - 1);
  if (rows !== 12) failures.push(`results list ${rows} cars instead of 12`);
  await admin.screenshot({ path: join(out, '08-results.png') });
  step(`results arrived (${rows} cars)`);
  await admin.click('[data-action=results-done]');
  await admin.waitForSelector('#room:not([hidden])');

  // Offline practice still works from the lobby.
  await rage.click('[data-action=results-done]');
  await rage.click('[data-action=leave-room]');
  await rage.waitForSelector('#lobby:not([hidden])');
  const history = await rage.$$eval('#history tr', (trs) => trs.length - 1);
  if (history < 1) failures.push('the race is missing from rage\'s history');
  await rage.click('[data-action=practice]');
  await rage.click('#start');
  await rage.waitForSelector('#view:not([hidden])');
  await rage.keyboard.down('KeyX');
  await rage.waitForTimeout(6000);
  await rage.keyboard.up('KeyX');
  await rage.screenshot({ path: join(out, '09-practice.png') });
  const practice = await hud(rage);
  if (!(Number(practice.speed) > 0)) failures.push('the practice car did not move');
  step(`practice race ran (${JSON.stringify(practice)})`);
} catch (error) {
  failures.push(String(error));
} finally {
  await browser.close();
  server.kill();
}
if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log(`ok — screenshots in ${out}`);
