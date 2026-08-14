"""PSY-Q VAB / VAG audio.

The game hands these blocks straight to the SDK, so the format is Sony's, not
Namco's: src/main/PAL/main/audio/audio_slot_loading.c StartAudioSlotLoad(slot, header, body,
table) calls SsVabOpenHeadSticky(header) + SsVabTransBody(body), i.e. argument 2
is a VAB header (VH) and argument 3 its body (VB). Which sub-block is which is
therefore fixed by the call, not guessed:

    CAR_xx.2ND   StartAudioSlotLoad(3, sub[1], sub[3], sub[2])
    SELBGM.BIN   StartAudioSlotLoad(1, sub[0], sub[2], sub[1])

VAB header, checked against RG3.VH: 0x20 fixed bytes, 128 * 16 program
attributes, programCount * 16 * 32 tone attributes, then 256 u16 VAG sizes in
units of 8 bytes. For RG3.VH that is 0x20 + 0x800 + 27*512 + 0x200 = 16416
bytes, exactly the asset's length on disc.

Sample rate is NOT stored with a VAG. The SPU resamples: pitch 0x1000 plays a
VAG at 44100 Hz, and libsnd derives the pitch from how far the note being keyed
sits from the tone's `center`/`shift`. A VAG therefore has no rate of its own,
only a rate per note, and which note that is depends on how the bank is driven:

  key-on banks (RG3.VH, VOICE.BIN, the CAR_xx.2ND engine banks) are played by
      audio/audio_slot_loading.c, and every key-on there passes note 0x3C - SsUtKeyOnV at
      lines 561, 961, 1233 and 1395, SsUtKeyOn at 1343, and SsUtChangePitch at
      666 and 1204, which bends away from 0x3C as the reference. So the note is
      known, and the rates land on the familiar ladder: 26 of VOICE.BIN's 27
      tones come out at exactly 11025, others at exactly 22050.

  sequenced banks (SELBGM.BIN) are played by a SEQ, whose note-ons span 29..77
      for this one - a melody, not one note. There is no single correct rate, so
      the preview uses each tone's own centre note, i.e. the pitch the sample
      was recorded at, which is the SPU's unshifted 44100.

Assuming 0x3C everywhere would put SELBGM.BIN samples at up to 74 kHz; assuming
44100 everywhere - which is what this file used to do - played VOICE.BIN's
speech four times too fast.
"""

from __future__ import annotations

import struct

VAB_MAGIC = b"pBAV"      # "VABp" little-endian
SEQ_MAGIC = b"pQES"      # "SEQp"

# Sony SPU ADPCM predictor coefficients (x64).
_F0 = (0.0, 60.0, 115.0, 98.0, 122.0)
_F1 = (0.0, 0.0, -52.0, -55.0, -60.0)


def looks_like_vab(buf: bytes, off: int) -> bool:
    return 0 <= off <= len(buf) - 4 and buf[off : off + 4] == VAB_MAGIC


def looks_like_seq(buf: bytes, off: int) -> bool:
    return 0 <= off <= len(buf) - 4 and buf[off : off + 4] == SEQ_MAGIC


def parse_vab_header(buf: bytes, off: int) -> dict | None:
    if not looks_like_vab(buf, off):
        return None
    ver, vab_id, fsize = struct.unpack_from("<III", buf, off + 4)
    _res, programs, tones, vags = struct.unpack_from("<4H", buf, off + 0x10)
    mvol, mpan = buf[off + 0x18], buf[off + 0x19]
    tone_base = off + 0x20 + 128 * 16
    size_base = tone_base + programs * 16 * 32
    if size_base + 512 > len(buf):
        return None
    raw = struct.unpack_from("<256H", buf, size_base)
    sizes = [w * 8 for w in raw[:vags + 1]]
    # The table is 1-based: entry 0 is unused in most VABs.
    sizes = [s for s in sizes if s]
    offsets, acc = [], 0
    for s in sizes:
        offsets.append(acc)
        acc += s
    return {
        "version": ver,
        "vabId": vab_id,
        "declaredSize": fsize,
        "programs": programs,
        "tones": tones,
        "vags": vags,
        "masterVolume": mvol,
        "masterPan": mpan,
        "headerSize": size_base + 512 - off,
        "vagSizes": sizes,
        "vagOffsets": offsets,
        "bodySize": acc,
    }


SPU_BASE_RATE = 44100      # what SPU pitch 0x1000 plays a VAG at
NOTE_KEY_ON = 0x3C         # the note every key-on in audio_slot_loading.c passes
TONE_SIZE = 32
PROGRAM_TONES = 16


def parse_tones(buf: bytes, off: int, header: dict) -> list:
    """The programCount * 16 tone attributes, keeping only the ones with a VAG.

    VagAtr is 32 bytes; the fields this needs are u8 center at +4, u8 shift at
    +5 (pitch correction in 1/128 of a semitone), the u8 key range min/max at
    +6/+7 and the 1-based s16 VAG index at +22.
    """
    base = off + 0x20 + 128 * 16
    out = []
    for prog in range(header["programs"]):
        for slot in range(PROGRAM_TONES):
            a = base + (prog * PROGRAM_TONES + slot) * TONE_SIZE
            if a + TONE_SIZE > len(buf):
                return out
            center, shift, kmin, kmax = struct.unpack_from("<4B", buf, a + 4)
            vag = struct.unpack_from("<h", buf, a + 22)[0]
            if vag <= 0:
                continue
            out.append({"program": prog, "slot": slot, "vag": vag,
                        "center": center, "shift": shift,
                        "keyMin": kmin, "keyMax": kmax})
    return out


def rate_for(center: int, shift: int, note: int = NOTE_KEY_ON, fine: int = 0) -> int:
    """Playback rate in Hz of a tone keyed at `note`.

    SsPitchFromNote works in 1/128 semitones: the sample plays at the base rate
    when the note reaches the tone's centre, and every 12 * 128 units away
    doubles or halves it.
    """
    cents = (note - center) * 128 + (fine - shift)
    return int(round(SPU_BASE_RATE * 2.0 ** (cents / (12.0 * 128.0))))


def vag_rates(tones: list, vag_count: int, note: int | None = NOTE_KEY_ON) -> list:
    """Per-VAG (rate, alternates) for the `vag_count` VAGs, index 0 = VAG 1.

    `note` is the note the bank is played at, or None for a sequenced bank,
    where each tone falls back to its own centre.

    A VAG can be reached by several tones. Most such pairs differ only in
    `shift` by a fraction of a semitone - deliberate detuning for layering, not
    a different sample rate - so rates within a semitone of the winner are
    folded into it. The winner is the rate the most tones agree on, ties going
    to the least-detuned tone. What is left over is genuinely the same sample
    used at two pitches (RG3.VH's VAG 6 is played at both 22050 and 6003) and is
    reported alongside rather than silently dropped.
    """
    by_vag = {}
    for t in tones:
        by_vag.setdefault(t["vag"], []).append(t)
    out = []
    for i in range(vag_count):
        group = by_vag.get(i + 1)
        if not group:
            out.append((None, []))
            continue
        counts = {}
        for t in group:
            n = t["center"] if note is None else note
            counts.setdefault(rate_for(t["center"], t["shift"], n), []).append(t["shift"])
        best = max(counts, key=lambda r: (len(counts[r]), -min(counts[r])))
        alts = sorted(r for r in counts
                      if r != best and abs(r - best) > best * 0.06)
        out.append((best, alts))
    return out


def decode_vag(data: bytes) -> bytes:
    """One SPU-ADPCM stream to signed 16-bit little-endian PCM."""
    out = bytearray()
    s1 = s2 = 0.0
    for b in range(0, len(data) - 15, 16):
        hdr = data[b]
        shift = hdr & 0x0F
        filt = min((hdr >> 4) & 0x0F, 4)
        flags = data[b + 1]
        if flags & 0x07 == 7:   # end marker with no data
            break
        f0, f1 = _F0[filt], _F1[filt]
        for i in range(28):
            byte = data[b + 2 + (i >> 1)]
            nib = (byte >> 4) if (i & 1) else (byte & 0x0F)
            if nib > 7:
                nib -= 16
            samp = float(nib << (12 - shift)) if shift <= 12 else 0.0
            samp += (s1 * f0 + s2 * f1) / 64.0
            s2, s1 = s1, samp
            v = int(round(samp))
            v = -32768 if v < -32768 else (32767 if v > 32767 else v)
            out += struct.pack("<h", v)
        if flags & 0x01:        # loop/end of this sample
            break
    return bytes(out)


def wav(pcm: bytes, rate: int = SPU_BASE_RATE) -> bytes:
    return (
        b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt "
        + struct.pack("<IHHIIHH", 16, 1, 1, rate, rate * 2, 2, 16)
        + b"data" + struct.pack("<I", len(pcm)) + pcm
    )
