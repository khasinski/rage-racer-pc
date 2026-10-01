// The server's checks of what the garage sends (server/looks.ts).
//   node scripts/looks-test.mjs
import { LOGO_BYTES } from '../shared/protocol.ts';
import { parseLogoRequest, parsePaintRequest } from '../server/looks.ts';
import { checks } from './lib/harness.mjs';

const { check, report } = checks();
const MODELS = 13;
const paint = (input) => parsePaintRequest(input, MODELS);
check(paint({ model: 0, paint: [9, 3] }).ok && paint({ model: 0, paint: [9, 3] }).value.paint.join() === '9,3', 'two catalogue colours are taken');
check(paint({ model: 9, paint: null }).ok, 'null returns a car to its factory colours');
check(!paint({ model: 0, paint: [18, 0] }).ok, 'a colour past the catalogue is refused');
check(!paint({ model: 0, paint: [1.5, 0] }).ok && !paint({ model: 0, paint: ['1', 0] }).ok, 'colours must be whole numbers');
check(!paint({ model: 0, paint: [1] }).ok && !paint({ model: 0, paint: [1, 2, 3] }).ok, 'a paint has exactly two colours');
check(!paint({ model: 0 }).ok, 'a missing paint is refused (null clears it)');
check(paint({ model: 13, paint: [0, 0] }).error === 'Unknown car.' && !paint({ model: -1, paint: [0, 0] }).ok, 'an unknown model is refused');
check(paint({ model: 10, paint: [0, 0] }).error === 'This car cannot be repainted.', 'the special cars are refused');

const logo = (value) => parseLogoRequest({ logo: value });
const good = Buffer.alloc(LOGO_BYTES, 0x21).toString('base64');
check(logo(good).ok && logo(good).value.length === LOGO_BYTES, 'a logo of the right size is taken');
check(logo(null).ok && logo(null).value === null, 'null removes the logo');
check(!logo(Buffer.alloc(LOGO_BYTES - 1).toString('base64')).ok, 'a short logo is refused');
check(!logo('not base64!').ok && !logo(5).ok && !logo(undefined).ok, 'anything but base64 text or null is refused');
report('looks ok');
