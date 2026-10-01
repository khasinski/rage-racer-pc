// Remembers the player's disc as a File System Access handle: permission to
// read that file or folder again. The image stays on their disk. IndexedDB
// stores the handle, not the bytes. Browsers without the API keep the plain
// file input and drag-and-drop.

import { droppedFiles } from './disc';

declare global {
  interface FileSystemHandle {
    queryPermission(descriptor?: { mode?: 'read' | 'readwrite' }): Promise<PermissionState>;
    requestPermission(descriptor?: { mode?: 'read' | 'readwrite' }): Promise<PermissionState>;
  }
  interface DataTransferItem {
    getAsFileSystemHandle?: () => Promise<FileSystemHandle | null>;
  }
  interface Window {
    showOpenFilePicker?: (options?: {
      multiple?: boolean;
      id?: string;
      excludeAcceptAllOption?: boolean;
      types?: { description?: string; accept: Record<string, string[]> }[];
    }) => Promise<FileSystemFileHandle[]>;
    showDirectoryPicker?: (options?: { id?: string; mode?: 'read' | 'readwrite' }) => Promise<FileSystemDirectoryHandle>;
  }
}

const DB_NAME = 'rage-racer-disc';
const STORE = 'handles';
const KEY = 'disc';
const READ = { mode: 'read' as const };
const PICKER_TYPES = [{
  description: 'Rage Racer disc',
  accept: { 'application/octet-stream': ['.bin', '.img', '.cue'] },
}];

/** The saved file or folder is no longer on disk. */
export class DiscGone extends Error {
  constructor() {
    super('That disc is no longer there. Drop it again, or browse for it.');
    this.name = 'DiscGone';
  }
}

export const canPickFiles = () => typeof window.showOpenFilePicker === 'function';
export const canPickDirectory = () => typeof window.showDirectoryPicker === 'function';

/** Name shown on the "use this disc again" button. */
export function discLabel(handles: FileSystemHandle[]): string {
  const cue = handles.find((handle) => handle.kind === 'file' && handle.name.toLowerCase().endsWith('.cue'));
  if (cue) return cue.name;
  if (handles.length === 1) return handles[0].name;
  return `${handles.length} disc files`;
}

function openDb(): Promise<IDBDatabase | null> {
  try {
    if (!globalThis.indexedDB) return Promise.resolve(null);
  } catch {
    return Promise.resolve(null);
  }
  return new Promise((resolve) => {
    let request: IDBOpenDBRequest;
    try { request = indexedDB.open(DB_NAME, 1); } catch { resolve(null); return; }
    request.onupgradeneeded = () => {
      if (!request.result.objectStoreNames.contains(STORE)) request.result.createObjectStore(STORE);
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => resolve(null);
  });
}

function withStore(mode: IDBTransactionMode, run: (store: IDBObjectStore) => IDBRequest): Promise<unknown> {
  return openDb().then((db) => {
    if (!db) return null;
    return new Promise((resolve) => {
      const finish = (value: unknown) => { db.close(); resolve(value); };
      try {
        const tx = db.transaction(STORE, mode);
        const request = run(tx.objectStore(STORE));
        request.onsuccess = () => finish(request.result);
        request.onerror = () => finish(null);
        tx.onabort = () => finish(null);
      } catch {
        finish(null);
      }
    });
  }).catch(() => null);
}

export async function rememberedDisc(): Promise<FileSystemHandle[] | null> {
  const stored = await withStore('readonly', (store) => store.get(KEY));
  return Array.isArray(stored) && stored.length ? stored as FileSystemHandle[] : null;
}

export function rememberDisc(handles: FileSystemHandle[]): Promise<unknown> {
  return withStore('readwrite', (store) => store.put(handles, KEY));
}

export function forgetDisc(): Promise<unknown> {
  return withStore('readwrite', (store) => store.delete(KEY));
}

export async function discPermission(handles: FileSystemHandle[]): Promise<PermissionState> {
  const states = await Promise.all(handles.map((handle) => handle.queryPermission(READ)));
  if (states.every((state) => state === 'granted')) return 'granted';
  if (states.some((state) => state === 'denied')) return 'denied';
  return 'prompt';
}

/** Starts every request in this turn, so one click still covers all of them. */
export async function requestDiscPermission(handles: FileSystemHandle[]): Promise<PermissionState> {
  const states = await Promise.all(handles.map((handle) => handle.requestPermission(READ)));
  if (states.every((state) => state === 'granted')) return 'granted';
  if (states.some((state) => state === 'denied')) return 'denied';
  return 'prompt';
}

async function filesIn(handle: FileSystemHandle): Promise<File[]> {
  if (handle.kind === 'file') return [await (handle as FileSystemFileHandle).getFile()];
  const files: File[] = [];
  for await (const child of (handle as FileSystemDirectoryHandle).values()) files.push(...await filesIn(child));
  return files;
}

/** Reads the handles. A missing file rejects with DiscGone. */
export async function filesFromHandles(handles: FileSystemHandle[]): Promise<File[]> {
  try {
    const files: File[] = [];
    for (const handle of handles) files.push(...await filesIn(handle));
    return files;
  } catch (error) {
    if (error instanceof DOMException && error.name === 'NotFoundError') throw new DiscGone();
    throw error;
  }
}

export async function pickDiscFiles(): Promise<FileSystemFileHandle[] | null> {
  const open = window.showOpenFilePicker;
  if (!open) return null;
  try {
    return await open({ multiple: true, id: 'rage-racer-disc', types: PICKER_TYPES });
  } catch (error) {
    if (error instanceof DOMException && error.name === 'AbortError') return null;
    throw error;
  }
}

export async function pickDiscFolder(): Promise<FileSystemDirectoryHandle | null> {
  const open = window.showDirectoryPicker;
  if (!open) return null;
  try {
    return await open({ id: 'rage-racer-disc-folder', mode: 'read' });
  } catch (error) {
    if (error instanceof DOMException && error.name === 'AbortError') return null;
    throw error;
  }
}

/**
 * Files from a drop. Chromium also returns handles worth remembering; any
 * other browser, or a drop that does not yield handles, keeps the classic
 * file list and remembers nothing.
 */
export async function droppedDisc(transfer: DataTransfer): Promise<{ files: File[]; handles: FileSystemHandle[] | null }> {
  const items = Array.from(transfer.items).filter((item) => item.kind === 'file');
  if (items.length && items.every((item) => typeof item.getAsFileSystemHandle === 'function')) {
    try {
      const handles: FileSystemHandle[] = [];
      for (const item of items) {
        const handle = await item.getAsFileSystemHandle!();
        if (!handle) throw new Error('no handle');
        handles.push(handle);
      }
      return { files: await filesFromHandles(handles), handles };
    } catch (error) {
      if (error instanceof DiscGone) throw error;
      const files = await droppedFiles(transfer);
      if (files.length) return { files, handles: null };
      throw error;
    }
  }
  return { files: await droppedFiles(transfer), handles: null };
}
