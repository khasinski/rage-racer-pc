// What players send to dress their cars (the garage): checked here before it
// reaches the store. Each parser returns the value or an error for the player.
import { LOGO_BYTES, PAINT_COLORS, PAINTABLE_MODELS, type Paint } from '../shared/protocol.ts';

export type Parsed<T> = { ok: true; value: T } | { ok: false; error: string };
const ok = <T>(value: T): Parsed<T> => ({ ok: true, value });
const fail = <T>(error: string): Parsed<T> => ({ ok: false, error });

const isColour = (v: unknown) => Number.isInteger(v) && (v as number) >= 0 && (v as number) < PAINT_COLORS;

/** PUT /api/garage { model, paint }: a model that takes paint, and two
 *  catalogue colours or null for the factory ones. */
export function parsePaintRequest(input: Record<string, unknown>, carModels: number): Parsed<{ model: number; paint: Paint | null }> {
  const { model, paint } = input;
  if (!Number.isInteger(model) || (model as number) < 0 || (model as number) >= carModels) return fail('Unknown car.');
  if ((model as number) >= PAINTABLE_MODELS) return fail('This car cannot be repainted.');
  if (paint !== null && !(Array.isArray(paint) && paint.length === 2 && paint.every(isColour))) {
    return fail('Choose two colours from the paint catalogue.');
  }
  return ok({ model: model as number, paint: paint as Paint | null });
}

/** PUT /api/logo { logo }: base64 of exactly LOGO_BYTES, or null for none. */
export function parseLogoRequest(input: Record<string, unknown>): Parsed<Buffer | null> {
  const { logo } = input;
  if (logo === null) return ok(null);
  const bytes = typeof logo === 'string' && /^[A-Za-z0-9+/]+=*$/.test(logo) ? Buffer.from(logo, 'base64') : null;
  if (!bytes || bytes.length !== LOGO_BYTES) return fail('A logo is 64 by 64 pixels with a 16-colour palette.');
  return ok(bytes);
}
