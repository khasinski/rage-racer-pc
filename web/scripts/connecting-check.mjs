// A message sent while the WebSocket is still opening is not lost: creating a
// room the moment the lobby shows works even when the connection is slow.
//   node scripts/connecting-check.mjs <disc file ...>
import { join, resolve } from 'node:path';
import { checks, launchBrowser, startServer, wait, web } from './lib/harness.mjs';

const discFiles = process.argv.slice(2).map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: connecting-check.mjs <disc file ...>'); process.exit(2); }
const { base, stop } = await startServer({ disc: discFiles[0], port: 4212, db: join(web, 'e2e-output', 'connecting.db') });
const { check, report } = checks();
const browser = await launchBrowser();
try {
  const context = await browser.newContext({ viewport: { width: 1280, height: 720 } });
  // The socket takes two seconds to open.
  await context.routeWebSocket(/\/ws\?/, async (ws) => {
    await wait(2000);
    const server = ws.connectToServer();
    ws.onMessage((m) => server.send(m));
    server.onMessage((m) => ws.send(m));
    ws.onClose((code, reason) => server.close({ code, reason }));
    server.onClose((code, reason) => ws.close({ code, reason }));
  });
  const page = await context.newPage();
  await page.goto(`${base}/#e2e`);
  await page.waitForSelector('#auth:not([hidden])');
  await page.click('#auth button[value=guest]');
  await page.waitForSelector('#disc:not([hidden])');
  await page.setInputFiles('#disc-input', discFiles);
  await page.waitForSelector('#lobby:not([hidden])', { timeout: 180_000 });
  await page.click('[data-action=create-room]');
  await page.fill('#settings-form input[name=name]', 'early');
  await page.click('#settings-submit');
  const arrived = await page.waitForSelector('#room:not([hidden])', { timeout: 15_000 }).then(() => true, () => false);
  check(arrived, 'a room asked for while the connection was still opening is created');
} finally {
  await browser.close();
  stop();
}
report('connecting ok');
