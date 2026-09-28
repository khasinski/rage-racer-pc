// The disc's CD audio (Red Book) tracks: where each lies in the BIN files the
// player dropped, and which one the race plays. Audio sectors are raw 44.1 kHz
// 16-bit little-endian stereo, 2352 bytes each, so a track is played straight
// from its file without any decoding. Without the audio BINs there is simply
// no music.

export interface DiscFile {
  readonly name: string;
  readonly size: number;
  slice(start?: number, end?: number): Blob;
}

export interface AudioTrack {
  number: number;
  file: DiscFile;
  /** Byte range of the track's INDEX 01 onwards within file. */
  offset: number;
  length: number;
}

const SECTOR = 2352;
const baseName = (path: string) => (path.split(/[\\/]/).pop() ?? path).toLowerCase();

function msf(text: string): number {
  const [m, s, f] = text.split(':').map(Number);
  return (m * 60 + s) * 75 + f;
}

/** Audio tracks a cue sheet describes, located in the given files. */
export function cueAudioTracks(cue: string, files: DiscFile[]): Map<number, AudioTrack> {
  type Entry = { number: number; audio: boolean; file: DiscFile | null; start: number; index0: number };
  const entries: Entry[] = [];
  let file: DiscFile | null = null;
  for (const raw of cue.split(/\r?\n/)) {
    const line = raw.trim();
    const named = /^FILE\s+"([^"]+)"/i.exec(line) ?? /^FILE\s+(\S+)/i.exec(line);
    if (named) {
      file = files.find((f) => baseName(f.name) === baseName(named[1])) ?? null;
      continue;
    }
    const track = /^TRACK\s+(\d+)\s+(\S+)/i.exec(line);
    if (track) {
      entries.push({ number: Number(track[1]), audio: track[2].toUpperCase() === 'AUDIO', file, start: -1, index0: -1 });
      continue;
    }
    const index = /^INDEX\s+(\d+)\s+(\d+:\d+:\d+)/i.exec(line);
    const current = entries[entries.length - 1];
    if (index && current) {
      if (Number(index[1]) === 1) current.start = msf(index[2]) * SECTOR;
      else if (Number(index[1]) === 0) current.index0 = msf(index[2]) * SECTOR;
    }
  }
  const tracks = new Map<number, AudioTrack>();
  entries.forEach((entry, i) => {
    if (!entry.audio || !entry.file || entry.start < 0) return;
    const next = entries[i + 1];
    const end = next && next.file === entry.file
      ? (next.index0 >= 0 ? next.index0 : next.start) : entry.file.size;
    if (end > entry.start) tracks.set(entry.number, { number: entry.number, file: entry.file, offset: entry.start, length: end - entry.start });
  });
  return tracks;
}

/** Without a cue sheet: Redump-style "(Track NN).bin" files, each audio
 *  track opening with its two-second pregap. */
function namedAudioTracks(files: DiscFile[]): Map<number, AudioTrack> {
  const tracks = new Map<number, AudioTrack>();
  for (const file of files) {
    const match = /track\s*0*(\d+)\)?\.(bin|img)$/i.exec(file.name);
    const number = match ? Number(match[1]) : 0;
    const pregap = 150 * SECTOR;
    if (number >= 2 && file.size > pregap) tracks.set(number, { number, file, offset: pregap, length: file.size - pregap });
  }
  return tracks;
}

/** CD audio tracks among whatever the player dropped. */
export async function discAudioTracks(files: DiscFile[]): Promise<Map<number, AudioTrack>> {
  const cue = files.find((f) => f.name.toLowerCase().endsWith('.cue'));
  if (cue) {
    const tracks = cueAudioTracks(await cue.slice().text(), files);
    if (tracks.size) return tracks;
  }
  return namedAudioTracks(files);
}

/** race/bgm_track.c: the ten race tunes are CD tracks 3..12, except that
 *  the tenth is track 17 (track 12 is the results music). */
export const BGM_TRACK_COUNT = 10;
export function bgmCdTrack(selected: number): number {
  const track = selected + 3;
  return track === 12 ? 17 : track;
}

/** Reads length bytes of a track from byteOffset (sector-aligned by the
 *  caller) as 16-bit stereo samples. */
export async function readTrackPcm(track: AudioTrack, byteOffset: number, length: number): Promise<Int16Array> {
  const start = track.offset + Math.min(byteOffset, track.length);
  const end = track.offset + Math.min(byteOffset + length, track.length);
  const bytes = await track.file.slice(start, end).arrayBuffer();
  return new Int16Array(bytes, 0, bytes.byteLength >> 1);
}
