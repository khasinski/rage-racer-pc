// Two windows on one account: the newer one takes over and the older one stays
// out instead of reconnecting (which would push the newer one out again, for
// ever).   node scripts/replaced-check.mjs <disc file ...>
import { join, resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, startServer, wait, web } from './lib/harness.mjs';

const discFiles = process.argv.slice(2).map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: replaced-check.mjs <disc file ...>'); process.exit(2); }
const { base, stop } = await startServer({ disc: discFiles[0], port: 4211, db: join(web, 'e2e-output', 'replaced.db') });
const { check, report } = checks();
const errors = [];
const browser = await launchBrowser();
try {
  const first = await openPlayer(browser, { base, discFiles, onError: (e) => errors.push(e) });
  const token = await first.evaluate(() => localStorage.getItem('rage-racer.session'));
  const context = await browser.newContext({ viewport: { width: 1280, height: 720 } });
  await context.addInitScript((t) => localStorage.setItem('rage-racer.session', t), token);
  const second = await context.newPage();
  await second.goto(`${base}/#e2e`);
  await second.waitForSelector('#disc:not([hidden])');
  await second.setInputFiles('#disc-input', discFiles);
  await second.waitForSelector('#lobby:not([hidden])', { timeout: 180_000 });
  await first.waitForFunction(() => document.querySelector('#connection-text').textContent.includes('another window') && !document.querySelector('#connection').hidden, null, { timeout: 15_000 });
  check(true, 'the older window is told another window took over');
  await wait(6_000); // long enough for several reconnect attempts
  check(await second.locator('#connection').isHidden(), 'the newer window is left alone');
  check((await first.locator('#connection-text').textContent()).includes('another window'), 'and the older one still says so instead of reconnecting');
  check(await first.locator('[data-action=create-room]').evaluate((b) => getComputedStyle(b).pointerEvents === 'none'), 'while the older window cannot create rooms');
  await first.click('#connection-here');
  await first.waitForFunction(() => document.querySelector('#connection').hidden, null, { timeout: 15_000 });
  await second.waitForFunction(() => document.querySelector('#connection-text').textContent.includes('another window') && !document.querySelector('#connection').hidden, null, { timeout: 15_000 });
  check(true, '“Play here” in the older window takes the account back, without a reload');
} finally {
  await browser.close();
  stop();
}
if (errors.length) { console.error(errors.join('\n')); process.exitCode = 1; }
report('replaced ok');
