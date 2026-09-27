// Finish-line checks in real (headless) browsers:
//   node scripts/finish-check.mjs <Track 01 BIN> [out dir]
// Two players hold the throttle and a guest joins the running race as a
// spectator. The guest follows the leading rival over the line: the car
// drives on and fades out, then the camera hands over to the nearest
// player, and everyone sees the time left to finish (bottom left).
import { spawn, spawnSync } from 'node:child_process';
import { mkdirSync, rmSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { chromium } from 'playwright';
const web = join(fileURLToPath(new URL('.', import.meta.url)), '..');
const [disc, out = join(web, 'e2e-output')] = process.argv.slice(2);
if (!disc) { console.error('usage: finish-check.mjs <disc> [out dir]'); process.exit(2); }
mkdirSync(out, { recursive: true });
const failures = [];
const check = (ok, what) => { console.log(`${ok ? '✓' : '✗'} ${what}`); if (!ok) failures.push(what); };
const db = join(out, 'finish.db'); for (const s of ['', '-wal', '-shm']) rmSync(db + s, { force: true });
spawnSync(process.execPath, [join(web, 'server/seed.ts'), '--db', db, '--admin-password', 'admin', '--rage-password', 'racer']);
const server = spawn(process.execPath, [join(web, 'server/main.ts'), '--disc', disc, '--port', '4191', '--db', db], { stdio: ['ignore', 'pipe', 'inherit'] });
await new Promise((r) => server.stdout.on('data', (d) => String(d).includes('server on') && r()));
const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
const errors = [];
async function open(name, password) {
  const page = await (await browser.newContext({ viewport: { width: 1280, height: 720 } })).newPage();
  page.on('pageerror', (e) => errors.push(`${name}: ${e.message}`));
  await page.goto('http://localhost:4191/#e2e');
  await page.waitForSelector('#auth:not([hidden])');
  if (name === 'guest') await page.click('#auth button[value=guest]');
  else { await page.fill('#auth input[name=name]', name); await page.fill('#auth input[name=password]', password); await page.click('#auth button[value=login]'); }
  await page.waitForSelector('#disc:not([hidden])');
  await page.setInputFiles('#disc-input', disc);
  await page.waitForSelector('#lobby:not([hidden])', { timeout: 180000 });
  return page;
}
try {
  const admin = await open('admin', 'admin'), rage = await open('rage', 'racer'), guest = await open('guest');
  await admin.click('[data-action=create-room]');
  await admin.fill('#settings-form input[name=name]', 'Finish');
  await admin.selectOption('#settings-form select[name=class]', '2');
  await admin.selectOption('#settings-form select[name=laps]', '1');
  await admin.selectOption('#settings-form select[name=maxPlayers]', '2');
  await admin.click('#settings-submit');
  await rage.click('#room-list button:has-text("Join")');
  await rage.waitForSelector('#room:not([hidden])');
  await rage.click('#ready-button');
  await admin.waitForSelector('#start-button:not([disabled])');
  await admin.click('#start-button');
  await Promise.all([admin, rage].map((p) => p.waitForSelector('#view:not([hidden])', { timeout: 120000 })));
  await Promise.all([admin, rage].map((p) => p.keyboard.down('KeyX')));
  await guest.click('#room-list button:has-text("Watch")');
  await guest.waitForSelector('#view:not([hidden])', { timeout: 120000 });
  await guest.waitForTimeout(8000);
  await guest.screenshot({ path: join(out, 'f1-guest-watching.png') });
  check(/^Watching (admin|rage)/.test(await guest.textContent('#spectating')), 'a spectator follows a player');
  check(await guest.$eval('#hud-hint', (e) => !e.textContent), 'the joining hint clears once the race runs');
  // Follow the leader until it finishes.
  const leader = await guest.evaluate(() => { const { rage } = window.__race; let best = 0; for (let s = 0; s < 12; s++) if (rage.standing(s).place === 1) best = s; rage.setViewSeat(best); return best; });
  await guest.waitForFunction((seat) => window.__race.rage.standing(seat).status === 2, leader, { timeout: 180000, polling: 100 });
  for (const ms of [0, 600, 1200, 1800, 2400]) {
    if (ms) await guest.waitForTimeout(600);
    await guest.screenshot({ path: join(out, `f2-fading-${ms}.png`), clip: { x: 440, y: 480, width: 400, height: 220 } });
  }
  await guest.waitForTimeout(2000);
  await guest.screenshot({ path: join(out, 'f3-handed-over.png') });
  check(await guest.evaluate((seat) => window.__race.rage.seatGone(seat), leader), 'the finished car has faded out');
  check(/^Watching (admin|rage)/.test(await guest.textContent('#spectating')), 'the camera hands over to the nearest player');
  check(/^Race closes in \d+:\d\d$/.test(await guest.textContent('#deadline')), 'the spectator sees the time left to finish');
  await admin.screenshot({ path: join(out, 'f4-admin-deadline.png') });
  check(!(await admin.$eval('#deadline', (e) => e.hidden)), 'players see the time left to finish');
} catch (error) { failures.push(String(error)); } finally { await browser.close(); server.kill(); }
failures.push(...errors);
if (failures.length) { console.error(failures.join('\n')); process.exit(1); }
console.log('ok');
