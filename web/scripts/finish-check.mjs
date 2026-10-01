// Finish-line checks in real (headless) browsers:
//   node scripts/finish-check.mjs <Track 01 BIN> [out dir]
// Two players hold the throttle and a guest joins the running race as a
// spectator. The guest follows the leading rival over the line: the car
// stays on the road, brakes, and disappears, then the camera hands over to the nearest
// player, and everyone sees the time left to finish (bottom left).
import { mkdirSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, startServer, web } from './lib/harness.mjs';

const [discArg, out = join(web, 'e2e-output')] = process.argv.slice(2);
if (!discArg) { console.error('usage: finish-check.mjs <disc> [out dir]'); process.exit(2); }
const disc = resolve(discArg);
mkdirSync(out, { recursive: true });
const { check, failures, report } = checks();
const { base, stop } = await startServer({ disc, port: 4191, db: join(out, 'finish.db') });
const browser = await launchBrowser();
const open = (name, password) => openPlayer(browser, { base, name, password, discFiles: disc, onError: (e) => failures.push(e) });
try {
  const admin = await open('admin', 'admin'), rage = await open('rage', 'racer'), guest = await open();
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
  check(await guest.evaluate((seat) => window.__race.rage.seatGone(seat), leader), 'the finished car has left the picture');
  check(/^Watching (admin|rage)/.test(await guest.textContent('#spectating')), 'the camera hands over to the nearest player');
  check(/^Race closes in \d+:\d\d$/.test(await guest.textContent('#deadline')), 'the spectator sees the time left to finish');
  await admin.screenshot({ path: join(out, 'f4-admin-deadline.png') });
  check(!(await admin.$eval('#deadline', (e) => e.hidden)), 'players see the time left to finish');
} catch (error) { failures.push(String(error)); } finally { await browser.close(); stop(); }
report();
