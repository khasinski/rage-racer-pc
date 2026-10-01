// Joining a race long after its start, coming back to it after a dropped
// connection, and a page that stalls longer than the per-tick catch-up allows
// (a tab in the background, a slow machine): each time the page jumps far
// ahead of what it showed, and it must stay in the race and catch up.
//   node scripts/stall-check.mjs <disc file ...>
import { join, resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, startServer, wait, web } from './lib/harness.mjs';

const discFiles = process.argv.slice(2).map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: stall-check.mjs <disc file ...>'); process.exit(2); }
const { base, stop } = await startServer({ disc: discFiles[0], port: 4218, db: join(web, 'e2e-output', 'stall.db') });
const { check, report } = checks();
const errors = [];
const browser = await launchBrowser();
const GAP_MS = 13_000; // past the 10 s the scenery and race view catch up per tick
const tickOf = (page) => page.evaluate(() => window.__race.rage.hud().tick);

/** The page stays in the race, follows the other page's race and keeps going. */
async function keepsRacing(who, page, other) {
  await wait(4000);
  const screen = await page.evaluate(() => document.body.dataset.screen);
  const said = await page.evaluate(() => ['lobby-status', 'room-status', 'hud-hint'].map((id) => document.getElementById(id).textContent).filter(Boolean).join(' | '));
  check(screen === 'race', `${who}: still in the race (screen: ${screen}${screen === 'race' ? '' : `; ${said}`})`);
  await wait(6000); // a player's clock may first settle back to its lead over the server
  const off = (await tickOf(other)) - (await tickOf(page));
  check(Math.abs(off) < 150, `${who}: caught up (${off} ticks off the other page)`);
  const t0 = await tickOf(page);
  await wait(1000);
  check((await tickOf(page)) - t0 >= 30, `${who}: keeps going`);
}

try {
  const player = await openPlayer(browser, { base, discFiles, onError: (e) => errors.push(e) });
  const watcher = await openPlayer(browser, { base, discFiles, onError: (e) => errors.push(e) });
  await player.click('[data-action=create-room]');
  await player.fill('#settings-form input[name=name]', 'stall');
  await player.selectOption('#settings-form select[name=laps]', '3');
  await player.selectOption('#settings-form select[name=maxPlayers]', '1');
  await player.click('#settings-submit');
  await player.waitForSelector('#start-button:not([disabled])');
  await player.click('#start-button');
  await player.waitForFunction(() => window.__race?.rage.hud().phase === 2, null, { timeout: 120_000 });
  await player.keyboard.down('KeyX');
  await wait(GAP_MS);

  // A spectator joins a race that has run for a while.
  const watch = watcher.locator('#room-list li', { hasText: 'stall' }).locator('button');
  await watch.waitFor();
  await watch.click();
  await watcher.waitForFunction(() => window.__race?.rage.hud().phase === 2, null, { timeout: 120_000 });
  await keepsRacing('a spectator joining late', watcher, player);

  // Pages that stall.
  for (const [who, page, other] of [['a stalled spectator', watcher, player], ['a stalled player', player, watcher]]) {
    await page.evaluate((ms) => { const t = performance.now(); while (performance.now() - t < ms) { /* frozen */ } }, GAP_MS);
    await keepsRacing(who, page, other);
  }

  // A player whose connection drops comes back to their car.
  await player.context().setOffline(true);
  await player.evaluate(() => window.__online?.connection()?.close());
  await wait(GAP_MS);
  await player.context().setOffline(false);
  await keepsRacing('a player coming back after a dropped connection', player, watcher);
} finally {
  await browser.close();
  stop();
}
if (errors.length) { console.error(errors.join('\n')); process.exitCode = 1; }
report('stall ok');
