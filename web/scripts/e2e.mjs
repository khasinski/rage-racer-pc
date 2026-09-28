// End-to-end check of the whole app in real (headless) browsers:
//   node scripts/e2e.mjs <disc file ...> [--out dir]
// Starts the multiplayer server with a scratch database and the development
// test accounts, then two players log in, choose the disc through
// the page's file picker, meet in a room, race each other on the server, leave
// and read the results; one of them then drives an offline practice race.
// Fails on any page error, a blank frame or a step that does not happen.
import { mkdirSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, startServer, web } from './lib/harness.mjs';

const args = process.argv.slice(2);
const outIndex = args.indexOf('--out');
const out = resolve(outIndex >= 0 ? args.splice(outIndex, 2)[1] : join(web, 'e2e-output'));
const discFiles = args.map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: e2e.mjs <disc file ...> [--out dir]'); process.exit(2); }
mkdirSync(out, { recursive: true });

// The server reads the Track 01 BIN (or the CUE) the players will choose.
const serverDisc = discFiles.find((f) => /\.cue$/i.test(f)) ?? discFiles[0];
const { base, stop } = await startServer({ disc: serverDisc, port: 4180, db: join(out, 'e2e.db') });

const { failures, report } = checks();
const step = (text) => console.log(`· ${text}`);
const browser = await launchBrowser();

// Tallies what the page hands its audio worklet (web/src/audio.ts): race
// effects PCM, streamed CD music, and whether the output is running.
function tallyAudio() {
  const tally = { sfxFrames: 0, sfxPeak: 0, musicFrames: 0, contexts: [] };
  window.__audio = tally;
  const post = MessagePort.prototype.postMessage;
  MessagePort.prototype.postMessage = function (message, ...rest) {
    if (message && (message.type === 'sfx' || message.type === 'music')) {
      const pcm = message.pcm;
      if (message.type === 'sfx') {
        tally.sfxFrames += pcm.length / 2;
        for (let i = 0; i < pcm.length; i++) tally.sfxPeak = Math.max(tally.sfxPeak, Math.abs(pcm[i]));
      } else tally.musicFrames += pcm.length / 2;
    }
    return post.call(this, message, ...rest);
  };
  const Base = window.AudioContext;
  window.AudioContext = class extends Base {
    constructor(...args) { super(...args); tally.contexts.push(this); }
  };
}

const player = (name, password) => openPlayer(browser, { base, name, password, discFiles, consoleErrors: true,
  init: tallyAudio, onError: (error) => failures.push(error) });

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
// The race state the HUD shows (web/src/rage.ts Hud), read from the page.
const hud = (page) => page.evaluate(() => window.__race.rage.hud());

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
    if (!(h.speed > 0)) failures.push(`${name}'s car did not move`);
    if (h.entrants !== 12) failures.push(`${name} does not see the full field: ${h.entrants}`);
  }
  if (!(hudAdmin.timeMs > 0)) failures.push('the race clock did not run');
  for (const [name, page] of [['admin', admin], ['rage', rage]]) if (await blank(page)) failures.push(`${name}'s race view is blank`);
  const withMusic = discFiles.some((f) => /\.cue$/i.test(f)) && discFiles.length > 2;
  for (const [name, page] of [['admin', admin], ['rage', rage]]) {
    const sound = await page.evaluate(() => ({ ...window.__audio, contexts: window.__audio.contexts.map((c) => c.state) }));
    step(`${name} audio: ${JSON.stringify(sound)}`);
    if (!sound.contexts.includes('running')) failures.push(`${name}'s audio output is not running`);
    if (!(sound.sfxFrames > 44100 * 4 && sound.sfxPeak > 1000)) failures.push(`${name} hears no race sound`);
    if (withMusic && !(sound.musicFrames > 44100)) failures.push(`${name} hears no music`);
  }
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
  if (!(practice.speed > 0)) failures.push('the practice car did not move');
  step(`practice race ran (${JSON.stringify(practice)})`);
} catch (error) {
  failures.push(String(error));
} finally {
  await browser.close();
  stop();
}
report(`ok — screenshots in ${out}`);
