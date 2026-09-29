// Nicknames hung above other players' cars. A plate shows only while that
// car is on screen and within a few of its own lengths of the car the
// camera is following; the last quarter of that distance fades it out.
// The renderer draws the plate in the world and lets the depth test hide
// it behind anything closer.

/** How many body-lengths count as close enough to read a name. */
export const NAMEPLATE_LENGTHS = 5;
/** Fraction of that distance over which the plate fades out. */
export const NAMEPLATE_FADE = 0.25;
/** Plate height in body-lengths. Width follows the painted name, so the
 *  letters keep their shape and the whole plate stays about as tall as the
 *  roof of the car. */
export const NAMEPLATE_HEIGHT = 0.18;

export interface CarMark {
  seat: number;
  name: string;
  /** Where the body was placed this frame, for the distance check. */
  origin: readonly [number, number, number];
  /** Point just above the roof. Absent when the body was not drawn. */
  anchor: readonly [number, number, number] | null;
  /** Longer horizontal side of the drawn body. 0 when it was not drawn. */
  length: number;
}

export interface Nameplate {
  seat: number;
  name: string;
  anchor: readonly [number, number, number];
  length: number;
  alpha: number;
}

/** Plates for the cars near `viewSeat`. The followed car itself is left out. */
export function selectNameplates(cars: readonly CarMark[], viewSeat: number): Nameplate[] {
  const view = cars.find((car) => car.seat === viewSeat);
  if (!view) return [];
  const plates: Nameplate[] = [];
  for (const car of cars) {
    if (car.seat === viewSeat || !car.anchor || !car.name.trim()) continue;
    const body = Math.max(car.length, view.length);
    if (!(body > 0)) continue;
    const range = body * NAMEPLATE_LENGTHS;
    const distance = Math.hypot(
      car.origin[0] - view.origin[0],
      car.origin[1] - view.origin[1],
      car.origin[2] - view.origin[2]);
    if (distance >= range) continue;
    const fade = range * NAMEPLATE_FADE;
    const alpha = distance <= range - fade ? 1 : (range - distance) / fade;
    plates.push({ seat: car.seat, name: car.name, anchor: car.anchor, length: car.length, alpha });
  }
  return plates;
}
