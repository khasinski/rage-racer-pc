// The lobby, room and race conveniences, in headless browsers:
//   node scripts/ux-check.mjs <disc file ...>
// A guest saves their account; the race forms open on the last car chosen and
// mark painted cars; "Race again" from the results starts another race; a
// player on the WebSocket fallback is marked next to their ping; the controls
// fold away; and a touch screen drives with on-screen buttons.
import { join, resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, startServer, wait, web } from './lib/harness.mjs';

const discFiles = process.argv.slice(2).map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: ux-check.mjs <disc file ...>'); process.exit(2); }
const { base, stop } = await startServer({ disc: discFiles[0], port: 4217, db: join(web, 'e2e-output', 'ux.db') });
const { check, report } = checks();
const errors = [];
const browser = await launchBrowser();
const settled = (page) => page.waitForFunction(() => !document.querySelector('#garage-status').textContent.startsWith('Loading'), null, { timeout: 60_000 });

try {
  // ---- a guest saves their account ----------------------------------------------
  const guest = await openPlayer(browser, { base, discFiles, onError: (e) => errors.push(e) });
  check(await guest.locator('[data-action=claim]').isVisible(), 'a guest is offered to save their account');
  await guest.click('[data-action=claim]');
  await guest.fill('#claim-form input[name=name]', 'rage'); // a development account: taken
  await guest.fill('#claim-form input[name=password]', 'secret1');
  await guest.click('#claim-form button[value=ok]');
  await guest.waitForFunction(() => document.querySelector('#claim-status').textContent.includes('taken'));
  check(true, 'a taken name is refused');
  await guest.fill('#claim-form input[name=name]', 'nightdriver');
  await guest.click('#claim-form button[value=ok]');
  await guest.waitForFunction(() => document.querySelector('#lobby .who').textContent === 'nightdriver');
  check(await guest.locator('[data-action=claim]').isHidden(), 'once saved the account has its name and no Save button');
  const login = await fetch(`${base}/api/login`, { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ name: 'nightdriver', password: 'secret1' }) });
  check(login.ok && (await login.json()).user.guest === false, 'and logs in with the new name and password');

  // ---- the last car, painted marks --------------------------------------------------
  await guest.click('[data-action=garage]');
  await guest.waitForFunction(() => document.querySelector('#garage-status').textContent !== 'Loading the car…', null, { timeout: 60_000 });
  const fatalita = await guest.locator('#garage-car option', { hasText: 'FATALITA III' }).getAttribute('value');
  await guest.selectOption('#garage-car', fatalita);
  await settled(guest);
  await guest.click('#paint-first .swatch:nth-child(10)');
  await guest.waitForFunction(() => document.querySelector('#garage-status').textContent.startsWith('Saved'), null, { timeout: 10_000 });
  await guest.click('#garage [data-action=garage-back]');
  await guest.waitForSelector('#lobby:not([hidden])');
  await guest.click('[data-action=practice]');
  const practiceCar = await guest.locator('#setup select[name=car] option:checked').textContent();
  check(practiceCar === 'FATALITA III · painted', `practice opens on the car last chosen, marked painted (“${practiceCar}”)`);
  await guest.click('#setup [data-action=back-to-lobby]');
  await guest.click('[data-action=create-room]');
  await guest.fill('#settings-form input[name=name]', 'again');
  await guest.selectOption('#settings-form select[name=class]', String(await guest.evaluate((v) => {
    const { rage } = window.__race; for (let c = 0; c < 6; c++) for (let m = 0; m < rage.carModels(); m++) if (rage.classCar(c, m) === Number(v)) return c; return 0;
  }, fatalita)));
  await guest.selectOption('#settings-form select[name=laps]', '1');
  await guest.selectOption('#settings-form select[name=maxPlayers]', '1');
  await guest.uncheck('#settings-form input[name=rivals]');
  await guest.click('#settings-submit');
  await guest.waitForSelector('#room:not([hidden])');
  await guest.waitForFunction((v) => document.querySelector('#car-model').value === v, fatalita, { timeout: 10_000 });
  check(true, 'a new room starts on the car last chosen');
  check((await guest.locator('#car-model option:checked').textContent()).endsWith('· painted'), 'and the room picker marks it painted');

  // ---- controls fold away --------------------------------------------------------
  check(await guest.locator('#room details.controls').evaluate((d) => !d.open), 'the controls are folded away in the room');

  // ---- race again ----------------------------------------------------------------
  await guest.click('#start-button');
  await guest.waitForFunction(() => window.__race?.rage.hud().phase === 2, null, { timeout: 120_000 });
  await guest.keyboard.press('Escape'); // leave: the race ends for a lone player
  await guest.waitForSelector('#results:not([hidden])', { timeout: 60_000 });
  check(await guest.locator('[data-action=race-again]').isVisible(), 'the results offer another race');
  await guest.click('[data-action=race-again]');
  const again = await guest.waitForFunction(() => window.__race?.rage.hud().phase >= 1 && !document.querySelector('#view').hidden, null, { timeout: 120_000 }).then(() => true, () => false);
  check(again, '“Race again” starts the next race');
  await guest.keyboard.press('Escape');
  await guest.waitForSelector('#results:not([hidden])', { timeout: 60_000 });
  await guest.click('[data-action=results-done]');

  // ---- the fallback marked next to the ping -----------------------------------
  const slow = await openPlayer(browser, { base, discFiles, onError: (e) => errors.push(e), query: '?transport=ws' });
  await slow.click('[data-action=create-room]');
  await slow.fill('#settings-form input[name=name]', 'marks');
  await slow.click('#settings-submit');
  await slow.waitForSelector('#room:not([hidden])');
  const marked = await slow.waitForFunction(() => document.querySelector('#room-players .ping.fallback')?.textContent.includes('⚠'), null, { timeout: 15_000 }).then(() => true, () => false);
  check(marked, 'a player racing over the WebSocket fallback is marked next to their ping');
  await slow.context().close();
  const fast = await openPlayer(browser, { base, discFiles, onError: (e) => errors.push(e) });
  await fast.waitForFunction(() => window.__transport?.() === 'rtc', null, { timeout: 20_000 });
  await fast.click('[data-action=create-room]');
  await fast.fill('#settings-form input[name=name]', 'clean');
  await fast.click('#settings-submit');
  await fast.waitForSelector('#room:not([hidden])');
  await fast.waitForFunction(() => /\d+ ms/.test(document.querySelector('#room-players .ping')?.textContent ?? ''), null, { timeout: 15_000 });
  check(await fast.locator('#room-players .ping.fallback').count() === 0, 'and one on the data channel is not');
  await fast.context().close();
  await guest.context().close();

  // ---- touch screens -----------------------------------------------------------
  const phone = await browser.newContext({ viewport: { width: 844, height: 390 }, isMobile: true, hasTouch: true });
  const page = await phone.newPage();
  page.on('pageerror', (e) => errors.push(`phone: ${e.message}`));
  await page.goto(`${base}/#e2e`);
  await page.waitForSelector('#auth:not([hidden])');
  await page.click('#auth button[value=guest]');
  await page.waitForSelector('#disc:not([hidden])');
  await page.setInputFiles('#disc-input', discFiles);
  await page.waitForSelector('#lobby:not([hidden])', { timeout: 180_000 });
  check(await page.locator('#touch').isHidden(), 'the touch controls stay off the lobby');
  await page.click('[data-action=practice]');
  await page.click('#start');
  await page.waitForFunction(() => window.__race?.rage.hud().phase === 2, null, { timeout: 120_000 });
  check(await page.locator('#touch [data-touch=accelerate]').isVisible(), 'a touch screen gets on-screen controls in the race');
  const gas = await page.locator('#touch [data-touch=accelerate]').boundingBox();
  const cdp = await phone.newCDPSession(page);
  const touch = (type) => cdp.send('Input.dispatchTouchEvent', { type, touchPoints: type === 'touchEnd' ? [] : [{ x: gas.x + gas.width / 2, y: gas.y + gas.height / 2 }] });
  await touch('touchStart');
  await wait(4000);
  const speed = await page.evaluate(() => window.__race.rage.hud().speed);
  await touch('touchEnd');
  check(speed > 30, `holding GAS drives the car (${speed} km/h)`);
  await phone.close();
} finally {
  await browser.close();
  stop();
}
if (errors.length) { console.error(errors.join('\n')); process.exitCode = 1; }
report('ux ok');
