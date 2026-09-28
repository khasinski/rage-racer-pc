// Netcode measurements under simulated network conditions:
//   node scripts/net-check.mjs <Track 01 BIN> [--net rtt:150,jitter:30,loss:2 ...]
// Two players race without rivals; B weaves left and right, so A has to keep
// correcting its prediction of B. For each condition it reports, per player,
// how far server frames moved the predicted cars (own / other, mean and max,
// world units) and, as A sees it, how jerky B's car looks: the per-frame
// change of its on-screen motion (99th percentile and max).
import { join, resolve } from 'node:path';
import { launchBrowser, openPlayer, startServer, wait, web } from './lib/harness.mjs';

const args = process.argv.slice(2);
const disc = args[0] && resolve(args[0]);
if (!disc) { console.error('usage: net-check.mjs <disc> [--net spec ...]'); process.exit(2); }
const conditions = [];
for (let i = 1; i < args.length; i++) if (args[i] === '--net') conditions.push(args[++i]);
if (!conditions.length) conditions.push('', 'rtt:150,jitter:20,loss:1', 'rtt:250,jitter:50,loss:3');

const { stop, base } = await startServer({ disc, port: 4194, db: join(web, 'e2e-output', 'net-check.db') });
const browser = await launchBrowser();
const errors = [];
const percentile = (values, p) => { const s = [...values].sort((a, b) => a - b); return s[Math.min(s.length - 1, Math.floor(p * s.length))] ?? 0; };
try {
  for (const net of conditions) {
    const query = net ? `?net=${net}` : '';
    const a = await openPlayer(browser, { base, discFiles: [disc], onError: (e) => errors.push(e), query });
    const b = await openPlayer(browser, { base, discFiles: [disc], onError: (e) => errors.push(e), query });
    await a.click('[data-action=create-room]');
    await a.fill('#settings-form input[name=name]', `net ${net || 'direct'}`);
    await a.selectOption('#settings-form select[name=laps]', '1');
    await a.selectOption('#settings-form select[name=maxPlayers]', '2');
    await a.uncheck('#settings-form input[name=rivals]');
    await a.click('#settings-submit');
    const joinButton = b.locator('#room-list li', { hasText: `net ${net || 'direct'}` }).locator('button');
    await joinButton.waitFor();
    await joinButton.click();
    await b.waitForSelector('#room:not([hidden])');
    await b.click('#ready-button');
    await a.waitForSelector('#start-button:not([disabled])');
    await a.click('#start-button');
    for (const p of [a, b]) await p.waitForFunction(() => window.__race?.rage.hud().phase === 2, null, { timeout: 120_000 });
    await a.keyboard.down('KeyX');
    await b.keyboard.down('KeyX');
    await wait(3000);
    const seats = await Promise.all([a, b].map((p) => p.evaluate(() => {
      const { rage } = window.__race; rage.resetNetStats(); return rage.viewSeat();
    })));
    // A records B's car where it is drawn, every frame; B weaves meanwhile.
    const recording = a.evaluate(async (other) => {
      const { rage } = window.__race; const points = []; const t0 = performance.now();
      while (performance.now() - t0 < 15_000) { await new Promise((r) => requestAnimationFrame(r)); points.push(rage.presented(other)); }
      return points;
    }, seats[1]);
    for (let k = 0; k < 20; k++) {
      const key = k % 2 ? 'ArrowLeft' : 'ArrowRight';
      await b.keyboard.down(key); await wait(450); await b.keyboard.up(key); await wait(300);
    }
    const points = await recording;
    const steps = points.slice(1).map((p, i) => [p[0] - points[i][0], p[2] - points[i][2]]);
    const jerk = steps.slice(1).map((d, i) => Math.hypot(d[0] - steps[i][0], d[1] - steps[i][1]));
    const [sa, sb] = await Promise.all([a, b].map((p) => p.evaluate(() => window.__race.rage.netStats())));
    const f = (x) => Math.round(x);
    console.log(`${net || 'direct'}:`);
    for (const [who, s] of [['A', sa], ['B', sb]]) {
      console.log(`  ${who} corrections over ${s[0]} frames: own mean ${f(s[1])} max ${f(s[2])}, other mean ${f(s[3])} max ${f(s[4])}, replay ${s[5].toFixed(1)} ticks`);
    }
    console.log(`  B as seen by A (${points.length} frames): jerk p99 ${f(percentile(jerk, 0.99))}, max ${f(Math.max(...jerk))}`);
    await Promise.all([a, b].map((p) => p.context().close()));
  }
} finally {
  await browser.close();
  stop();
}
if (errors.length) { console.error(errors.join('\n')); process.exit(1); }
