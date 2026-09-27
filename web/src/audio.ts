// Race sound in the page. The WebAssembly module mixes the retail race audio
// (web/wasm/web_audio.c) at 44.1 kHz on every simulation tick; this hands
// that PCM to an AudioWorklet, together with the race tune streamed from the
// disc's CD audio track (./cdda.ts), as the desktop mixes CD audio into the
// SPU output. Browsers only start audio after a user gesture, so the output
// opens on the first click or key press. M mutes.
import { BGM_TRACK_COUNT, bgmCdTrack, discAudioTracks, readTrackPcm, type AudioTrack, type DiscFile } from './cdda';
import type { Hud, Rage } from './rage';
import { PHASE_FINISHED, PHASE_RACING } from './rage';

const RATE = 44100;
/* One second of CD audio: 75 sectors. */
const MUSIC_CHUNK = 75 * 2352;
/* cd_volume.c: g_CdMixPresets 0x7F of 0x80 at the default volume setting. */
const MUSIC_GAIN = 127 / 128;
/* race/lap_and_finish.c StartCdVolumeFade(8) at the finish: eight frames. */
const FINISH_FADE_SECONDS = 8 / 25;

/* Two queues of interleaved 16-bit stereo at 44.1 kHz, resampled to the
 * context rate. Effects keep a small latency window; music is requested
 * ahead whenever less than two seconds are queued. */
const WORKLET = `
class RageMixer extends AudioWorkletProcessor {
  constructor() {
    super();
    this.sfx = []; this.sfxFrames = 0; this.sfxPos = 0;
    this.music = []; this.musicFrames = 0; this.musicPos = 0;
    this.musicGain = 0; this.musicTarget = 0; this.musicStep = 1;
    this.step = ${RATE} / sampleRate; this.phase = 0; this.asked = false;
    this.port.onmessage = ({ data }) => {
      if (data.type === 'sfx') {
        this.sfx.push(data.pcm); this.sfxFrames += data.pcm.length / 2;
        const limit = 0.25 * ${RATE}, keep = 0.08 * ${RATE};
        while (this.sfxFrames - this.sfxPos > limit && this.sfx.length > 1) this.drop(this.sfx, 'sfx');
        if (this.sfxFrames - this.sfxPos > limit) this.sfxPos = this.sfxFrames - keep;
      } else if (data.type === 'music') {
        this.music.push(data.pcm); this.musicFrames += data.pcm.length / 2; this.asked = false;
      } else if (data.type === 'musicStop') {
        this.music = []; this.musicFrames = 0; this.musicPos = 0; this.asked = false;
        this.musicGain = this.musicTarget = 0;
      } else if (data.type === 'musicGain') {
        this.musicTarget = data.gain;
        this.musicStep = data.seconds > 0 ? 1 / (data.seconds * sampleRate) : 1;
        if (data.seconds <= 0) this.musicGain = data.gain;
      } else if (data.type === 'sfxStop') {
        this.sfx = []; this.sfxFrames = 0; this.sfxPos = 0;
      }
    };
  }
  drop(queue, kind) {
    const frames = queue.shift().length / 2;
    this[kind + 'Frames'] -= frames; this[kind + 'Pos'] = Math.max(0, this[kind + 'Pos'] - frames);
  }
  read(queue, kind, out) {
    let pos = this[kind + 'Pos'];
    while (queue.length && pos >= queue[0].length / 2) { pos -= queue[0].length / 2; this.drop(queue, kind); pos = this[kind + 'Pos']; }
    if (!queue.length) { out[0] = out[1] = 0; return false; }
    const chunk = queue[0], i = Math.floor(pos) * 2;
    out[0] = chunk[i] / 32768; out[1] = chunk[i + 1] / 32768;
    return true;
  }
  process(_, outputs) {
    const left = outputs[0][0], right = outputs[0][1] ?? outputs[0][0];
    const s = [0, 0], m = [0, 0];
    for (let i = 0; i < left.length; i++) {
      this.read(this.sfx, 'sfx', s);
      const musicOn = this.read(this.music, 'music', m);
      if (this.musicGain < this.musicTarget) this.musicGain = Math.min(this.musicTarget, this.musicGain + this.musicStep);
      else if (this.musicGain > this.musicTarget) this.musicGain = Math.max(this.musicTarget, this.musicGain - this.musicStep);
      left[i] = s[0] + m[0] * this.musicGain;
      right[i] = s[1] + m[1] * this.musicGain;
      if (this.sfx.length) this.sfxPos += this.step;
      if (musicOn) this.musicPos += this.step;
    }
    if (!this.asked && this.musicFrames - this.musicPos < 2 * ${RATE}) { this.asked = true; this.port.postMessage('more'); }
    return true;
  }
}
registerProcessor('rage-mixer', RageMixer);
`;

type MusicState = 'none' | 'waiting' | 'playing' | 'fading';

export class RaceAudio {
  private context: AudioContext | null = null;
  private node: AudioWorkletNode | null = null;
  private master: GainNode | null = null;
  private opening: Promise<void> | null = null;
  private tracks = new Map<number, AudioTrack>();
  private muted = false;
  /* The retail shuffle bag (race/bgm_select_control.c): every tune once
   * before any repeats. */
  private bag: number[] = [];
  private tune: AudioTrack | null = null;
  private tuneOffset = 0;
  private reading = false;
  private music: MusicState = 'none';
  private generation = 0;

  constructor(private readonly rage: Rage) {
    const unlock = () => { void this.open(); };
    addEventListener('pointerdown', unlock, { capture: true });
    addEventListener('keydown', unlock, { capture: true });
  }

  /** The files the player gave: their CD audio tracks become the music. */
  async useDisc(files: DiscFile[]): Promise<void> {
    this.tracks = await discAudioTracks(files);
    this.bag = [];
  }

  /** Whether the disc's race tunes were found. */
  get hasMusic(): boolean { return this.tunes().length > 0; }

  private tunes(): AudioTrack[] {
    return Array.from({ length: BGM_TRACK_COUNT }, (_, i) => this.tracks.get(bgmCdTrack(i)))
      .filter((track): track is AudioTrack => track !== undefined);
  }

  private open(): Promise<void> {
    if (this.context) {
      if (this.context.state === 'suspended') void this.context.resume();
      return this.opening ?? Promise.resolve();
    }
    let context: AudioContext;
    try {
      context = new AudioContext({ sampleRate: RATE, latencyHint: 'interactive' });
    } catch {
      try { context = new AudioContext(); } catch { return Promise.resolve(); }
    }
    this.context = context;
    const url = URL.createObjectURL(new Blob([WORKLET], { type: 'text/javascript' }));
    this.opening = context.audioWorklet.addModule(url).then(() => {
      URL.revokeObjectURL(url);
      this.node = new AudioWorkletNode(context, 'rage-mixer', { numberOfInputs: 0, outputChannelCount: [2] });
      this.master = context.createGain();
      this.master.gain.value = this.muted ? 0 : 1;
      this.node.connect(this.master).connect(context.destination);
      this.node.port.onmessage = () => { void this.feedMusic(); };
      this.rage.audioEnable(true);
      if (this.music === 'playing') this.post({ type: 'musicGain', gain: MUSIC_GAIN, seconds: 0 });
      void context.resume();
    }).catch(() => { this.node = null; });
    return this.opening;
  }

  private post(message: { type: string; [key: string]: unknown }, transfer: Transferable[] = []): void {
    this.node?.port.postMessage(message, transfer);
  }

  toggleMute(): boolean {
    this.muted = !this.muted;
    if (this.master && this.context) this.master.gain.setTargetAtTime(this.muted ? 0 : 1, this.context.currentTime, 0.02);
    return this.muted;
  }

  /** A new race: its tune is chosen now and starts at the green light. */
  startRace(): void {
    this.generation++;
    this.post({ type: 'musicStop' });
    this.post({ type: 'sfxStop' });
    const tunes = this.tunes();
    if (!tunes.length) {
      this.tune = null;
      this.music = 'none';
      return;
    }
    if (!this.bag.length) {
      this.bag = tunes.map((_, i) => i);
      for (let i = this.bag.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [this.bag[i], this.bag[j]] = [this.bag[j], this.bag[i]];
      }
    }
    this.tune = tunes[this.bag.pop()!];
    this.tuneOffset = 0;
    this.music = 'waiting';
  }

  stopRace(): void {
    this.generation++;
    this.music = 'none';
    this.tune = null;
    this.post({ type: 'musicStop' });
    this.post({ type: 'sfxStop' });
    this.rage.audioTake();
  }

  /** After the frame's ticks: hands over their PCM and follows the race. */
  update(hud: Hud, paused: boolean): void {
    const pcm = this.rage.audioTake();
    if (this.context && this.context.state === 'running' && paused) void this.context.suspend();
    else if (this.context && this.context.state === 'suspended' && !paused && navigator.userActivation?.hasBeenActive !== false) void this.context.resume();
    if (pcm.length && this.node) this.post({ type: 'sfx', pcm }, [pcm.buffer]);
    const finished = hud.phase === PHASE_FINISHED || hud.status === 2 || hud.status === 3;
    if (this.music === 'waiting' && hud.phase === PHASE_RACING && !finished) {
      /* race_scene.c StartCdAudio at the start of the race. */
      this.music = 'playing';
      this.post({ type: 'musicGain', gain: MUSIC_GAIN, seconds: 0 });
      void this.feedMusic();
    } else if (this.music === 'playing' && finished) {
      this.music = 'fading';
      this.post({ type: 'musicGain', gain: 0, seconds: FINISH_FADE_SECONDS });
    }
  }

  /* Streams the tune a second at a time; race tunes loop from the start
   * (cd_audio.c restarts a track with a loop point when it ends). */
  private async feedMusic(): Promise<void> {
    const tune = this.tune;
    if (!tune || this.reading || this.music === 'none' || this.music === 'waiting' || !this.node) return;
    this.reading = true;
    const generation = this.generation;
    try {
      let queued = 0;
      while (queued < 3 * RATE * 4 && generation === this.generation) {
        if (this.tuneOffset >= tune.length) this.tuneOffset = 0;
        const pcm = await readTrackPcm(tune, this.tuneOffset, MUSIC_CHUNK);
        if (generation !== this.generation || !pcm.length) break;
        this.tuneOffset += pcm.length * 2;
        queued += pcm.length * 2;
        this.post({ type: 'music', pcm }, [pcm.buffer]);
      }
    } catch {
      /* An unreadable file only silences the music. */
    } finally {
      this.reading = false;
    }
  }
}
