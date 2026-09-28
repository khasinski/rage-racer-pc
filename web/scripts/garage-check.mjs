// The garage and paint, end to end in headless browsers:
//   node scripts/garage-check.mjs <disc file ...> [--out dir]
// A player opens the garage, sees the car (a non-blank preview), paints it
// and saves; the paint survives a new login, and in a race against a guest the
// guest's own client draws the painted car. Screenshots go to the output dir.
import { mkdirSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, startServer, until, wait, web } from './lib/harness.mjs';

const args = process.argv.slice(2);
const outIndex = args.indexOf('--out');
const out = resolve(outIndex >= 0 ? args.splice(outIndex, 2)[1] : join(web, 'e2e-output'));
const discFiles = args.map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: garage-check.mjs <disc file ...> [--out dir]'); process.exit(2); }
mkdirSync(out, { recursive: true });
const serverDisc = discFiles.find((f) => /\.cue$/i.test(f)) ?? discFiles[0];
const { base, stop } = await startServer({ disc: serverDisc, port: 4196, db: join(out, 'garage.db') });
const { check, report } = checks();
const errors = [];
const browser = await launchBrowser();

/** How many car spans the last built frame has. */
const carSpans = (page) => page.evaluate(() => {
  const { fields } = window.__race.rage.spans();
  let n = 0;
  for (let i = 0; i + 14 <= fields.length; i += 14) if (fields[i + 3] === 0) n++;
  return n;
});

/** The paint colours of the car spans in the last built frame. */
const paintsDrawn = (page) => page.evaluate(() => {
  const { rage } = window.__race;
  const { fields } = rage.spans();
  const seen = new Set();
  for (let i = 0; i + 14 <= fields.length; i += 14) if (fields[i + 3] === 0 && fields[i + 7]) seen.add(`${fields[i + 8]},${fields[i + 9]}`);
  return [...seen];
});

try {
  const owner = await openPlayer(browser, { base, name: 'rage', password: 'racer', discFiles, onError: (e) => errors.push(e) });
  await owner.click('[data-action=garage]');
  await owner.waitForSelector('#garage:not([hidden])');
  await until(() => true, 1);
  await owner.waitForFunction(() => document.querySelector('#garage-status').textContent === '', null, { timeout: 60_000 });
  await wait(600);
  check(await carSpans(owner) > 0, `the preview draws the car (${await carSpans(owner)} spans)`);
  check(await paintsDrawn(owner).then((p) => p.length === 0), 'the factory car is drawn without paint');
  const factory = await owner.locator('#garage-view').screenshot({ path: join(out, 'garage-factory.png') });

  await owner.click('#paint-first .swatch:nth-child(10)'); // colour index 9
  await owner.click('#paint-second .swatch:nth-child(4)'); // colour index 3
  await wait(600);
  check((await paintsDrawn(owner)).join('|') === '9,3', `the preview draws the chosen colours (${await paintsDrawn(owner)})`);
  const painted = await owner.locator('#garage-view').screenshot({ path: join(out, 'garage-painted.png') });
  await owner.screenshot({ path: join(out, 'garage-page.png') });
  check(!factory.equals(painted), 'the painted preview looks different from the factory one');
  check(await owner.locator('#garage-save').isEnabled(), 'an unsaved change enables Save');
  await owner.click('#garage-save');
  await owner.waitForFunction(() => document.querySelector('#garage-status').textContent.startsWith('Saved'), null, { timeout: 10_000 });
  check(await owner.locator('#garage-save').isDisabled(), 'Save is off once saved');

  // A different grade of the same car shares the paint.
  const options = await owner.locator('#garage-car option').evaluateAll((all) => all.map((o) => ({ value: o.value, text: o.textContent })));
  check(options.length >= 13, `the garage lists the cars (${options.length})`);
  await owner.selectOption('#garage-car', options[options.length - 1].value);
  await owner.waitForFunction(() => !document.querySelector('#garage-status').textContent.startsWith('Loading'), null, { timeout: 60_000 });
  await wait(400);
  check((await paintsDrawn(owner)).join('|') === '', 'another car model starts with its factory colours');
  check(await owner.locator('#paint-first .swatch:not([disabled])').count() === 0 &&
        await owner.locator('#garage-factory').isDisabled(), 'the last three cars cannot be repainted (retail)');
  await owner.selectOption('#garage-car', options[0].value);
  await owner.waitForFunction(() => document.querySelector('#garage-status').textContent === '', null, { timeout: 60_000 });
  await wait(400);
  check((await paintsDrawn(owner)).join('|') === '9,3', 'going back to the car shows its saved paint');

  // Many combinations in a row: the renderer recycles its texture layers, so
  // the last one still shows.
  for (let i = 0; i < 60; i++) {
    await owner.click(`#paint-first .swatch:nth-child(${1 + (i * 5) % 18})`);
    await owner.click(`#paint-second .swatch:nth-child(${1 + (i * 7) % 18})`);
  }
  const lastFirst = (59 * 5) % 18, lastSecond = (59 * 7) % 18;
  await wait(600);
  check((await paintsDrawn(owner)).join('|') === `${lastFirst},${lastSecond}`, 'after sixty repaints the preview shows the last one');
  await owner.click('#paint-first .swatch:nth-child(10)');
  await owner.click('#paint-second .swatch:nth-child(4)');

  // Every car really changes: painting it alters the pixels of its materials.
  const { stuck: unchanged, erriso } = await owner.evaluate(() => {
    const { rage } = window.__race;
    const carTextures = () => {
      const { fields } = rage.spans();
      const list = [];
      for (let i = 0; i * 14 + 14 <= fields.length; i++) {
        const texture = fields[i * 14 + 3] === 0 && rage.decodeTexture(i);
        if (texture) list.push(Uint8Array.from(texture.levels[0].data));
      }
      return list;
    };
    const stuck = [];
    let erriso = 0;
    for (const variant of rage.garageVariants().filter((v) => rage.modelOf(v) < 10)) {
      rage.startShowroom(variant);
      rage.setShowroomPaint(null); rage.buildShowroom(1.6, 30);
      const plain = carTextures();
      rage.setShowroomPaint([9, 3]); rage.buildShowroom(1.6, 30);
      const painted = carTextures();
      const changed = painted.filter((texture, i) => plain[i] && texture.some((byte, k) => byte !== plain[i][k])).length;
      if (!changed) stuck.push(variant);
      if (variant === 0) erriso = changed;
    }
    return { stuck, erriso };
  });
  check(erriso >= 9, `the Erriso's roof is painted with its body (${erriso} of its materials change)`);
  check(unchanged.length === 0, `painting changes the pixels of every car (unchanged: ${unchanged.join(', ') || 'none'})`);

  // The server keeps it and validates it.
  const token = await owner.evaluate(() => localStorage.getItem('rage-racer.session'));
  const api = async (method, body) => {
    const response = await fetch(`${base}/api/garage`, { method, headers: { authorization: `Bearer ${token}`, 'content-type': 'application/json' }, body: body && JSON.stringify(body) });
    return { status: response.status, body: await response.json() };
  };
  const paints = (await api('GET')).body.paints;
  check(Object.values(paints).length === 1 && paints[Object.keys(paints)[0]].join() === '9,3', 'the server stores the paint');
  check((await api('PUT', { model: 0, paint: [18, 0] })).status === 400, 'a colour outside the catalogue is refused');
  check((await api('PUT', { model: 99, paint: [0, 0] })).status === 400, 'an unknown car is refused');
  check((await api('PUT', { model: 10, paint: [0, 0] })).status === 400, 'a car retail never let you repaint is refused');
  check((await api('PUT', { model: 0, paint: [1] })).status === 400, 'a paint with one colour is refused');

  // In a race, the other player's client draws the painted car.
  const rival = await openPlayer(browser, { base, discFiles, onError: (e) => errors.push(e) });
  await rival.click('[data-action=create-room]');
  await rival.fill('#settings-form input[name=name]', 'paint');
  await rival.selectOption('#settings-form select[name=laps]', '1');
  await rival.selectOption('#settings-form select[name=maxPlayers]', '2');
  await rival.uncheck('#settings-form input[name=rivals]');
  await rival.click('#settings-submit');
  await owner.click('#garage [data-action=garage-back]');
  const joinButton = owner.locator('#room-list li', { hasText: 'paint' }).locator('button');
  await joinButton.waitFor();
  await joinButton.click();
  await owner.waitForSelector('#room:not([hidden])');
  // The owner's car: the one whose paint was saved (model of options[0]).
  await owner.selectOption('#car-model', options[0].value).catch(() => {});
  await owner.click('#ready-button');
  await rival.waitForSelector('#start-button:not([disabled])');
  await rival.click('#start-button');
  for (const p of [owner, rival]) await p.waitForFunction(() => window.__race?.rage.hud().phase === 2, null, { timeout: 120_000 });
  await wait(1500);
  const seenByRival = await paintsDrawn(rival);
  check(seenByRival.includes('9,3'), `the other player sees the painted car in the race (${seenByRival})`);
  await rival.screenshot({ path: join(out, 'garage-race.png') });
} finally {
  await browser.close();
  stop();
}
if (errors.length) { console.error(errors.join('\n')); process.exitCode = 1; }
report('garage ok');
