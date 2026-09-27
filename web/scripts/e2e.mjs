// End-to-end check in a real (headless) browser:
//   node scripts/e2e.mjs <disc file ...> [--out dir]
// Serves dist/ with `vite preview`, chooses the disc through the page's file
// picker exactly as a player would, starts a race, drives with the keyboard
// and saves screenshots. Fails on any page error or a blank frame.
import { spawn } from 'node:child_process';
import { mkdirSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { chromium } from 'playwright';

const web = join(dirname(fileURLToPath(import.meta.url)), '..');
const args = process.argv.slice(2);
const outIndex = args.indexOf('--out');
const out = resolve(outIndex >= 0 ? args.splice(outIndex, 2)[1] : join(web, 'e2e-output'));
const discFiles = args.map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: e2e.mjs <disc file ...> [--out dir]'); process.exit(2); }
mkdirSync(out, { recursive: true });

const port = 4179;
const server = spawn(process.execPath, [join(web, 'node_modules/vite/bin/vite.js'), 'preview',
  '--port', String(port), '--strictPort'], { cwd: web, stdio: ['ignore', 'pipe', 'inherit'] });
await new Promise((ready) => server.stdout.on('data', (d) => String(d).includes(String(port)) && ready()));

const failures = [];
const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
try {
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  page.on('pageerror', (error) => failures.push(`page error: ${error.message}`));
  page.on('console', (message) => {
    if (message.type() === 'error') failures.push(`console: ${message.text()}`);
  });
  await page.goto(`http://localhost:${port}/#e2e`);
  await page.screenshot({ path: join(out, '01-disc.png') });

  await page.setInputFiles('#disc-input', discFiles);
  await page.waitForSelector('#setup:not([hidden])', { timeout: 120_000 });
  await page.screenshot({ path: join(out, '02-setup.png') });

  await page.click('#start');
  await page.waitForSelector('#view:not([hidden])', { timeout: 120_000 });
  await page.waitForTimeout(1500);
  await page.screenshot({ path: join(out, '03-countdown.png') });

  // Desktop keys (input_config.c): X is cross (accelerate), S triangle
  // (camera), Enter start (pause), the arrows the D-pad.
  const raceTime = () => page.evaluate(() => document.getElementById('hud-time')?.textContent);
  await page.keyboard.down('KeyX');
  await page.waitForTimeout(6000);
  await page.screenshot({ path: join(out, '04-car-view.png') });
  await page.keyboard.press('KeyS');
  await page.keyboard.down('ArrowLeft');
  await page.waitForTimeout(1200);
  await page.keyboard.up('ArrowLeft');
  await page.waitForTimeout(3000);
  await page.screenshot({ path: join(out, '05-chase-view.png') });
  await page.keyboard.down('ArrowDown');
  await page.waitForTimeout(600);
  await page.screenshot({ path: join(out, '06-look-behind.png') });
  await page.keyboard.up('ArrowDown');
  await page.keyboard.press('Enter');
  await page.waitForTimeout(200);
  const pausedAt = await raceTime();
  await page.waitForTimeout(1000);
  if (await raceTime() !== pausedAt) failures.push('Enter did not pause the race');
  await page.keyboard.press('Enter');
  await page.waitForTimeout(1000);
  if (await raceTime() === pausedAt) failures.push('Enter did not resume the race');
  await page.keyboard.up('KeyX');

  const hud = await page.evaluate(() => ({
    place: document.getElementById('hud-place')?.textContent,
    lap: document.getElementById('hud-lap')?.textContent,
    time: document.getElementById('hud-time')?.textContent,
    speed: document.getElementById('hud-speed')?.textContent,
    gear: document.getElementById('hud-gear')?.textContent,
  }));
  console.log('hud', JSON.stringify(hud));
  if (!(Number(hud.speed) > 0)) failures.push('X did not accelerate the car');
  const blank = await page.evaluate(() => {
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
  if (blank) failures.push('race view is blank');
  if (!hud.time || hud.time === '0:00.00') failures.push('race clock did not advance');
} finally {
  await browser.close();
  server.kill();
}
if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log(`ok — screenshots in ${out}`);
