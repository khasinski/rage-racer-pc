// A duel is a page link: the host shares it, the other driver opens it, and
// both race one on one in the car and on the course the server rolled.
//   node scripts/duel-check.mjs <disc file ...>
import { join, resolve } from 'node:path';
import { checks, launchBrowser, openPlayer, startServer, web } from './lib/harness.mjs';

const discFiles = process.argv.slice(2).map((file) => resolve(file));
if (!discFiles.length) { console.error('usage: duel-check.mjs <disc file ...>'); process.exit(2); }
const out = join(web, 'e2e-output');
const { base, stop } = await startServer({ disc: discFiles[0], port: 4213, db: join(out, 'duel.db') });
const { failures, check, report } = checks();
const browser = await launchBrowser();

try {
  const host = await openPlayer(browser, {
    base, name: 'admin', password: 'admin', discFiles, onError: (error) => failures.push(error),
  });
  await host.click('[data-action=create-duel]');
  await host.waitForSelector('#duel-invite:not([hidden])');
  const link = await host.locator('#duel-link').inputValue();
  const token = new URL(link).searchParams.get('duel');
  const hostCar = await host.locator('#duel-car').innerText();
  const hostCourse = await host.locator('#room-summary').innerText();
  check(Boolean(token) && link === `${base}/?duel=${token}`, `the invite is a link to this page (${link})`);
  check(hostCar.startsWith('Both drivers:'), `the rolled car is shown (${hostCar})`);
  check(await host.locator('#car-model-field').isHidden(), 'the rolled car cannot be swapped');
  check(await host.locator('#car-transmission').isVisible() && await host.locator('#car-tires').isVisible(),
        'the host picks a gearbox and tires');
  check(await host.locator('#ready-button').isVisible(), 'the host confirms ready too');
  check(await host.locator('[data-action=edit-room]').isHidden(), 'the course cannot be edited');
  check(await host.locator('#spectate-button').isHidden(), 'a duel is the two drivers');
  check(await host.locator('#start-button').isDisabled(), 'the host waits for the other driver');
  await host.screenshot({ path: join(out, 'duel-room.png') });

  await host.setViewportSize({ width: 390, height: 844 });
  const box = await host.locator('#duel-link').boundingBox();
  const fits = box !== null && box.x >= 0 && box.y >= 0 && box.x + box.width <= 390 && box.width > 40;
  check(fits, `the invite link fits a phone (${box ? `${Math.round(box.width)}px at ${Math.round(box.x)},${Math.round(box.y)}` : 'missing'})`);
  await host.screenshot({ path: join(out, 'duel-room-phone.png') });
  await host.setViewportSize({ width: 1280, height: 720 });

  const guest = await openPlayer(browser, {
    base, name: 'rage', password: 'racer', discFiles, query: `?duel=${token}`,
    ready: '#room:not([hidden])',
    onError: (error) => failures.push(error),
  });
  const guestCar = await guest.locator('#duel-car').innerText();
  const guestCourse = await guest.locator('#room-summary').innerText();
  check(guestCar === hostCar && guestCourse === hostCourse,
        `the link joins the same car and course (${guestCar} · ${guestCourse})`);
  check(await guest.locator('#car-model-field').isHidden() && await guest.locator('#car-tires').isVisible()
        && await guest.locator('#ready-button').isVisible(),
        'the opponent picks tires and confirms ready');
  await guest.selectOption('#car-tires', '3');
  await guest.click('#ready-button');
  await host.waitForFunction(() => document.querySelector('#room-status')?.textContent === 'Waiting for admin.');
  check(await host.locator('#start-button').isDisabled(), 'one driver ready does not start the duel');
  await host.selectOption('#car-transmission', 'manual');
  await host.selectOption('#car-tires', '1');
  await host.click('#ready-button');
  await host.waitForSelector('#start-button:not([disabled])');
  await host.click('#start-button');
  await Promise.all([host, guest].map((page) => page.waitForSelector('#view:not([hidden])', { timeout: 120_000 })));
  check(true, 'both drivers reach the race');
} finally {
  await browser.close();
  stop();
}
report('duel ok');
