// The race moves to the WebSocket when its data channel goes silent (a NAT
// forgetting the mapping, say), keeps running, and gets a channel back once the
// network allows; a page that stalls for a while (loading, a background tab)
// does not lose its channel for good either.
//   node scripts/rtc-fallback-check.mjs <disc file ...>
import { join, resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, startServer, wait, web } from './lib/harness.mjs';

const discFiles = process.argv.slice(2).map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: rtc-fallback-check.mjs <disc file ...>'); process.exit(2); }
const { base, stop } = await startServer({ disc: discFiles[0], port: 4214, db: join(web, 'e2e-output', 'rtc-fallback.db') });
const { check, report } = checks();
const errors = [];
const browser = await launchBrowser();
/* Counts the race frames each transport delivers to the page (before the
 * client's own handling), so a check reads what the network did. */
const countFrames = () => {
  window.__frames = { ws: 0, rtc: 0 };
  const Socket = window.WebSocket;
  window.WebSocket = class extends Socket {
    constructor(...args) {
      super(...args);
      this.addEventListener('message', (e) => { if (typeof e.data !== 'string') window.__frames.ws++; });
    }
  };
  // window.__cut drops the channel's traffic both ways, as a dead path would.
  window.__cut = false;
  const create = RTCPeerConnection.prototype.createDataChannel;
  RTCPeerConnection.prototype.createDataChannel = function (...args) {
    const channel = create.apply(this, args);
    channel.addEventListener('message', (e) => {
      if (window.__cut) return e.stopImmediatePropagation(); // before the client's own handler
      if (new Uint8Array(e.data)[0] !== 3) window.__frames.rtc++;
    });
    return channel;
  };
  const send = RTCDataChannel.prototype.send;
  RTCDataChannel.prototype.send = function (...args) { if (!window.__cut) send.apply(this, args); };
};
/** Frames each transport delivered in one second. */
const framesInOneSecond = (page) => page.evaluate(async () => {
  window.__frames.ws = window.__frames.rtc = 0;
  await new Promise((r) => setTimeout(r, 1000));
  return { ...window.__frames };
});
try {
  const page = await openPlayer(browser, { base, discFiles, onError: (e) => errors.push(e), init: countFrames });
  await page.waitForFunction(() => window.__transport?.() === 'rtc', null, { timeout: 20_000 });
  await page.click('[data-action=create-room]');
  await page.fill('#settings-form input[name=name]', 'fallback');
  await page.selectOption('#settings-form select[name=laps]', '1');
  await page.selectOption('#settings-form select[name=maxPlayers]', '1');
  await page.click('#settings-submit');
  await page.waitForSelector('#start-button:not([disabled])');
  await page.click('#start-button');
  await page.waitForFunction(() => window.__race?.rage.hud().phase === 2, null, { timeout: 120_000 });
  await wait(2000); // past the start's clock settling
  check(await page.evaluate(() => window.__transport()) === 'rtc', 'the race starts on the data channel');
  const before = await framesInOneSecond(page);
  check(before.rtc >= 20 && before.ws === 0, `frames arrive over it (${before.rtc} on the channel, ${before.ws} on the WebSocket)`);
  // Cut the channel in both directions.
  await page.evaluate(() => { window.__cut = true; });
  await wait(3500);
  check(await page.evaluate(() => window.__transport()) === 'ws', 'a silent channel is given up for the WebSocket');
  const after = await framesInOneSecond(page);
  check(after.ws >= 20, `and the frames come over the WebSocket (${after.ws} in a second, ${after.rtc} on the channel)`);
  const t0 = await page.evaluate(() => window.__race.rage.hud().tick);
  await wait(1000);
  check(await page.evaluate(() => window.__race.rage.hud().tick) - t0 >= 40, 'the local clock keeps up');
  const backOnChannel = (ms) => page.waitForFunction(() => window.__transport() === 'rtc', null, { timeout: ms }).then(() => true, () => false);
  // The network recovers: a new channel is offered and the race goes back to it.
  await page.evaluate(() => { window.__cut = false; });
  check(await backOnChannel(20_000), 'once the channel works again the race returns to it');
  const back = await framesInOneSecond(page);
  check(back.rtc >= 20, `with the frames (${back.rtc} on the channel, ${back.ws} on the WebSocket)`);
  // The page freezes for three seconds, as loading a race can.
  await page.evaluate(() => { const t = performance.now(); while (performance.now() - t < 3000) { /* stall */ } });
  check(await backOnChannel(15_000), 'a three-second stall does not lose the channel for good');
} finally {
  await browser.close();
  stop();
}
if (errors.length) { console.error(errors.join('\n')); process.exitCode = 1; }
report('rtc fallback ok');
