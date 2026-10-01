// The page: accounts, the disc, the lobby and offline practice. The online
// side is in online.ts, the race loop in race.ts.
import './style.css';
import { app, ragePromise, session, setHint, show } from './app';
import { RaceAudio } from './audio';
import {
  canPickDirectory, canPickFiles, DiscGone, discLabel, discPermission, droppedDisc, filesFromHandles,
  forgetDisc, pickDiscFiles, pickDiscFolder, rememberedDisc, rememberDisc, requestDiscPermission,
} from './disc-access';
import { chooseDataTrack } from './disc';
import { enterGarage, loadGarage, ownLook } from './garage';
import { disconnect, enterLobby, send } from './online';
import { beginRace, practiceRace, raceSettings, runRaceLoop } from './race';
import { $, editSettings, fillPractice, readPractice } from './views';

const discStatus = $('disc-status');
const picker = $<HTMLInputElement>('disc-input');
const setup = $<HTMLFormElement>('setup');

// ---- accounts ---------------------------------------------------------------

$<HTMLFormElement>('auth').addEventListener('submit', async (event) => {
  event.preventDefault();
  const form = event.target as HTMLFormElement;
  const action = (event.submitter as HTMLButtonElement | null)?.value;
  const register = action === 'register';
  const value = (name: string) => (form.elements.namedItem(name) as HTMLInputElement).value;
  $('auth-status').textContent = action === 'guest' ? 'Joining as a guest…' : register ? 'Creating your account…' : 'Logging in…';
  try {
    if (action === 'guest') await session.guest();
    else await session.login(value('name').trim(), value('password'), register);
    $('auth-status').textContent = '';
    afterLogin();
  } catch (error) {
    $('auth-status').textContent = (error as Error).message;
  }
});

function afterLogin() {
  $('lobby').querySelector('.who')!.textContent = `${session.user!.name}${session.user!.admin ? ' (admin)' : ''}`;
  void loadGarage();
  if (app.discLoaded) enterLobby();
  else {
    show('disc');
    void offerRememberedDisc();
  }
}

// ---- disc -------------------------------------------------------------------

const disc = $('disc');
const again = $<HTMLButtonElement>('disc-again');
const browse = $<HTMLButtonElement>('disc-browse');
const folder = $<HTMLButtonElement>('disc-folder');
/** Loads run one at a time. A newer choice supersedes one already queued. */
let discTail = Promise.resolve();
let discTicket = 0;

function setDiscBusy(busy: boolean) {
  again.disabled = busy;
  browse.disabled = busy;
  folder.disabled = busy;
}

function showRemembered(label: string) {
  again.hidden = false;
  again.textContent = `Use “${label}”`;
  browse.classList.add('secondary');
}

function hideRemembered() {
  again.hidden = true;
  browse.classList.remove('secondary');
}

function useFiles(files: File[], handles: FileSystemHandle[] | null = null): Promise<void> {
  const ticket = ++discTicket;
  const job = discTail.then(() => loadDiscFiles(files, handles, ticket));
  discTail = job.then(() => undefined, () => undefined);
  return job;
}

async function loadDiscFiles(files: File[], handles: FileSystemHandle[] | null, ticket: number) {
  try {
    if (ticket !== discTicket) return;
    const choice = await chooseDataTrack(files);
    if (ticket !== discTicket) return;
    if ('error' in choice) {
      discStatus.textContent = choice.error;
      disc.dataset.state = 'error';
      setDiscBusy(false);
      return;
    }
    setDiscBusy(true);
    disc.dataset.state = 'busy';
    discStatus.textContent = `Reading ${choice.file.name}…`;
    const rage = await ragePromise;
    const ok = await rage.loadDisc(choice.file, (fraction) => {
      if (ticket !== discTicket) return;
      discStatus.textContent = `Reading ${choice.file.name}… ${Math.round(fraction * 100)}%`;
    });
    if (ticket !== discTicket) return;
    if (!ok) {
      disc.dataset.state = 'error';
      discStatus.textContent = `${choice.file.name} is not a Rage Racer disc image this build can read.`;
      setDiscBusy(false);
      return;
    }
    if (handles?.length) await rememberDisc(handles);
    if (ticket !== discTicket) return;
    if (session.discId && rage.discId() !== session.discId) {
      disc.dataset.state = 'error';
      discStatus.textContent = `This disc (${rage.discId()}) differs from the server's (${session.discId}). ` +
        'Online races need the same release; practice still works offline.';
    } else {
      disc.dataset.state = '';
    }
    app.discLoaded = true;
    void app.audio?.useDisc(files);
    app.automaticCars = rage.carAutomatic();
    fillPractice(rage, setup, app.automaticCars);
    setDiscBusy(false);
    if (disc.dataset.state === 'error') {
      setTimeout(() => show('setup'), 2500);
      return;
    }
    enterLobby();
  } catch {
    if (ticket !== discTicket) return;
    disc.dataset.state = 'error';
    discStatus.textContent = 'The disc could not be read.';
    setDiscBusy(false);
  }
}

async function openRemembered(handles: FileSystemHandle[], ask: boolean) {
  setDiscBusy(true);
  try {
    if (ask && await requestDiscPermission(handles) !== 'granted') {
      disc.dataset.state = 'error';
      discStatus.textContent = 'The browser did not allow reading the saved disc. Drop it again, or browse for it.';
      setDiscBusy(false);
      return;
    }
    await useFiles(await filesFromHandles(handles), handles);
  } catch (error) {
    if (error instanceof DiscGone) {
      await forgetDisc();
      hideRemembered();
    }
    disc.dataset.state = 'error';
    discStatus.textContent = error instanceof DiscGone
      ? error.message
      : 'The saved disc could not be read. Drop it again, or browse for it.';
    setDiscBusy(false);
  }
}

async function offerRememberedDisc() {
  const handles = await rememberedDisc();
  if (!handles?.length || app.discLoaded) return;
  const label = discLabel(handles);
  showRemembered(label);
  again.onclick = () => { void openRemembered(handles, true); };
  let granted = false;
  try { granted = await discPermission(handles) === 'granted'; } catch { granted = false; }
  if (app.discLoaded) return;
  if (granted) {
    await openRemembered(handles, false);
    return;
  }
  disc.dataset.state = '';
  discStatus.textContent = `“${label}” is remembered on this computer.`;
}

async function usePicked(handles: FileSystemHandle[] | null) {
  if (!handles?.length) return;
  try {
    await useFiles(await filesFromHandles(handles), handles);
  } catch (error) {
    disc.dataset.state = 'error';
    discStatus.textContent = error instanceof DiscGone
      ? error.message
      : 'Those files could not be read.';
    setDiscBusy(false);
  }
}

if (canPickDirectory()) folder.hidden = false;
browse.addEventListener('click', () => {
  if (!canPickFiles()) { picker.click(); return; }
  void pickDiscFiles().then(usePicked).catch((error: unknown) => {
    if (error instanceof DOMException && error.name === 'AbortError') return;
    picker.click();
  });
});
folder.addEventListener('click', () => {
  void pickDiscFolder().then((handle) => usePicked(handle ? [handle] : null)).catch((error: unknown) => {
    if (error instanceof DOMException && error.name === 'AbortError') return;
    disc.dataset.state = 'error';
    discStatus.textContent = 'The disc folder could not be opened. Drop it on this page instead.';
  });
});
picker.addEventListener('change', () => { if (picker.files?.length) void useFiles(Array.from(picker.files)); });
addEventListener('dragover', (event) => { event.preventDefault(); $('disc').dataset.drag = 'on'; });
addEventListener('dragleave', () => { $('disc').dataset.drag = ''; });
addEventListener('drop', (event) => {
  event.preventDefault();
  $('disc').dataset.drag = '';
  if (app.screen !== 'disc' || !event.dataTransfer) return;
  void droppedDisc(event.dataTransfer).then(
    (dropped) => useFiles(dropped.files, dropped.handles),
    (error: unknown) => {
      disc.dataset.state = 'error';
      discStatus.textContent = error instanceof DiscGone ? error.message : 'Those files could not be read.';
    },
  );
});

// ---- lobby ------------------------------------------------------------------

$('lobby').addEventListener('click', async (event) => {
  const action = (event.target as HTMLElement).closest<HTMLElement>('[data-action]')?.dataset.action;
  if (action === 'logout') {
    disconnect();
    await session.logout();
    show('auth');
  } else if (action === 'practice') {
    show('setup');
  } else if (action === 'garage' && app.rage) {
    enterGarage();
  } else if (action === 'create-duel') {
    send({ t: 'createDuel' });
  } else if (action === 'create-room' && app.rage) {
    const settings = await editSettings(app.rage, null, 1);
    if (settings) send({ t: 'createRoom', settings });
  }
});

// ---- offline practice ---------------------------------------------------------

setup.querySelector('[data-action=back-to-lobby]')!.addEventListener('click', () => {
  if (session.user) enterLobby();
  else show('auth');
});

setup.addEventListener('submit', async (event) => {
  event.preventDefault();
  const rage = await ragePromise;
  const options = readPractice(setup);
  $('setup-status').textContent = 'Preparing the course and cars…';
  await new Promise(requestAnimationFrame);
  rage.setLook(0, ownLook(rage, options.car));
  if (!rage.startRace(options)) {
    $('setup-status').textContent = 'This course could not be prepared from the disc.';
    return;
  }
  $('setup-status').textContent = '';
  setHint('');
  show('race');
  raceSettings.shadows = options.shadows;
  beginRace(rage, practiceRace());
});

// ---- start ------------------------------------------------------------------

void ragePromise.then(async (rage) => {
  app.rage = rage;
  // The automated browser checks (scripts/e2e.mjs) inspect the race.
  if (location.hash === '#e2e') Object.assign(window, { __race: { rage } });
  app.audio = new RaceAudio(rage);
  if (await session.resume()) afterLogin();
  else show('auth');
});
show('auth');
$('auth').hidden = true; // until the stored session has been checked
runRaceLoop();
