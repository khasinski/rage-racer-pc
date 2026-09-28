// Checks a running deployment from a real browser, over the real network:
//   node scripts/live-check.mjs <server url> <disc file ...>
// A guest races alone twice, once with the WebRTC data channel and once
// forced onto the WebSocket, and each time reports which transport carried
// the race, the round trips and how evenly the frames arrived. (It leaves a
// guest account and a finished solo race on the server.)
import { resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, wait } from './lib/harness.mjs';

const [base, ...files] = process.argv.slice(2);
const discFiles = files.map((file) => resolve(file));
if (!base || !discFiles.length) { console.error('usage: live-check.mjs <server url> <disc file ...>'); process.exit(2); }
const { check, report } = checks();
const percentile = (values, p) => [...values].sort((a, b) => a - b)[Math.min(values.length - 1, Math.floor(p * values.length))] ?? 0;

/* Timestamps every race message as the page receives it, on either transport,
 * and remembers the peer connection for its statistics. */
const instrument = () => {
  window.__stamps = [];
  const Socket = window.WebSocket;
  window.WebSocket = class extends Socket {
    constructor(...args) {
      super(...args);
      this.addEventListener('message', (e) => { if (typeof e.data !== 'string') window.__stamps.push(performance.now()); });
    }
  };
  const createChannel = RTCPeerConnection.prototype.createDataChannel;
  RTCPeerConnection.prototype.createDataChannel = function (...args) {
    const channel = createChannel.apply(this, args);
    channel.addEventListener('message', (e) => { if (new Uint8Array(e.data)[0] !== 3) window.__stamps.push(performance.now()); });
    return channel;
  };
  const Peer = window.RTCPeerConnection;
  window.RTCPeerConnection = class extends Peer {
    constructor(...args) { super(...args); window.__pc = this; }
  };
};

const browser = await launchBrowser();
const results = {};
try {
  for (const transport of ['rtc', 'ws']) {
    const errors = [];
    const page = await openPlayer(browser, {
      base, discFiles, onError: (e) => errors.push(e), init: instrument, query: transport === 'ws' ? '?transport=ws' : '',
    });
    // The channel opens some moments after the connection.
    await page.waitForFunction(() => window.__transport?.() != null, null, { timeout: 15_000 });
    if (transport === 'rtc') await page.waitForFunction(() => window.__transport() === 'rtc', null, { timeout: 20_000 }).catch(() => {});
    await page.click('[data-action=create-room]');
    await page.fill('#settings-form input[name=name]', `live check ${transport}`);
    await page.selectOption('#settings-form select[name=laps]', '1');
    await page.selectOption('#settings-form select[name=maxPlayers]', '1');
    await page.uncheck('#settings-form input[name=rivals]');
    await page.click('#settings-submit');
    await page.waitForSelector('#room:not([hidden])', { timeout: 15_000 }).catch(async () => {
      throw new Error(`${transport}: no room appeared (lobby says “${await page.locator('#lobby-status').textContent()}”, transport ${await page.evaluate(() => window.__transport?.())}, errors ${errors.join('; ')})`);
    });
    await page.waitForSelector('#start-button:not([disabled])');
    await page.click('#start-button');
    await page.waitForFunction(() => window.__race?.rage.hud().phase === 2, null, { timeout: 120_000 });
    await page.keyboard.down('KeyX');
    await wait(2000);
    await page.evaluate(() => { window.__stamps.length = 0; });
    await wait(15_000);
    const measured = await page.evaluate(async () => {
      const stamps = window.__stamps;
      let iceRtt = null;
      if (window.__pc) {
        for (const report of (await window.__pc.getStats()).values()) {
          if (report.type === 'candidate-pair' && report.nominated && report.currentRoundTripTime != null) iceRtt = report.currentRoundTripTime * 1000;
        }
      }
      const pings = [...document.body.innerText.matchAll(/(\d+) ms/g)].map((m) => Number(m[1]));
      return { transport: window.__transport(), stamps, iceRtt, pings };
    });
    const gaps = measured.stamps.slice(1).map((t, i) => t - measured.stamps[i]);
    results[transport] = {
      transport: measured.transport, frames: measured.stamps.length,
      gapMean: gaps.reduce((a, b) => a + b, 0) / Math.max(1, gaps.length), gapP99: percentile(gaps, 0.99), gapMax: Math.max(0, ...gaps),
      iceRtt: measured.iceRtt, serverPing: measured.pings.at(-1) ?? null,
    };
    check(measured.transport === transport, `${transport}: the race ran over ${measured.transport}`);
    check(errors.length === 0, `${transport}: no page errors ${errors.join('; ')}`);
    await page.keyboard.up('KeyX');
    await page.keyboard.press('Escape'); // give up the solo race
    await wait(500);
    await page.context().close();
  }
} finally {
  await browser.close();
}
const f = (x, unit = 'ms') => (x == null ? 'n/a' : `${x.toFixed(1)} ${unit}`);
for (const [name, r] of Object.entries(results)) {
  console.log(`${name.padEnd(4)} ${r.frames} frames in 15 s, gaps mean ${f(r.gapMean)} p99 ${f(r.gapP99)} max ${f(r.gapMax)}; server ping ${r.serverPing ?? 'n/a'} ms, data channel round trip ${f(r.iceRtt)}`);
}
report('live ok');
