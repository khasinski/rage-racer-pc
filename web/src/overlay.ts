// The race view's overlay: the event feed, the online standings, the finish
// deadline, and the camera that follows another car once yours is gone (or
// when you only watch).
import type { Transport } from '../shared/protocol.ts';
import { SEAT_FINISHED, SEAT_RACING, SEAT_RETIRED } from './constants';
import { consumeKey } from './input';
import type { Rage } from './rage';
import type { RaceState } from './race';
import { $, el, formatTime, pingCell, textWriter } from './views';

const feedList = $<HTMLOListElement>('feed');
const standings = $<HTMLOListElement>('standings');
const spectating = $('spectating');
const deadline = $('deadline');
const setDeadline = textWriter(deadline);
/* The name the spectating label shows. */
let watched: string | null = null;

/** A line in the event feed (bottom left) for a few seconds. */
export function feed(text: string): void {
  const item = el('li', { textContent: text });
  feedList.append(item);
  setTimeout(() => item.remove(), 6000);
  while (feedList.children.length > 5) feedList.firstElementChild!.remove();
}

export function resetOverlay(race: RaceState): void {
  feedList.replaceChildren();
  standings.replaceChildren();
  standings.hidden = race.mode === 'offline';
  spectating.hidden = deadline.hidden = true;
  watched = null;
}

const isPlayer = (race: RaceState, seat: number) => seat < race.humans;

/* Cars still racing, in race order: players first when `playersFirst`. */
function racingSeats(rage: Rage, race: RaceState, playersFirst: boolean): { seat: number; place: number; player: boolean }[] {
  const seats = [];
  for (let seat = 0; seat < race.names.length; seat++) {
    const standing = rage.standing(seat);
    if (standing.status !== SEAT_RACING || rage.seatGone(seat)) continue;
    seats.push({ seat, place: standing.place || 99, player: isPlayer(race, seat) });
  }
  return seats.sort((a, b) => (playersFirst ? Number(b.player) - Number(a.player) : 0) || a.place - b.place);
}

/** Once your car has finished and left the picture (or retired, or you only
 *  watch), the camera follows the player nearest to you in the race; with
 *  no player left, the nearest rival. A followed car that leaves the
 *  picture hands over the same way. */
export function followRace(rage: Rage, race: RaceState): void {
  const own = race.localSeat;
  const view = rage.viewSeat();
  if (race.mode !== 'spectator' && !rage.seatGone(own)) {
    if (view !== own) rage.setViewSeat(own);
  } else if (view === own || rage.seatGone(view)) {
    const reference = rage.standing(race.mode === 'spectator' ? view : own).place || 1;
    const candidates = racingSeats(rage, race, true);
    const pool = candidates.some((c) => c.player) ? candidates.filter((c) => c.player) : candidates;
    pool.sort((a, b) => Math.abs(a.place - reference) - Math.abs(b.place - reference));
    if (pool.length) rage.setViewSeat(pool[0].seat);
  }
  const name = rage.viewSeat() !== own ? race.names[rage.viewSeat()] ?? '' : null;
  if (name === watched) return;
  watched = name;
  spectating.hidden = name === null;
  if (name !== null) spectating.replaceChildren(`Watching ${name}`, el('span', { textContent: '  ← → switch car' }));
}

/** ←/→ (or the D-pad) pick the previous/next car in race order; true when
 *  the camera moved. */
export function spectateKeys(rage: Rage, race: RaceState): boolean {
  const previous = consumeKey('ArrowLeft');
  const next = consumeKey('ArrowRight');
  if ((!previous && !next) || rage.viewSeat() === race.localSeat) return false;
  const order = racingSeats(rage, race, false);
  if (!order.length) return false;
  const at = order.findIndex((c) => c.seat === rage.viewSeat());
  const step = next ? 1 : -1;
  rage.setViewSeat(order[(Math.max(at, 0) + step + order.length) % order.length].seat);
  return true;
}

/** The race closes this long after the first car finished (bottom left). */
export function drawDeadline(race: RaceState, now: number): void {
  const hidden = race.deadlineAt === null;
  if (deadline.hidden !== hidden) deadline.hidden = hidden;
  if (race.deadlineAt !== null) setDeadline(`Race closes in ${formatTime(Math.max(0, race.deadlineAt - now)).slice(0, -3)}`);
}

/** The online players in race order, styled like the event feed. */
export function drawStandings(rage: Rage, race: RaceState, latency: Record<number, number | null>,
                              transport: Record<number, Transport>): void {
  const rows = Array.from({ length: race.humans }, (_, seat) => ({ seat, ...rage.standing(seat) }))
    .sort((a, b) => (a.place || 99) - (b.place || 99) || a.seat - b.seat);
  standings.replaceChildren(...rows.map((row) => {
    const userId = race.userIds[row.seat];
    return el('li', { className: row.seat === race.localSeat ? 'me' : '' },
      el('b', {}, row.status === SEAT_RETIRED ? 'RET' : row.place ? String(row.place) : '–'),
      el('span', {}, race.names[row.seat]),
      el('span', { className: 'dim' },
        row.status === SEAT_FINISHED ? 'finished' : row.status === SEAT_RETIRED ? '' : `lap ${Math.max(1, row.lap)}`),
      pingCell(userId === null ? undefined : latency[userId], undefined, userId === null ? undefined : transport[userId]));
  }));
}
