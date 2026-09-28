// Picks the disc's data track from whatever the player dropped or browsed:
// a Track 01 BIN on its own, a CUE together with its BINs, or the whole disc
// folder. The CUE names the data track; without one, the Track 01 (or only)
// BIN is used. Nothing else is ever asked of the player.

type DiscChoice = { file: File } | { error: string };

const lower = (name: string) => name.toLowerCase();
const isCue = (file: File) => lower(file.name).endsWith('.cue');
const isBin = (file: File) => /\.(bin|img)$/.test(lower(file.name));

/** File name of the data track (TRACK 01) referenced by a cue sheet. */
function cueDataTrack(cue: string): string | null {
  let current: string | null = null;
  for (const raw of cue.split(/\r?\n/)) {
    const line = raw.trim();
    const file = /^FILE\s+"([^"]+)"/i.exec(line) ?? /^FILE\s+(\S+)/i.exec(line);
    if (file) current = file[1];
    else if (/^TRACK\s+0*1\s+/i.test(line)) return current;
  }
  return null;
}

const baseName = (path: string) => path.split(/[\\/]/).pop() ?? path;

export async function chooseDataTrack(files: File[]): Promise<DiscChoice> {
  const cues = files.filter(isCue);
  const bins = files.filter(isBin);
  if (cues.length) {
    const wanted = cueDataTrack(await cues[0].text());
    if (!wanted) return { error: `${cues[0].name} does not describe a data track.` };
    const match = bins.find((bin) => lower(bin.name) === lower(baseName(wanted)));
    if (match) return { file: match };
    return { error: `${cues[0].name} refers to “${baseName(wanted)}”. Drop it together with the CUE, or drop the whole disc folder.` };
  }
  if (!bins.length) return { error: 'Choose your Rage Racer CUE file or its Track 01 BIN image.' };
  const trackOne = bins.find((bin) => /track\s*0*1\b/.test(lower(bin.name)));
  return { file: trackOne ?? bins.reduce((a, b) => (b.size > a.size ? b : a)) };
}

async function entryFiles(entry: FileSystemEntry): Promise<File[]> {
  if (entry.isFile) {
    return [await new Promise<File>((resolve, reject) => (entry as FileSystemFileEntry).file(resolve, reject))];
  }
  if (!entry.isDirectory) return [];
  const reader = (entry as FileSystemDirectoryEntry).createReader();
  const children: FileSystemEntry[] = [];
  for (;;) {
    const batch = await new Promise<FileSystemEntry[]>((resolve, reject) => reader.readEntries(resolve, reject));
    if (!batch.length) break;
    children.push(...batch);
  }
  return (await Promise.all(children.map(entryFiles))).flat();
}

/** Files from a drop, descending into dropped folders. */
export async function droppedFiles(transfer: DataTransfer): Promise<File[]> {
  const entries = Array.from(transfer.items)
    .map((item) => item.webkitGetAsEntry?.())
    .filter((entry): entry is FileSystemEntry => entry != null);
  if (entries.length) return (await Promise.all(entries.map(entryFiles))).flat();
  return Array.from(transfer.files);
}
