// The page: accounts, the disc, the lobby and offline practice. The online
// side is in online.ts, the race loop in race.ts.
import './style.css';
import { app, ragePromise, session, setHint, show } from './app';
import { RaceAudio } from './audio';
import { chooseDataTrack, droppedFiles } from './disc';
import { enterGarage, loadGarage, paintOf } from './garage';
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
  else show('disc');
}

// ---- disc -------------------------------------------------------------------

async function useFiles(files: File[]) {
  const disc = $('disc');
  const choice = await chooseDataTrack(files);
  if ('error' in choice) {
    discStatus.textContent = choice.error;
    disc.dataset.state = 'error';
    return;
  }
  disc.dataset.state = 'busy';
  discStatus.textContent = `Reading ${choice.file.name}…`;
  const rage = await ragePromise;
  const ok = await rage.loadDisc(choice.file, (fraction) => {
    discStatus.textContent = `Reading ${choice.file.name}… ${Math.round(fraction * 100)}%`;
  });
  if (!ok) {
    disc.dataset.state = 'error';
    discStatus.textContent = `${choice.file.name} is not a Rage Racer disc image this build can read.`;
    return;
  }
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
  if (disc.dataset.state === 'error') {
    setTimeout(() => show('setup'), 2500);
    return;
  }
  enterLobby();
}

picker.addEventListener('change', () => { if (picker.files?.length) void useFiles(Array.from(picker.files)); });
addEventListener('dragover', (event) => { event.preventDefault(); $('disc').dataset.drag = 'on'; });
addEventListener('dragleave', () => { $('disc').dataset.drag = ''; });
addEventListener('drop', (event) => {
  event.preventDefault();
  $('disc').dataset.drag = '';
  if (app.screen === 'disc' && event.dataTransfer) void droppedFiles(event.dataTransfer).then(useFiles);
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
  rage.setPaint(0, paintOf(rage, options.car));
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
