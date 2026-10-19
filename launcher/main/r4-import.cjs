const fs = require('node:fs/promises');
const fsSync = require('node:fs');
const path = require('node:path');
const { randomUUID } = require('node:crypto');

const R4_TRACKS = [
  { course: 0, slot: 1, texAsset: 135, trackAsset: 136, name: 'HELTER SKELTER',    lengthText: '4,715m' },
  { course: 1, slot: 2, texAsset: 137, trackAsset: 138, name: 'OUT OF BLUE',       lengthText: '5,155m' },
  { course: 2, slot: 3, texAsset: 139, trackAsset: 140, name: 'WONDERHILL',        lengthText: '5,137m' },
  { course: 3, slot: 4, texAsset: 141, trackAsset: 142, name: 'HEAVEN AND HELL',   lengthText: '5,299m' },
  { course: 4, slot: 5, texAsset: 143, trackAsset: 144, name: 'EDGE OF THE EARTH', lengthText: '6,133m' },
  { course: 5, slot: 6, texAsset: 145, trackAsset: 146, name: 'BRIGHTEST NITE',    lengthText: '5,917m' },
  { course: 6, slot: 7, texAsset: 147, trackAsset: 148, name: 'PHANTOMILE',        lengthText: '2,772m' },
  { course: 7, slot: 8, texAsset: 149, trackAsset: 150, name: 'SHOOTING HOOPS',    lengthText: '3,573m' },
];

// R4 terrain pages across all 8 tracks: 12, 14, 15, 21..29. Skybox uses page 11.
// Pages 8 (512,0), 30 (896,256), and 31 (960,256) are 100% free in Rage Racer races (HUD uses 9..10, player car uses 11, rivals/countdown use 12..14).
const USED_R4_PAGES = new Set([12, 14, 15, 21, 22, 23, 24, 25, 26, 27, 28, 29]);
const SKY_CLUTS = [
  0x783c, 0x783d, 0x783e, 0x783f,
  0x787c, 0x787d, 0x787e, 0x787f,
  0x78bc, 0x78bd, 0x78be, 0x78bf,
];

function remapTpage(tp) {
  const page = tp & 0x1f;
  if (page === 12) return (tp & ~0x1f) | 8;  // 12:0 (768,0) -> 8:0 (512,0)
  if (page === 14) return (tp & ~0x1f) | 30; // 14:0 (896,0) -> 14:1 (896,256)
  if (page === 11) return (tp & ~0x1f) | 31; // 11:0 (704,0) -> 15:1 (960,256)
  return tp;
}

function isR4SectorTable(buf) {
  if (!buf || buf.length < 72) return false;
  const count = buf.readUInt32LE(0);
  if (count < 16 || count > 2048) return false;
  const first = buf.readUInt32LE(4);
  if (first < 1 || first > 16) return false;
  let prev = first;
  for (let i = 1; i <= Math.min(count, 16); i++) {
    const cur = buf.readUInt32LE(4 + i * 4);
    if (cur <= prev || cur - prev > 4096) return false;
    prev = cur;
  }
  return true;
}

async function openR4ArchiveReader(inputPath) {
  let targetPath = inputPath;
  if (path.extname(inputPath).toLowerCase() === '.cue') {
    const cueText = await fs.readFile(inputPath, 'utf8');
    const m = /FILE\s+"([^"]+)"\s+BINARY/i.exec(cueText) || /FILE\s+(\S+)\s+BINARY/i.exec(cueText);
    if (!m) throw Error('Could not find a Track 01 BIN entry in the selected CUE file');
    targetPath = path.resolve(path.dirname(inputPath), m[1]);
  }
  const handle = await fs.open(targetPath, 'r');
  try {
    const stat = await handle.stat();
    if (!stat.isFile() || stat.size < 65536) {
      throw Error('Selected file is too small to be a Ridge Racer Type 4 disc image or R4.BIN archive');
    }
    // 1. Check direct 2048-byte/sector R4.BIN at offset 0
    const probe0 = Buffer.alloc(128);
    await handle.read(probe0, 0, 128, 0);
    if (isR4SectorTable(probe0)) {
      return {
        handle,
        close: () => handle.close(),
        readSectors: async (startSector, numSectors) => {
          const out = Buffer.alloc(numSectors * 2048);
          const { bytesRead } = await handle.read(out, 0, out.length, startSector * 2048);
          if (bytesRead < out.length) throw Error('Unexpected EOF reading R4.BIN');
          return out;
        },
      };
    }
    // 2. Check raw 2352-byte/sector Track 01 BIN (LBA 337 first, then scan LBA 16..1024)
    const candidateLbas = [337];
    for (let s = 16; s <= 1024; s++) if (s !== 337) candidateLbas.push(s);
    for (const lba of candidateLbas) {
      const pos = lba * 2352 + 24;
      if (pos + 128 > stat.size) continue;
      const probe = Buffer.alloc(128);
      await handle.read(probe, 0, 128, pos);
      if (isR4SectorTable(probe)) {
        return {
          handle,
          close: () => handle.close(),
          readSectors: async (startSector, numSectors) => {
            const rawLen = numSectors * 2352;
            const rawBuf = Buffer.alloc(rawLen);
            const { bytesRead } = await handle.read(rawBuf, 0, rawLen, (lba + startSector) * 2352);
            if (bytesRead < rawLen) throw Error('Unexpected EOF reading R4 Track 01 BIN');
            const out = Buffer.alloc(numSectors * 2048);
            for (let s = 0; s < numSectors; s++) {
              rawBuf.copy(out, s * 2048, s * 2352 + 24, s * 2352 + 24 + 2048);
            }
            return out;
          },
        };
      }
    }
    throw Error('Could not locate R4.BIN track archive in the selected image. Choose a Ridge Racer Type 4 Track 01 BIN, CUE, or R4.BIN file.');
  } catch (err) {
    await handle.close().catch(() => {});
    throw err;
  }
}

function buildRelocatedImageChain(streams) {
  const chunks = [];
  const hdr = Buffer.alloc(4);
  hdr.writeInt32LE(256, 0);
  chunks.push(hdr);

  for (const { buf, isSky } of streams) {
    let pos = 4;
    while (pos + 4 <= buf.length) {
      const sz = buf.readInt32LE(pos);
      pos += 4;
      if (sz <= 0 || pos + sz > buf.length) break;
      const entry = Buffer.from(buf.subarray(pos, pos + sz));
      pos += sz;

      const flags = entry.readUInt32LE(4);
      const hasClut = (flags & 8) !== 0;
      let pOff = 8;
      let keepClut = false;
      if (hasClut) {
        const cSize = entry.readUInt32LE(8);
        const cx = entry.readUInt16LE(12);
        const cy = entry.readUInt16LE(14);
        keepClut = isSky ? true : (cx >= 832 && cx < 896 && cy >= 256 && cy < 352);
        pOff += cSize;
      }

      const px = entry.readUInt16LE(pOff + 4);
      const py = entry.readUInt16LE(pOff + 6);
      const pPage = Math.floor(px / 64) + (py >= 256 ? 16 : 0);
      const keepPixels = isSky ? (pPage === 11) : USED_R4_PAGES.has(pPage);

      if (!keepClut && !keepPixels) continue;

      if (keepPixels) {
        if (isSky && pPage === 11) {
          entry.writeUInt16LE(px + 256, pOff + 4); // 704 -> 960 (page 31 = 15:1)
          entry.writeUInt16LE(py + 256, pOff + 6); // 0..223 -> 256..479
        } else if (pPage === 12) {
          entry.writeUInt16LE(px - 256, pOff + 4); // 768 -> 512 (page 8 = 8:0)
        } else if (pPage === 14) {
          entry.writeUInt16LE(py + 256, pOff + 6); // 896,0..255 -> 896,256..511 (page 30 = 14:1)
        }
      } else {
        entry.writeUInt32LE(12, pOff + 0);
        entry.writeUInt16LE(0, pOff + 4);
        entry.writeUInt16LE(0, pOff + 6);
        entry.writeUInt16LE(0, pOff + 8);
        entry.writeUInt16LE(0, pOff + 10);
        const newEntry = entry.subarray(0, pOff + 12);
        const szBuf = Buffer.alloc(4);
        szBuf.writeInt32LE(newEntry.length, 0);
        chunks.push(szBuf, newEntry);
        continue;
      }

      if (hasClut && !keepClut) {
        const newEntry = Buffer.alloc(8 + (entry.length - pOff));
        entry.copy(newEntry, 0, 0, 8);
        newEntry.writeUInt32LE(flags & ~8, 4);
        entry.copy(newEntry, 8, pOff);
        const szBuf = Buffer.alloc(4);
        szBuf.writeInt32LE(newEntry.length, 0);
        chunks.push(szBuf, newEntry);
        continue;
      }

      const szBuf = Buffer.alloc(4);
      szBuf.writeInt32LE(entry.length, 0);
      chunks.push(szBuf, entry);
    }
  }

  const term = Buffer.alloc(4);
  term.writeInt32LE(0, 0);
  chunks.push(term);
  return Buffer.concat(chunks);
}

function filterOrigSecondaryImages(buf) {
  const chunks = [];
  const hdr = Buffer.alloc(4);
  hdr.writeInt32LE(buf.readInt32LE(0), 0);
  chunks.push(hdr);
  let pos = 4;
  while (pos + 4 <= buf.length) {
    const sz = buf.readInt32LE(pos);
    pos += 4;
    if (sz <= 0 || pos + sz > buf.length) break;
    const entry = buf.subarray(pos, pos + sz);
    pos += sz;
    const flags = entry.readUInt32LE(4);
    const hasClut = (flags & 8) !== 0;
    const pOff = hasClut ? 8 + entry.readUInt32LE(8) : 8;
    const px = entry.readUInt16LE(pOff + 4);
    const py = entry.readUInt16LE(pOff + 6);
    if (py < 256 && px >= 832 && px < 896) {
      const szBuf = Buffer.alloc(4);
      szBuf.writeInt32LE(sz, 0);
      chunks.push(szBuf, Buffer.from(entry));
    }
  }
  const term = Buffer.alloc(4);
  term.writeInt32LE(0, 0);
  chunks.push(term);
  return Buffer.concat(chunks);
}

function decodeWindow(word) {
  if ((word >>> 24) !== 0xe2) return [0, 0, 0, 0];
  const value = word & 0xfffff;
  const maskU = value & 0x1f;
  const maskV = (value >>> 5) & 0x1f;
  if (maskU === 0 && maskV === 0) return [0, 0, 0, 0];
  const offsetU = (value >>> 10) & 0x1f;
  const offsetV = (value >>> 15) & 0x1f;
  const wU = 256 - maskU * 8;
  const wV = 256 - maskV * 8;
  const oU = (offsetU & maskU) * 8;
  const oV = (offsetV & maskV) * 8;
  if (!wU || !wV || oU + wU > 256 || oV + wV > 256) return [0, 0, 0, 0];
  return [wU, wV, oU, oV];
}

function scaleVertexColor(rgb) {
  return rgb.map(c => Math.min(255, Math.round(c * 1.06)));
}

function lerp(a, b, t) { return a + (b - a) * t; }
function lerpArr(a, b, t) { return a.map((v, i) => lerp(v, b[i], t)); }
function lerpColor(a, b, t) { return a.map((v, i) => Math.round(lerp(v, b[i], t))); }

function isRoadDecalTri(p0, p1, p2) {
  const ax = p1[0] - p0[0], ay = p1[1] - p0[1], az = p1[2] - p0[2];
  const bx = p2[0] - p0[0], by = p2[1] - p0[1], bz = p2[2] - p0[2];
  const cx = p2[0] - p1[0], cy = p2[1] - p1[1], cz = p2[2] - p1[2];
  const nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
  const len = Math.hypot(nx, ny, nz);
  if (len <= 1e-6) return false;
  const ab = Math.hypot(ax, ay, az), ac = Math.hypot(bx, by, bz), bc = Math.hypot(cx, cy, cz);
  const shortest = Math.min(ab, ac, bc), longest = Math.max(ab, ac, bc);
  return shortest <= 16.5 && longest >= 62.0 && longest >= shortest * 7.5 && Math.abs(ny) >= len * 0.84;
}

function updateQuadMeta(q) {
  const v = q.worldVerts;
  const cx = (v[0][0] + v[1][0] + v[2][0] + v[3][0]) * 0.25;
  const cy = (v[0][1] + v[1][1] + v[2][1] + v[3][1]) * 0.25;
  const cz = (v[0][2] + v[1][2] + v[2][2] + v[3][2]) * 0.25;
  q.cx = cx; q.cy = cy; q.cz = cz;
  if (q.cellIdx === undefined) {
    q.gx = Math.max(0, Math.min(31, Math.floor(cx / 2048)));
    q.gz = Math.max(0, Math.min(31, Math.floor(cz / 2048)));
    q.cellIdx = (31 - q.gz) * 32 + q.gx;
  }

  const rv = v.map(p => [p[0], -p[1], -p[2]]);
  q.rv = rv;
  function triPlane(a, b, c) {
    const ax = b[0] - a[0], ay = b[1] - a[1], az = b[2] - a[2];
    const bx = c[0] - a[0], by = c[1] - a[1], bz = c[2] - a[2];
    let nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
    const len = Math.hypot(nx, ny, nz) || 1;
    return { nx: nx / len, ny: ny / len, nz: nz / len, p: a };
  }
  q.t0 = triPlane(rv[0], rv[2], rv[1]);
  q.t1 = triPlane(rv[1], rv[2], rv[3]);
  let nx = q.t0.nx + q.t1.nx, ny = q.t0.ny + q.t1.ny, nz = q.t0.nz + q.t1.nz;
  const len = Math.hypot(nx, ny, nz) || 1;
  q.nx = nx / len; q.ny = ny / len; q.nz = nz / len;
  q.rcx = (rv[0][0] + rv[1][0] + rv[2][0] + rv[3][0]) * 0.25;
  q.rcy = (rv[0][1] + rv[1][1] + rv[2][1] + rv[3][1]) * 0.25;
  q.rcz = (rv[0][2] + rv[1][2] + rv[2][2] + rv[3][2]) * 0.25;
  q.minX = Math.min(rv[0][0], rv[1][0], rv[2][0], rv[3][0]);
  q.maxX = Math.max(rv[0][0], rv[1][0], rv[2][0], rv[3][0]);
  q.minY = Math.min(rv[0][1], rv[1][1], rv[2][1], rv[3][1]);
  q.maxY = Math.max(rv[0][1], rv[1][1], rv[2][1], rv[3][1]);
  q.minZ = Math.min(rv[0][2], rv[1][2], rv[2][2], rv[3][2]);
  q.maxZ = Math.max(rv[0][2], rv[1][2], rv[2][2], rv[3][2]);
  return q;
}

function subdivideQuadIfDecal(q, depth = 0) {
  updateQuadMeta(q);
  const rv = q.rv;
  const isHorizOverlay = depth === 0 && Math.abs(q.ny) >= 0.85 && q.depthBias <= 5;
  const matchesDecal = isRoadDecalTri(rv[0], rv[2], rv[1]) || isRoadDecalTri(rv[1], rv[2], rv[3]);
  if ((!matchesDecal && !isHorizOverlay) || depth >= 3) return [q];

  const v = q.worldVerts, u = q.uvs, c = q.colors;
  const e01 = Math.hypot(v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2]);
  const e23 = Math.hypot(v[3][0] - v[2][0], v[3][1] - v[2][1], v[3][2] - v[2][2]);
  const e02 = Math.hypot(v[2][0] - v[0][0], v[2][1] - v[0][1], v[2][2] - v[0][2]);
  const e13 = Math.hypot(v[3][0] - v[1][0], v[3][1] - v[1][1], v[3][2] - v[1][2]);
  const max01 = Math.max(e01, e23), max02 = Math.max(e02, e13);
  const targetLen = matchesDecal ? 32.0 : 36.0;

  if (max02 >= max01 && max02 > targetLen) {
    const steps = Math.max(2, Math.ceil(max02 / targetLen));
    const out = [];
    for (let s = 0; s < steps; s++) {
      const t0 = s / steps, t1 = (s + 1) / steps;
      out.push(...subdivideQuadIfDecal({
        ...q,
        worldVerts: [lerpArr(v[0], v[2], t0), lerpArr(v[1], v[3], t0), lerpArr(v[0], v[2], t1), lerpArr(v[1], v[3], t1)],
        uvs: [lerpArr(u[0], u[2], t0), lerpArr(u[1], u[3], t0), lerpArr(u[0], u[2], t1), lerpArr(u[1], u[3], t1)],
        colors: [lerpColor(c[0], c[2], t0), lerpColor(c[1], c[3], t0), lerpColor(c[0], c[2], t1), lerpColor(c[1], c[3], t1)],
      }, depth + 1));
    }
    return out;
  } else if (max01 > targetLen) {
    const steps = Math.max(2, Math.ceil(max01 / targetLen));
    const out = [];
    for (let s = 0; s < steps; s++) {
      const t0 = s / steps, t1 = (s + 1) / steps;
      out.push(...subdivideQuadIfDecal({
        ...q,
        worldVerts: [lerpArr(v[0], v[1], t0), lerpArr(v[0], v[1], t1), lerpArr(v[2], v[3], t0), lerpArr(v[2], v[3], t1)],
        uvs: [lerpArr(u[0], u[1], t0), lerpArr(u[0], u[1], t1), lerpArr(u[2], u[3], t0), lerpArr(u[2], u[3], t1)],
        colors: [lerpColor(c[0], c[1], t0), lerpColor(c[0], c[1], t1), lerpColor(c[2], c[3], t0), lerpColor(c[2], c[3], t1)],
      }, depth + 1));
    }
    return out;
  }
  return [q];
}

function clipPoly2D(subject, clip) {
  let out = subject;
  const ax = clip[1][0] - clip[0][0], ay = clip[1][1] - clip[0][1];
  const bx = clip[2][0] - clip[0][0], by = clip[2][1] - clip[0][1];
  const sign = (ax * by - ay * bx) >= 0 ? 1 : -1;
  for (let i = 0; i < clip.length; i++) {
    const a = clip[i], b = clip[(i + 1) % clip.length];
    const ex = b[0] - a[0], ey = b[1] - a[1];
    const input = out;
    out = [];
    if (input.length === 0) break;
    let s = input[input.length - 1];
    let sIn = sign * (ex * (s[1] - a[1]) - ey * (s[0] - a[0])) >= -1e-5;
    for (const e of input) {
      const eIn = sign * (ex * (e[1] - a[1]) - ey * (e[0] - a[0])) >= -1e-5;
      if (eIn) {
        if (!sIn) {
          const dx = e[0] - s[0], dy = e[1] - s[1];
          const denom = ex * dy - ey * dx;
          const t = Math.abs(denom) > 1e-9 ? (ex * (a[1] - s[1]) - ey * (a[0] - s[0])) / denom : 0;
          out.push([s[0] + t * dx, s[1] + t * dy]);
        }
        out.push(e);
      } else if (sIn) {
        const dx = e[0] - s[0], dy = e[1] - s[1];
        const denom = ex * dy - ey * dx;
        const t = Math.abs(denom) > 1e-9 ? (ex * (a[1] - s[1]) - ey * (a[0] - s[0])) / denom : 0;
        out.push([s[0] + t * dx, s[1] + t * dy]);
      }
      s = e; sIn = eIn;
    }
  }
  return out;
}

function polyArea2D(p) {
  if (p.length < 3) return 0;
  let a = 0;
  for (let i = 0; i < p.length; i++) {
    const j = (i + 1) % p.length;
    a += p[i][0] * p[j][1] - p[j][0] * p[i][1];
  }
  return Math.abs(a) * 0.5;
}

function buildTerrainRmesh(meshQuadsList) {
  const meshCount = meshQuadsList.length;
  let totalQuads = 0;
  for (const m of meshQuadsList) totalQuads += m.quads.length;
  const vertexCount = totalQuads * 4;
  const indexCount = totalQuads * 6;
  const offsetsBytes = (meshCount + 1) * 4;
  const verticesOffset = 24 + offsetsBytes;
  const indicesOffset = verticesOffset + vertexCount * 40;
  const totalSize = indicesOffset + indexCount * 4;
  const buf = Buffer.alloc(totalSize);

  buf.write('RRMESH1\0', 0, 8, 'ascii');
  buf.writeUInt32LE(1, 8);
  buf.writeUInt32LE(meshCount, 12);
  buf.writeUInt32LE(vertexCount, 16);
  buf.writeUInt32LE(indexCount, 20);

  let vCursor = verticesOffset;
  let iCursor = indicesOffset;
  let vIndex = 0;
  let iCount = 0;
  const triOrder = [0, 2, 1, 1, 2, 3];
  const RAGE_RUNTIME_MATERIAL_METADATA = 1 << 29;
  const RAGE_RUNTIME_MATERIAL_FOGGED = 1 << 27;

  buf.writeUInt32LE(0, 24);
  for (let m = 0; m < meshCount; m++) {
    const { gx, gz, quads: mQuads } = meshQuadsList[m];
    const cellWorldX = gx * 2048 + 1024;
    const cellWorldZ = gz * 2048 + 1024;

    for (const q of mQuads) {
      const p0 = [(q.worldVerts[0][0] - cellWorldX) * 4, -q.worldVerts[0][1] * 4, -(q.worldVerts[0][2] - cellWorldZ) * 4];
      const p1 = [(q.worldVerts[1][0] - cellWorldX) * 4, -q.worldVerts[1][1] * 4, -(q.worldVerts[1][2] - cellWorldZ) * 4];
      const p2 = [(q.worldVerts[2][0] - cellWorldX) * 4, -q.worldVerts[2][1] * 4, -(q.worldVerts[2][2] - cellWorldZ) * 4];
      const ax = p2[0] - p0[0], ay = p2[1] - p0[1], az = p2[2] - p0[2];
      const bx = p1[0] - p0[0], by = p1[1] - p0[1], bz = p1[2] - p0[2];
      let nx = ay * bz - az * by;
      let ny = az * bx - ax * bz;
      let nz = ax * by - ay * bx;
      const len = Math.hypot(nx, ny, nz) || 1;
      nx /= len; ny /= len; nz /= len;

      const scaledBias = Math.max(-128, Math.min(127, Math.round((q.depthBias - 15) * 3)));
      const encodedMat = ((q.matIdx & 0xffff) | RAGE_RUNTIME_MATERIAL_METADATA | RAGE_RUNTIME_MATERIAL_FOGGED | ((scaledBias & 0xff) << 16)) >>> 0;

      for (let c = 0; c < 4; c++) {
        const lx = (q.worldVerts[c][0] - cellWorldX) * 4;
        const ly = -q.worldVerts[c][1] * 4;
        const lz = -(q.worldVerts[c][2] - cellWorldZ) * 4;
        buf.writeFloatLE(lx, vCursor + 0);
        buf.writeFloatLE(ly, vCursor + 4);
        buf.writeFloatLE(lz, vCursor + 8);
        buf.writeFloatLE(nx, vCursor + 12);
        buf.writeFloatLE(ny, vCursor + 16);
        buf.writeFloatLE(nz, vCursor + 20);
        buf[vCursor + 24] = q.colors[c][0];
        buf[vCursor + 25] = q.colors[c][1];
        buf[vCursor + 26] = q.colors[c][2];
        buf[vCursor + 27] = 255;
        buf.writeFloatLE((q.uvs[c][0] + 0.5) / 256.0, vCursor + 28);
        buf.writeFloatLE((q.uvs[c][1] + 0.5) / 256.0, vCursor + 32);
        buf.writeUInt32LE(encodedMat, vCursor + 36);
        vCursor += 40;
      }
      for (let k = 0; k < 6; k++) {
        buf.writeUInt32LE(vIndex + triOrder[k], iCursor);
        iCursor += 4;
      }
      vIndex += 4;
      iCount += 6;
    }
    buf.writeUInt32LE(iCount, 24 + (m + 1) * 4);
  }
  return buf;
}

function extractSkyPaletteFromB1(b1Buf) {
  const count = b1Buf.readUInt32LE(0);
  const STRIDES = { 1: 28, 7: 20, 11: 32 };
  const byY = new Map();
  for (let m = 0; m < Math.min(2, count); m++) {
    const vOff = b1Buf.readUInt32LE(4 + m * 12 + 0);
    const vCnt = b1Buf.readUInt32LE(4 + m * 12 + 4);
    const sOff = b1Buf.readUInt32LE(4 + m * 12 + 8);
    const vs = [];
    for (let i = 0; i < vCnt; i++) {
      vs.push(b1Buf.readInt16LE(vOff + i * 8 + 2));
    }
    let pos = sOff;
    while (pos + 4 <= b1Buf.length) {
      const prim = b1Buf.readUInt16LE(pos);
      const fCnt = b1Buf.readUInt16LE(pos + 2);
      pos += 4;
      if (fCnt === 0) break;
      const stride = STRIDES[prim];
      if (!stride) break;
      if (prim === 7) {
        for (let f = 0; f < fCnt; f++) {
          const r = b1Buf.subarray(pos + f * stride, pos + (f + 1) * stride);
          for (let k = 0; k < 3; k++) {
            const vi = r.readUInt16LE(k * 2);
            const vy = vs[vi];
            const co = 8 + k * 4;
            if (vy !== undefined && !byY.has(vy)) {
              byY.set(vy, [r[co], r[co + 1], r[co + 2]]);
            }
          }
        }
      }
      pos += fCnt * stride;
    }
  }
  const sortedY = [...byY.keys()].sort((a, b) => a - b);
  if (sortedY.length < 2) {
    return {
      zenith:  [64, 144, 192],
      mid:     [88, 176, 216],
      horizon: [192, 228, 220],
      bottom:  [48, 48, 48],
    };
  }
  const zenith = byY.get(sortedY[0]);
  const mid = byY.get(sortedY[Math.min(1, sortedY.length - 1)]);
  const horizCandidates = sortedY.filter(y => y <= 10);
  const horizon = byY.get(horizCandidates.length ? horizCandidates[horizCandidates.length - 1] : sortedY[Math.floor(sortedY.length / 2)]);
  const bottom = byY.get(sortedY[sortedY.length - 1]);
  return { zenith, mid, horizon, bottom };
}

function buildSkyCourseRmesh(b1Buf, skyBottomColor) {
  const vOff = b1Buf.readUInt32LE(4 + 2 * 12 + 0);
  const vCnt = b1Buf.readUInt32LE(4 + 2 * 12 + 4);
  const sOff = b1Buf.readUInt32LE(4 + 2 * 12 + 8);
  const skyVerts = [];
  for (let i = 0; i < vCnt; i++) {
    skyVerts.push({
      vx: b1Buf.readInt16LE(vOff + i * 8 + 0),
      vy: b1Buf.readInt16LE(vOff + i * 8 + 2),
      vz: b1Buf.readInt16LE(vOff + i * 8 + 4),
    });
  }
  const triCnt = b1Buf.readUInt16LE(sOff + 2);
  const SKY_XZ_SCALE = 76.0;
  const SKY_Y_SCALE = 76.0;

  const verts = [];
  const indices = [];

  function pushTriDoubleSided(p0, p1, p2, uv0, uv1, uv2, col, matIdx) {
    const base = verts.length;
    const pts = [p0, p1, p2];
    const uvs = [uv0, uv1, uv2];
    for (let k = 0; k < 3; k++) {
      verts.push({
        px: pts[k][0],
        py: pts[k][1],
        pz: pts[k][2],
        nx: 0,
        ny: 1,
        nz: 0,
        r: col[0],
        g: col[1],
        b: col[2],
        a: 255,
        u: (uvs[k][0] + 0.5) / 256.0,
        v: (uvs[k][1] + 0.5) / 256.0,
        mat: matIdx,
      });
    }
    indices.push(base + 0, base + 1, base + 2, base + 0, base + 2, base + 1);
  }

  const skirtCol = skyBottomColor || [48, 48, 48];
  for (let f = 0; f < triCnt; f++) {
    const r = b1Buf.subarray(sOff + 4 + f * 32, sOff + 4 + (f + 1) * 32);
    const i0 = r.readUInt16LE(0);
    const i1 = r.readUInt16LE(2);
    const i2 = r.readUInt16LE(4);
    const clut = r.readUInt16LE(22) & 0x7fff;
    const matIdx = Math.max(0, SKY_CLUTS.indexOf(clut));
    const v0 = skyVerts[i0], v1 = skyVerts[i1], v2 = skyVerts[i2];
    const p0 = [v0.vx * SKY_XZ_SCALE, -v0.vy * SKY_Y_SCALE, -v0.vz * SKY_XZ_SCALE];
    const p1 = [v1.vx * SKY_XZ_SCALE, -v1.vy * SKY_Y_SCALE, -v1.vz * SKY_XZ_SCALE];
    const p2 = [v2.vx * SKY_XZ_SCALE, -v2.vy * SKY_Y_SCALE, -v2.vz * SKY_XZ_SCALE];
    const uv0 = [r[20], r[21]], uv1 = [r[24], r[25]], uv2 = [r[28], r[29]];
    pushTriDoubleSided(p0, p1, p2, uv0, uv1, uv2, [255, 255, 255], matIdx);

    if (v0.vy > 0 && v2.vy > 0) {
      const b0Pt = [v0.vx * SKY_XZ_SCALE, -450 * SKY_Y_SCALE, -v0.vz * SKY_XZ_SCALE];
      const b2Pt = [v2.vx * SKY_XZ_SCALE, -450 * SKY_Y_SCALE, -v2.vz * SKY_XZ_SCALE];
      const darkUv = [0, 31];
      pushTriDoubleSided(p0, p2, b2Pt, darkUv, darkUv, darkUv, skirtCol, 0);
      pushTriDoubleSided(p0, b2Pt, b0Pt, darkUv, darkUv, darkUv, skirtCol, 0);
    }
  }

  const meshCount = 64;
  const headerBytes = 24 + (meshCount + 1) * 4;
  const vertBytes = verts.length * 40;
  const idxBytes = indices.length * 4;
  const buf = Buffer.alloc(headerBytes + vertBytes + idxBytes);
  buf.write('RRMESH1\0', 0, 8, 'ascii');
  buf.writeUInt32LE(1, 8);
  buf.writeUInt32LE(meshCount, 12);
  buf.writeUInt32LE(verts.length, 16);
  buf.writeUInt32LE(indices.length, 20);
  buf.writeUInt32LE(0, 24);
  for (let m = 1; m <= meshCount; m++) {
    buf.writeUInt32LE(indices.length, 24 + m * 4);
  }
  let vCursor = headerBytes;
  for (const v of verts) {
    buf.writeFloatLE(v.px, vCursor + 0);
    buf.writeFloatLE(v.py, vCursor + 4);
    buf.writeFloatLE(v.pz, vCursor + 8);
    buf.writeFloatLE(v.nx, vCursor + 12);
    buf.writeFloatLE(v.ny, vCursor + 16);
    buf.writeFloatLE(v.nz, vCursor + 20);
    buf[vCursor + 24] = v.r;
    buf[vCursor + 25] = v.g;
    buf[vCursor + 26] = v.b;
    buf[vCursor + 27] = v.a;
    buf.writeFloatLE(v.u, vCursor + 28);
    buf.writeFloatLE(v.v, vCursor + 32);
    buf.writeUInt32LE(v.mat, vCursor + 36);
    vCursor += 40;
  }
  let iCursor = headerBytes + vertBytes;
  for (const idx of indices) {
    buf.writeUInt32LE(idx, iCursor);
    iCursor += 4;
  }
  return buf;
}

function buildCleanBlock5() {
  const modelCount = 64;
  const headerBytes = 4 + modelCount * 12;
  const geomOffset = headerBytes;
  const model0Offset = geomOffset + 8;
  const emptyModelOffset = model0Offset + 4 + SKY_CLUTS.length * 28;
  const b5 = Buffer.alloc(emptyModelOffset + 4);
  b5.writeInt32LE(modelCount, 0);
  for (let m = 0; m < modelCount; m++) {
    const eOff = 4 + m * 12;
    b5.writeInt32LE(geomOffset, eOff + 0);
    b5.writeInt32LE(1, eOff + 4);
    b5.writeInt32LE(m === 0 ? model0Offset : emptyModelOffset, eOff + 8);
  }
  b5.writeUInt16LE(1, model0Offset + 0);
  b5.writeUInt16LE(SKY_CLUTS.length, model0Offset + 2);
  for (let i = 0; i < SKY_CLUTS.length; i++) {
    const rOff = model0Offset + 4 + i * 28;
    b5[rOff + 8] = 128; b5[rOff + 9] = 128; b5[rOff + 10] = 128; b5[rOff + 11] = 0x2c;
    b5.writeUInt16LE(SKY_CLUTS[i], rOff + 14);
    b5.writeUInt16LE(0x1f, rOff + 18);
  }
  b5.writeUInt32LE(0, emptyModelOffset);
  return b5;
}

function buildFullBlock7(cells) {
  const numCells = cells.length;
  const vertList = [];
  const vertMap = new Map();

  function getVertIdx(vx, vy, vz) {
    const k = `${vx}:${vy}:${vz}`;
    let idx = vertMap.get(k);
    if (idx === undefined) {
      idx = vertList.length;
      vertMap.set(k, idx);
      vertList.push([vx, vy, vz]);
    }
    return idx;
  }

  const cellBuffers = [];
  for (let m = 0; m < numCells; m++) {
    const cell = cells[m];
    const cellWorldX = cell.gx * 2048 + 1024;
    const cellWorldZ = cell.gz * 2048 + 1024;
    const m2 = cell.mode2;
    const m4 = cell.mode4;
    const byteLen = (m2.length > 0 ? 4 + m2.length * 32 : 0) +
                    (m4.length > 0 ? 4 + m4.length * 36 : 0) + 4;
    const cBuf = Buffer.alloc(byteLen);
    let cur = 0;

    for (const [mode, list, stride] of [[2, m2, 32], [4, m4, 36]]) {
      if (list.length === 0) continue;
      cBuf.writeUInt16LE(mode, cur + 0);
      cBuf.writeUInt16LE(list.length, cur + 2);
      cur += 4;
      for (const q of list) {
        for (let c = 0; c < 4; c++) {
          const vx = Math.max(-32768, Math.min(32767, Math.round((q.origWorldVerts[c][0] - cellWorldX) * 4)));
          const vy = Math.max(-32768, Math.min(32767, Math.round(q.origWorldVerts[c][1] * 4)));
          const vz = Math.max(-32768, Math.min(32767, Math.round((q.origWorldVerts[c][2] - cellWorldZ) * 4)));
          cBuf.writeUInt16LE(getVertIdx(vx, vy, vz), cur + c * 2);
        }
        cBuf[cur + 8]  = q.uvs[0][0];
        cBuf[cur + 9]  = q.uvs[0][1];
        cBuf.writeUInt16LE(q.clut, cur + 10);
        cBuf[cur + 12] = q.uvs[1][0];
        cBuf[cur + 13] = q.uvs[1][1];
        cBuf.writeUInt16LE(q.tpage, cur + 14);
        cBuf[cur + 16] = q.uvs[2][0];
        cBuf[cur + 17] = q.uvs[2][1];
        cBuf[cur + 18] = q.uvs[3][0];
        cBuf[cur + 19] = q.uvs[3][1];
        cBuf[cur + 20] = 0;
        cBuf.writeInt8(Math.max(-128, Math.min(127, q.depthBias)), cur + 21);
        cBuf[cur + 22] = q.uLevel;
        cBuf[cur + 23] = q.vLevel;
        cBuf.writeUInt32LE(0x80000000, cur + 24);
        cBuf[cur + 28] = q.flatColor[0];
        cBuf[cur + 29] = q.flatColor[1];
        cBuf[cur + 30] = q.flatColor[2];
        cBuf[cur + 31] = 0x2c;
        if (mode === 4) {
          cBuf.writeUInt32LE(q.rawWin >>> 0, cur + 32);
        }
        cur += stride;
      }
    }
    cBuf.writeUInt32LE(0, cur);
    cellBuffers.push(cBuf);
  }

  const headerSize = 8 + numCells * 4;
  let streamCursor = headerSize;
  const cellOffsets = [];
  for (const cBuf of cellBuffers) {
    cellOffsets.push(streamCursor);
    streamCursor += cBuf.length;
  }
  const vertsStartRel = streamCursor;
  const totalVertsBytes = vertList.length * 8;
  const b7 = Buffer.alloc(6144 + vertsStartRel + totalVertsBytes);

  const activeCellLookup = new Map();
  cells.forEach((c, meshIdx) => activeCellLookup.set(c.cellIdx, meshIdx));
  for (let c = 0; c < 1024; c++) {
    const model = activeCellLookup.has(c) ? activeCellLookup.get(c) : 1023;
    b7.writeUInt16LE(model, c * 2);
  }
  b7.fill(0xff, 2048, 6144);

  b7.writeUInt32LE(numCells, 6144 + 0);
  b7.writeUInt32LE(vertsStartRel, 6144 + 4);
  for (let m = 0; m < numCells; m++) {
    b7.writeUInt32LE(cellOffsets[m], 6144 + 8 + m * 4);
    cellBuffers[m].copy(b7, 6144 + cellOffsets[m]);
  }
  for (let v = 0; v < vertList.length; v++) {
    const vOff = 6144 + vertsStartRel + v * 8;
    b7.writeInt16LE(vertList[v][0], vOff + 0);
    b7.writeInt16LE(vertList[v][1], vOff + 2);
    b7.writeInt16LE(vertList[v][2], vOff + 4);
    b7.writeInt16LE(0, vOff + 6);
  }
  return b7;
}

function detectTunnelAndBridgeZones(courseIdx, r4Road, quads) {
  if (courseIdx === 0) {
    return [
      { kStart: 26,  kEnd: 33,  code: 1,  val: 45  },
      { kStart: 61,  kEnd: 67,  code: 1,  val: 45  },
      { kStart: 78,  kEnd: 82,  code: 1,  val: 45  },
      { kStart: 98,  kEnd: 145, code: 0,  val: 110 },
      { kStart: 180, kEnd: 183, code: 1,  val: 40  },
      { kStart: 209, kEnd: 336, code: 0,  val: 110 },
      { kStart: 375, kEnd: 377, code: -1, val: 0   },
      { kStart: 411, kEnd: 420, code: 1,  val: 45  },
      { kStart: 454, kEnd: 497, code: 0,  val: 110 },
      { kStart: 506, kEnd: 511, code: 1,  val: 45  },
    ];
  }
  const nPts = r4Road.length;
  const covered = new Uint8Array(nPts);
  for (let k = 0; k < nPts; k++) {
    const rp = r4Road[k];
    for (let q = 0; q < quads.length; q++) {
      const qd = quads[q];
      const dy = qd.cy - rp.y;
      if (dy < -420 || dy > -38) continue;
      if (Math.abs(qd.cx - rp.x) < 340 && Math.abs(qd.cz - rp.z) < 340) {
        covered[k] = 1;
        break;
      }
    }
  }
  // Bridge 1-point gaps
  for (let k = 1; k + 1 < nPts; k++) {
    if (!covered[k] && covered[k - 1] && covered[k + 1]) covered[k] = 1;
  }
  const runs = [];
  let inRun = false, rStart = 0;
  for (let k = 0; k < nPts; k++) {
    if (covered[k] && !inRun) { inRun = true; rStart = k; }
    else if (!covered[k] && inRun) {
      inRun = false;
      if (k - rStart >= 2) runs.push({ kStart: rStart, kEnd: k - 1, len: k - rStart });
    }
  }
  if (inRun && nPts - rStart >= 2) runs.push({ kStart: rStart, kEnd: nPts - 1, len: nPts - rStart });
  return runs
    .sort((a, b) => b.len - a.len)
    .slice(0, 20)
    .sort((a, b) => a.kStart - b.kStart)
    .map(r => ({
      kStart: r.kStart,
      kEnd: r.kEnd,
      code: r.len >= 14 ? 0 : 1,
      val: r.len >= 14 ? 110 : 45,
    }));
}

function convertSingleR4Track({ courseSpec, texEntryBuf, trackEntryBuf, b87_0, b87_1, origPack088, origScene088 }) {
  // 1. Relocate R4 Texture Pack (entry[2*c]) into PS1 VRAM container (asset_135..149.bin)
  const b0Off = texEntryBuf.readUInt32LE(4);
  const b1Off = texEntryBuf.readUInt32LE(8);
  const rawB0 = texEntryBuf.subarray(b0Off, b1Off);
  const rawB1 = texEntryBuf.subarray(b1Off);

  const r4RelocChain = buildRelocatedImageChain([
    { buf: rawB0, isSky: false },
    { buf: rawB1, isSky: true },
  ]);

  const b87_2 = Buffer.alloc(20);
  b87_2.writeUInt32LE(12, 8);
  const minBlock3Size = Math.max(r4RelocChain.length, 0x38000);
  const b87_3 = Buffer.alloc(minBlock3Size);
  r4RelocChain.copy(b87_3, 0);
  const b87_4 = Buffer.alloc(8);
  b87_4.writeInt32LE(256, 0);
  b87_4.writeInt32LE(0, 4);

  const b87Blocks = [b87_0, b87_1, b87_2, b87_3, b87_4];
  const b87Header = Buffer.alloc(20);
  let b87Cursor = 20;
  for (let i = 0; i < 5; i++) {
    b87Header.writeUInt32LE(b87Cursor, i * 4);
    b87Cursor += b87Blocks[i].length;
  }
  const outTexPack = Buffer.concat([b87Header, ...b87Blocks]);

  // 2. Parse R4 TrackPack (entry[2*c + 1])
  const nBlocks = trackEntryBuf.readUInt32LE(0);
  const bo = [];
  for (let i = 0; i < nBlocks; i++) bo.push(trackEntryBuf.readUInt32LE(4 + i * 4));
  bo.push(trackEntryBuf.length);

  const b0 = trackEntryBuf.subarray(bo[0], bo[1]);
  const b1 = trackEntryBuf.subarray(bo[1], bo[2]);
  const b4 = trackEntryBuf.subarray(bo[4], bo[5]);

  // Read exact quad stream offsets from Block 0 header (+0x20..+0x30)
  const streams = [
    { name: 'subA', start: b0.readUInt32LE(0x20), end: b0.readUInt32LE(0x24), stride: 64, gouraud: true,  hasWin: false },
    { name: 'subB', start: b0.readUInt32LE(0x24), end: b0.readUInt32LE(0x28), stride: 68, gouraud: true,  hasWin: true  },
    { name: 'subC', start: b0.readUInt32LE(0x28), end: b0.readUInt32LE(0x2c), stride: 48, gouraud: false, hasWin: false },
    { name: 'subD', start: b0.readUInt32LE(0x2c), end: b0.readUInt32LE(0x30), stride: 52, gouraud: false, hasWin: true  },
  ];

  // Compute dynamic Y_OFFSET so all SVECTOR vy*4 stay within [-32000..+32000] and horizon sits near +6000
  let minRawY = Infinity, maxRawY = -Infinity;
  for (const s of streams) {
    for (let off = s.start; off + s.stride <= s.end; off += s.stride) {
      for (const yOff of [2, 10, 14, 22]) {
        const qy = b0.readInt16LE(off + yOff);
        if (qy < minRawY) minRawY = qy;
        if (qy > maxRawY) maxRawY = qy;
      }
    }
  }
  const Y_OFFSET = Math.max(5000, Math.min(8000 - maxRawY, 500 - minRawY));

  // 3. Parse R4 road points from Block 4
  const numR4Pts = Math.floor((b4.length - 0x808) / 60);
  const r4Road = [];
  let minRoadX = Infinity, maxRoadX = -Infinity;
  let minRoadY = Infinity, maxRoadY = -Infinity;
  let minRoadZ = Infinity, maxRoadZ = -Infinity;

  for (let i = 0; i < numR4Pts; i++) {
    const rp = b4.subarray(0x808 + i * 60, 0x808 + (i + 1) * 60);
    const halfAsphaltL = rp.readInt16LE(44) * 4 || 328;
    const halfAsphaltR = rp.readInt16LE(46) * 4 || 328;
    const rx = rp.readUInt32LE(8);
    const rz = rp.readUInt32LE(12);
    const ry = Math.round(rp.readInt16LE(16) / 4) + Y_OFFSET;
    if (rx < minRoadX) minRoadX = rx;
    if (rx > maxRoadX) maxRoadX = rx;
    if (ry < minRoadY) minRoadY = ry;
    if (ry > maxRoadY) maxRoadY = ry;
    if (rz < minRoadZ) minRoadZ = rz;
    if (rz > maxRoadZ) maxRoadZ = rz;
    r4Road.push({
      x: rx,
      z: rz,
      y: ry,
      r4Pitch: rp.readInt16LE(20),
      crossSlope: -rp.readInt16LE(22),
      tangentAngle: ((rp.readInt16LE(30) - 1024) % 4096 + 4096) % 4096,
      leftWidth: Math.max(320, Math.min(900, halfAsphaltL + 80)),
      rightWidth: Math.max(320, Math.min(900, halfAsphaltR + 80)),
    });
  }

  // 4. Parse all R4 terrain/track quads from Block 0
  const quads = [];
  for (const s of streams) {
    for (let off = s.start; off + s.stride <= s.end; off += s.stride) {
      const q = b0.subarray(off, off + s.stride);
      const verts = [
        [q.readInt16LE(0),  q.readInt16LE(2),  q.readInt16LE(4)],
        [q.readInt16LE(8),  q.readInt16LE(10), q.readInt16LE(6)],
        [q.readInt16LE(12), q.readInt16LE(14), q.readInt16LE(16)],
        [q.readInt16LE(20), q.readInt16LE(22), q.readInt16LE(18)],
      ];
      const uvs = [
        [q[24], q[25]],
        [q[28], q[29]],
        [q[32], q[33]],
        [q[34], q[35]],
      ];
      const clut = q.readUInt16LE(26) & 0x7fff;
      const tpage = remapTpage(q.readUInt16LE(30) & 0x1ff);
      const rawWin = s.hasWin ? q.readUInt32LE(s.stride - 4) : 0;
      const win = s.hasWin ? decodeWindow(rawWin) : [0, 0, 0, 0];
      const hasValidWin = win[0] !== 0 && win[1] !== 0;

      let colors, flatColor;
      if (s.gouraud) {
        colors = [
          scaleVertexColor([q[48], q[49], q[50]]),
          scaleVertexColor([q[52], q[53], q[54]]),
          scaleVertexColor([q[56], q[57], q[58]]),
          scaleVertexColor([q[60], q[61], q[62]]),
        ];
        flatColor = [
          Math.round((colors[0][0] + colors[1][0] + colors[2][0] + colors[3][0]) * 0.25),
          Math.round((colors[0][1] + colors[1][1] + colors[2][1] + colors[3][1]) * 0.25),
          Math.round((colors[0][2] + colors[1][2] + colors[2][2] + colors[3][2]) * 0.25),
        ];
      } else {
        const r = q[44], g = q[45], b = q[46];
        const c = (r === 0 && g === 0 && b === 0) ? [128, 128, 128] : scaleVertexColor([r, g, b]);
        colors = [c, c, c, c];
        flatColor = c;
      }

      const worldVerts = verts.map(([qx, qy, qz]) => [
        qx + 32768,
        qy + Y_OFFSET,
        qz + 32768,
      ]);
      const cx = (worldVerts[0][0] + worldVerts[1][0] + worldVerts[2][0] + worldVerts[3][0]) * 0.25;
      const cy = (worldVerts[0][1] + worldVerts[1][1] + worldVerts[2][1] + worldVerts[3][1]) * 0.25;
      const cz = (worldVerts[0][2] + worldVerts[1][2] + worldVerts[2][2] + worldVerts[3][2]) * 0.25;
      const gx = Math.max(0, Math.min(31, Math.floor(cx / 2048)));
      const gz = Math.max(0, Math.min(31, Math.floor(cz / 2048)));
      const cellIdx = (31 - gz) * 32 + gx;

      const depthBias = q.readInt8(37);
      const uLevel = q[38] & 7;
      const vLevel = (q[38] >> 5) & 7;

      quads.push({
        worldVerts,
        origWorldVerts: worldVerts.map(v => v.slice()),
        uvs,
        colors,
        flatColor,
        tpage,
        clut,
        win,
        rawWin: hasValidWin ? rawWin : 0,
        hasValidWin,
        depthBias,
        uLevel,
        vLevel,
        gx,
        gz,
        cellIdx,
        cx,
        cy,
        cz,
      });
    }
  }

  // Group into cells in exact Block 7 scan order so NativeAssetImporter material indices match 1:1
  const classicCellMap = new Map();
  for (const q of quads) {
    if (!classicCellMap.has(q.cellIdx)) {
      classicCellMap.set(q.cellIdx, { cellIdx: q.cellIdx, gx: q.gx, gz: q.gz, mode2: [], mode4: [] });
    }
    const cell = classicCellMap.get(q.cellIdx);
    if (q.hasValidWin) cell.mode4.push(q);
    else cell.mode2.push(q);
  }
  const classicCells = [...classicCellMap.values()].sort((a, b) => a.cellIdx - b.cellIdx);

  const matMap = new Map();
  const matList = [];
  for (const cell of classicCells) {
    for (const q of [...cell.mode2, ...cell.mode4]) {
      const fullKey = `${q.tpage}:${q.clut}:${q.win.join(':')}`;
      let matIdx = matMap.get(fullKey);
      if (matIdx === undefined) {
        matIdx = matList.length;
        matMap.set(fullKey, matIdx);
        matList.push([q.tpage, q.clut, ...q.win]);
      }
      q.matIdx = matIdx;
    }
  }

  // Subdivide decals & snap coplanar overlays onto parent surface triangle planes
  const processedQuads = [];
  for (const q of quads) processedQuads.push(...subdivideQuadIfDecal(q));

  const spatialGrid = new Map();
  for (const q of processedQuads) {
    const gx0 = Math.floor(q.minX / 512), gx1 = Math.floor(q.maxX / 512);
    const gz0 = Math.floor(q.minZ / 512), gz1 = Math.floor(q.maxZ / 512);
    for (let gx = gx0; gx <= gx1; gx++) {
      for (let gz = gz0; gz <= gz1; gz++) {
        const k = `${gx}:${gz}`;
        if (!spatialGrid.has(k)) spatialGrid.set(k, []);
        spatialGrid.get(k).push(q);
      }
    }
  }

  const sortedForSnap = processedQuads.slice().sort((a, b) => b.depthBias - a.depthBias);
  for (const fg of sortedForSnap) {
    const gx0 = Math.floor(fg.minX / 512), gx1 = Math.floor(fg.maxX / 512);
    const gz0 = Math.floor(fg.minZ / 512), gz1 = Math.floor(fg.maxZ / 512);
    const candidates = new Set();
    for (let gx = gx0; gx <= gx1; gx++) {
      for (let gz = gz0; gz <= gz1; gz++) {
        for (const o of (spatialGrid.get(`${gx}:${gz}`) || [])) {
          if (o.depthBias > fg.depthBias) candidates.add(o);
        }
      }
    }
    let bestBg = null, bestOverlap = 0;
    for (const bg of candidates) {
      if (fg.maxX < bg.minX - 2 || fg.minX > bg.maxX + 2 ||
          fg.maxY < bg.minY - 2 || fg.minY > bg.maxY + 2 ||
          fg.maxZ < bg.minZ - 2 || fg.minZ > bg.maxZ + 2) continue;
      const dot = fg.nx * bg.nx + fg.ny * bg.ny + fg.nz * bg.nz;
      if (dot < 0.94) continue;
      const dist = (fg.rcx - bg.rcx) * bg.nx + (fg.rcy - bg.rcy) * bg.ny + (fg.rcz - bg.rcz) * bg.nz;
      if (Math.abs(dist) > 2.5) continue;
      const ax = Math.abs(bg.nx), ay = Math.abs(bg.ny), az = Math.abs(bg.nz);
      const [uIdx, vIdx] = (ay >= ax && ay >= az) ? [0, 2] : (ax >= az ? [1, 2] : [0, 1]);
      const polyFg = [fg.rv[0], fg.rv[1], fg.rv[3], fg.rv[2]].map(v => [v[uIdx], v[vIdx]]);
      const polyBg = [bg.rv[0], bg.rv[1], bg.rv[3], bg.rv[2]].map(v => [v[uIdx], v[vIdx]]);
      const ov = polyArea2D(clipPoly2D(polyFg, polyBg));
      if (ov > Math.max(20, bestOverlap)) {
        bestOverlap = ov;
        bestBg = bg;
      }
    }
    if (bestBg) {
      const isRoad = Math.abs(bestBg.ny) >= 0.85;
      const ax = Math.abs(bestBg.nx), ay = Math.abs(bestBg.ny), az = Math.abs(bestBg.nz);
      const [uIdx, vIdx] = (ay >= ax && ay >= az) ? [0, 2] : (ax >= az ? [1, 2] : [0, 1]);
      const d1u = bestBg.rv[1][uIdx], d1v = bestBg.rv[1][vIdx];
      const d2u = bestBg.rv[2][uIdx], d2v = bestBg.rv[2][vIdx];
      const r0u = bestBg.rv[0][uIdx], r0v = bestBg.rv[0][vIdx];
      const r0Side = (d2u - d1u) * (r0v - d1v) - (d2v - d1v) * (r0u - d1u);
      for (let c = 0; c < 4; c++) {
        const v = fg.rv[c];
        const vu = v[uIdx], vv = v[vIdx];
        const vSide = (d2u - d1u) * (vv - d1v) - (d2v - d1v) * (vu - d1u);
        const tri = (vSide * r0Side >= 0) ? bestBg.t0 : bestBg.t1;
        if (isRoad && Math.abs(tri.ny) > 0.5) {
          v[1] = tri.p[1] - (tri.nx * (v[0] - tri.p[0]) + tri.nz * (v[2] - tri.p[2])) / tri.ny;
        } else {
          const vd = (v[0] - tri.p[0]) * tri.nx + (v[1] - tri.p[1]) * tri.ny + (v[2] - tri.p[2]) * tri.nz;
          const shift = 0.01 - vd;
          v[0] += tri.nx * shift;
          v[1] += tri.ny * shift;
          v[2] += tri.nz * shift;
        }
        fg.worldVerts[c] = [v[0], -v[1], -v[2]];
      }
      const origCellIdx = fg.cellIdx, origGx = fg.gx, origGz = fg.gz;
      updateQuadMeta(fg);
      fg.cellIdx = origCellIdx; fg.gx = origGx; fg.gz = origGz;
    }
  }

  const cellMap = new Map();
  for (const q of processedQuads) {
    if (!cellMap.has(q.cellIdx)) {
      cellMap.set(q.cellIdx, { cellIdx: q.cellIdx, gx: q.gx, gz: q.gz, quads: [] });
    }
    cellMap.get(q.cellIdx).quads.push(q);
  }
  const activeCells = [...cellMap.values()].sort((a, b) => a.cellIdx - b.cellIdx);

  const skyPalette = extractSkyPaletteFromB1(b1);
  const terrainRmeshBuf = buildTerrainRmesh(activeCells);
  const courseRmeshBuf = buildSkyCourseRmesh(b1, skyPalette.bottom);

  // 5. Build Block 4 road spline & 3D wall distances
  const rrCount = numR4Pts;
  const START_RR_IDX = (rrCount - 73 + rrCount) % rrCount;
  const sampledRoad = new Array(rrCount);
  for (let k = 0; k < rrCount; k++) {
    const rrIdx = (START_RR_IDX - k + rrCount) % rrCount;
    sampledRoad[rrIdx] = { ...r4Road[k], r4Idx: k };
  }

  function signedAngleDelta(from, to) {
    return ((to - from + 2048) % 4096 + 4096) % 4096 - 2048;
  }

  for (let i = 0; i < rrCount; i++) {
    const p0 = sampledRoad[i];
    const p1 = sampledRoad[(i + 1) % rrCount];
    const dx = p1.x - p0.x;
    const dz = p1.z - p0.z;
    const chord = Math.max(1, Math.hypot(dx, dz));
    const chordAngle = ((Math.round((Math.atan2(dz, dx) * 4096) / (2 * Math.PI)) % 4096) + 4096) % 4096;
    const dAngle = signedAngleDelta(p0.tangentAngle, p1.tangentAngle);
    const halfTheta = (dAngle * Math.PI) / 4096;
    const r = dAngle !== 0 ? Math.abs((chord * 0.5) / Math.sin(halfTheta)) : Infinity;
    p0.chord = chord;
    p0.chordAngle = chordAngle;
    p0.isCurved = Math.abs(dAngle) >= 6 && r <= 26000;
    p0.angle = p0.isCurved ? p0.tangentAngle : chordAngle;
  }

  const arcs = [];
  for (let i = 0; i < rrCount; i++) {
    const p0 = sampledRoad[i];
    const p1 = sampledRoad[(i + 1) % rrCount];
    const dx = p1.x - p0.x;
    const dz = p1.z - p0.z;
    const dy = p0.y - p1.y;
    const chord = p0.chord;
    p0.pitch = Math.max(-512, Math.min(512, Math.round((Math.atan2(dy, chord) * 4096) / (2 * Math.PI))));
    p0.length = Math.max(1, Math.round(chord));

    const dAngle = signedAngleDelta(p0.angle, p1.angle);
    const halfTheta = (dAngle * Math.PI) / 4096;
    const r = dAngle !== 0 ? Math.abs((chord * 0.5) / Math.sin(halfTheta)) : Infinity;
    if (p0.isCurved && Math.abs(dAngle) >= 4 && r <= 28000) {
      const h = (chord * 0.5) / Math.tan(halfTheta);
      const arcX = Math.round((p0.x + p1.x) * 0.5 - (dz / chord) * h);
      const arcZ = Math.round((p0.z + p1.z) * 0.5 + (dx / chord) * h);
      const mode = dAngle < 0 ? 1 : 2;
      const arcIdx = arcs.length;
      arcs.push({ x: arcX, z: arcZ });
      p0.arc = (arcIdx << 4) | mode;
    } else {
      p0.angle = p0.chordAngle;
      p0.arc = 0;
    }
  }

  const wallCandidates = [];
  for (const q of quads) {
    const v = q.origWorldVerts;
    const ax = v[1][0] - v[0][0], ay = v[1][1] - v[0][1], az = v[1][2] - v[0][2];
    const bx = v[2][0] - v[0][0], by = v[2][1] - v[0][1], bz = v[2][2] - v[0][2];
    const nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
    const len = Math.hypot(nx, ny, nz);
    if (len > 1 && Math.abs(ny / len) <= 0.55) {
      wallCandidates.push({
        v,
        qcx: (v[0][0] + v[1][0] + v[2][0] + v[3][0]) * 0.25,
        qcz: (v[0][2] + v[1][2] + v[2][2] + v[3][2]) * 0.25,
        nx: nx / len,
        ny: ny / len,
        nz: nz / len,
      });
    }
  }

  for (let i = 0; i < rrCount; i++) {
    const p = sampledRoad[i];
    const rad = (p.angle * 2 * Math.PI) / 4096;
    const dirX = Math.cos(rad), dirZ = Math.sin(rad);
    const latX = -dirZ, latZ = dirX;
    let wL = Infinity, wR = Infinity;
    let ptL = [p.x - latX * p.leftWidth, p.z - latZ * p.leftWidth];
    let ptR = [p.x + latX * p.rightWidth, p.z + latZ * p.rightWidth];
    for (const w of wallCandidates) {
      if (Math.hypot(w.qcx - p.x, w.qcz - p.z) > 950) continue;
      if (Math.abs(w.nx * latX + w.nz * latZ) < 0.65) continue;
      const avgLat = (w.qcx - p.x) * latX + (w.qcz - p.z) * latZ;
      const localRoadY = p.y + (p.crossSlope * avgLat) / 128;
      const minY = Math.min(w.v[0][1], w.v[1][1], w.v[2][1], w.v[3][1]);
      const maxY = Math.max(w.v[0][1], w.v[1][1], w.v[2][1], w.v[3][1]);
      if (maxY < localRoadY - 25 || minY > localRoadY - 8) continue;
      for (const [e0, e1] of [[w.v[0], w.v[2]], [w.v[1], w.v[3]], [w.v[0], w.v[1]], [w.v[2], w.v[3]]]) {
        const s0 = (e0[0] - p.x) * dirX + (e0[2] - p.z) * dirZ;
        const s1 = (e1[0] - p.x) * dirX + (e1[2] - p.z) * dirZ;
        if ((s0 <= 15 && s1 >= -15) || (s1 <= 15 && s0 >= -15)) {
          const t = Math.abs(s1 - s0) > 1e-3 ? Math.max(0, Math.min(1, -s0 / (s1 - s0))) : 0.5;
          const wx = e0[0] + t * (e1[0] - e0[0]);
          const wz = e0[2] + t * (e1[2] - e0[2]);
          const lat = (wx - p.x) * latX + (wz - p.z) * latZ;
          if (lat < -150 && -lat < wL) {
            wL = -lat;
            ptL = [wx, wz];
          }
          if (lat > 150 && lat < wR) {
            wR = lat;
            ptR = [wx, wz];
          }
        }
      }
    }
    p.wallL = Number.isFinite(wL) ? Math.round(wL) : p.leftWidth;
    p.wallR = Number.isFinite(wR) ? Math.round(wR) : p.rightWidth;
    p.ptL = ptL;
    p.ptR = ptR;
  }

  function getAngleDist12(a, b) {
    let d = (b - a) & 0xfff;
    if (d >= 2048) d = 4096 - d;
    return d;
  }

  function evalCarTrackLat(segIdx, wx, wz) {
    const p0 = sampledRoad[segIdx];
    const p1 = sampledRoad[(segIdx + 1) % rrCount];
    const rad = (p0.angle * 2 * Math.PI) / 4096;
    const cosH = Math.cos(rad), sinH = Math.sin(rad);
    let lat = -sinH * (wx - p0.x) + cosH * (wz - p0.z);
    const curveMode = p0.arc & 3;
    if (curveMode !== 0) {
      const arcCenter = arcs[p0.arc >> 4];
      const r0 = Math.hypot(p0.x - arcCenter.x, p0.z - arcCenter.z);
      const r1 = Math.hypot(p1.x - arcCenter.x, p1.z - arcCenter.z);
      const rc = Math.hypot(wx - arcCenter.x, wz - arcCenter.z);
      const a0 = ((Math.round((Math.atan2(p0.z - arcCenter.z, p0.x - arcCenter.x) * 4096) / (2 * Math.PI)) % 4096) + 4096) % 4096;
      const a1 = ((Math.round((Math.atan2(p1.z - arcCenter.z, p1.x - arcCenter.x) * 4096) / (2 * Math.PI)) % 4096) + 4096) % 4096;
      const ac = ((Math.round((Math.atan2(wz - arcCenter.z, wx - arcCenter.x) * 4096) / (2 * Math.PI)) % 4096) + 4096) % 4096;
      const span = Math.max(1, getAngleDist12(a0, a1));
      const swept = getAngleDist12(a0, ac);
      const interpR = (swept * r0 + (span - swept) * r1) / span;
      lat = rc - interpR;
      if (curveMode === 2) lat = -lat;
    }
    return lat;
  }

  const WALL_CLEARANCE = 8;
  const sagittaTrimL = new Array(rrCount).fill(0);
  const sagittaTrimR = new Array(rrCount).fill(0);
  for (let i = 0; i < rrCount; i++) {
    const p0 = sampledRoad[i];
    const p1 = sampledRoad[(i + 1) % rrCount];
    for (const t of [0.25, 0.5, 0.75]) {
      const wxL = p0.ptL[0] + t * (p1.ptL[0] - p0.ptL[0]);
      const wzL = p0.ptL[1] + t * (p1.ptL[1] - p0.ptL[1]);
      const wxR = p0.ptR[0] + t * (p1.ptR[0] - p0.ptR[0]);
      const wzR = p0.ptR[1] + t * (p1.ptR[1] - p0.ptR[1]);
      const latL = -evalCarTrackLat(i, wxL, wzL);
      const latR = evalCarTrackLat(i, wxR, wzR);
      const interpWL = p0.wallL + t * (p1.wallL - p0.wallL);
      const interpWR = p0.wallR + t * (p1.wallR - p0.wallR);
      const excessL = Math.max(0, Math.ceil(interpWL - latL));
      const excessR = Math.max(0, Math.ceil(interpWR - latR));
      if (excessL > 0) {
        sagittaTrimL[i] = Math.max(sagittaTrimL[i], excessL);
        sagittaTrimL[(i + 1) % rrCount] = Math.max(sagittaTrimL[(i + 1) % rrCount], excessL);
      }
      if (excessR > 0) {
        sagittaTrimR[i] = Math.max(sagittaTrimR[i], excessR);
        sagittaTrimR[(i + 1) % rrCount] = Math.max(sagittaTrimR[(i + 1) % rrCount], excessR);
      }
    }
  }

  for (let i = 0; i < rrCount; i++) {
    const p = sampledRoad[i];
    p.leftWidth = Math.max(180, p.wallL - sagittaTrimL[i] - WALL_CLEARANCE);
    p.rightWidth = Math.max(180, p.wallR - sagittaTrimR[i] - WALL_CLEARANCE);
  }

  const newBlock4 = Buffer.alloc(4 + rrCount * 24 + arcs.length * 12);
  newBlock4.writeUInt32LE(rrCount, 0);
  for (let i = 0; i < rrCount; i++) {
    const p = sampledRoad[i];
    const off = 4 + i * 24;
    newBlock4.writeInt32LE(p.x, off + 0);
    newBlock4.writeInt32LE(p.z, off + 4);
    newBlock4.writeInt16LE(p.y, off + 8);
    newBlock4.writeInt16LE(p.angle, off + 10);
    newBlock4.writeInt16LE(p.pitch, off + 12);
    newBlock4.writeInt16LE(p.crossSlope, off + 14);
    newBlock4.writeInt16LE(p.leftWidth, off + 16);
    newBlock4.writeInt16LE(p.rightWidth, off + 18);
    newBlock4.writeUInt16LE(p.arc, off + 20);
    newBlock4.writeUInt16LE(p.length, off + 22);
  }
  for (let a = 0; a < arcs.length; a++) {
    const off = 4 + rrCount * 24 + a * 12;
    newBlock4.writeInt32LE(arcs[a].x, off + 0);
    newBlock4.writeInt32LE(arcs[a].z, off + 4);
    newBlock4.writeInt32LE(0, off + 8);
  }

  // 6. Assemble 11-block TrackPack (asset_136..150.bin)
  const origBlocks = [];
  for (let b = 0; b < 11; b++) {
    const start = origPack088.readUInt32LE(b * 4);
    const end = b + 1 < 11 ? origPack088.readUInt32LE((b + 1) * 4) : origPack088.length;
    origBlocks.push(Buffer.from(origPack088.subarray(start, end)));
  }

  origBlocks[1].fill(0);
  origBlocks[2].writeUInt32LE(0, 0);
  const envPalette = [
    skyPalette.horizon, // 0: ENV_FOG
    skyPalette.zenith,  // 1: ENV_SKY_TOP
    skyPalette.mid,     // 2: ENV_SKY_MIDDLE
    skyPalette.horizon, // 3: ENV_SKY_HORIZON
    skyPalette.bottom,  // 4: ENV_SKY_BOTTOM
    skyPalette.horizon, // 5: ENV_GROUND_NEAR_TOP
    skyPalette.bottom,  // 6: ENV_GROUND_NEAR_BOTTOM
    skyPalette.horizon, // 7: ENV_GROUND_FAR_TOP
    skyPalette.bottom,  // 8: ENV_GROUND_FAR_BOTTOM
  ];
  for (let c = 0; c < 5; c++) {
    const cueOff = 8 + c * 48;
    for (let s = 0; s < 9; s++) {
      const colOff = cueOff + 4 + s * 4;
      origBlocks[2][colOff + 0] = envPalette[s][0];
      origBlocks[2][colOff + 1] = envPalette[s][1];
      origBlocks[2][colOff + 2] = envPalette[s][2];
      origBlocks[2][colOff + 3] = 0;
    }
  }

  origBlocks[4] = newBlock4;
  origBlocks[5] = buildCleanBlock5();
  origBlocks[7] = buildFullBlock7(classicCells);

  const centerRoadX = (minRoadX + maxRoadX) >> 1;
  const centerRoadZ = (minRoadZ + maxRoadZ) >> 1;
  const skyPlacementY = Math.round((minRoadY + maxRoadY) * 0.5 - 288);
  const numPlacements = origBlocks[8].readUInt32LE(0);
  for (let i = 0; i < numPlacements; i++) {
    const pOff = 4 + i * 20;
    if (i === 0) {
      origBlocks[8].writeInt16LE(0, pOff + 0);
      origBlocks[8].writeInt16LE(0, pOff + 2);
      origBlocks[8].writeInt32LE(centerRoadX, pOff + 4);
      origBlocks[8].writeInt32LE(skyPlacementY, pOff + 8);
      origBlocks[8].writeInt32LE(centerRoadZ, pOff + 12);
      origBlocks[8].writeUInt32LE(0, pOff + 16);
    } else {
      origBlocks[8].writeInt16LE(-1, pOff + 0);
    }
  }

  const b9 = origBlocks[9];
  b9.writeInt32LE(START_RR_IDX, 0);
  for (let c = 0; c < 16; c++) {
    b9.writeInt32LE(-1, 4 + c * 8 + 4);
  }
  for (let h = 0; h < 60; h++) {
    const hOff = 132 + h * 12;
    if (b9.readInt16LE(hOff) !== -1) {
      b9.writeInt16LE(0, hOff + 4);
      b9.writeInt16LE(0, hOff + 6);
      b9.writeInt16LE(0, hOff + 8);
    }
  }

  const cumDist = new Array(rrCount + 1);
  cumDist[0] = 0;
  for (let k = 0; k < rrCount; k++) {
    const rrIdx = (START_RR_IDX - k + rrCount) % rrCount;
    const prevIdx = (rrIdx - 1 + rrCount) % rrCount;
    cumDist[k + 1] = cumDist[k] + sampledRoad[prevIdx].length;
  }

  for (let i = 0; i < 10; i++) {
    const off = 2300 + i * 12;
    b9.writeInt32LE(-1, off + 0);
    b9.writeInt32LE(-1, off + 4);
  }
  for (let i = 0; i < 16; i++) {
    const off = 2420 + i * 16;
    b9.writeInt32LE(-1, off + 0);
    b9.writeInt32LE(-1, off + 4);
  }

  const r4Zones = detectTunnelAndBridgeZones(courseSpec.course, r4Road, quads);
  for (let z = 0; z < 20; z++) {
    const zOff = 2676 + z * 12;
    if (z < r4Zones.length) {
      const rz = r4Zones[z];
      b9.writeInt32LE(cumDist[rz.kStart], zOff + 0);
      b9.writeInt32LE(cumDist[rz.kEnd], zOff + 4);
      b9.writeInt16LE(rz.code, zOff + 8);
      b9.writeInt16LE(rz.val, zOff + 10);
    } else {
      b9.writeInt32LE(-1, zOff + 0);
      b9.writeInt32LE(-1, zOff + 4);
      b9.writeInt16LE(0, zOff + 8);
      b9.writeInt16LE(0, zOff + 10);
    }
  }

  const ORIG_START_IDX = 299;
  for (let g = 0; g < 2; g++) {
    for (let s = 0; s < 12; s++) {
      const origSlot = origScene088.ai.starts[g][s];
      const deltaPts = origSlot.point - ORIG_START_IDX;
      const ptIdx = (START_RR_IDX + deltaPts + rrCount * 4) % rrCount;
      const p0 = sampledRoad[ptIdx];
      const pPrev = sampledRoad[(ptIdx - 1 + rrCount) % rrCount];
      const fx = pPrev.x - p0.x;
      const fz = pPrev.z - p0.z;
      const flen = Math.max(1, Math.hypot(fx, fz));
      const ux = fx / flen;
      const uz = fz / flen;
      const side = (s % 2 === 0 ? -1 : 1) * 90;
      const rx = -uz;
      const rz = ux;
      const sx = Math.max(0, Math.min(65535, Math.round(p0.x + rx * side)));
      const sz = Math.max(0, Math.min(65535, Math.round(p0.z + rz * side)));
      const sOff = 852 + (g * 12 + s) * 12;
      b9.writeInt32LE(sx, sOff + 0);
      b9.writeInt32LE(sz, sOff + 4);
      b9.writeInt16LE(ptIdx, sOff + 8);
      b9.writeInt16LE(origSlot.seed, sOff + 10);

      if (s === 0 && g === 0) {
        const fKeys = [
          { off: 3344, fwd: 1100, right: -140, up: -55 },
          { off: 3364, fwd: 1100, right: -140, up: -55 },
          { off: 3384, fwd: 180,  right: -70,  up: -45 },
          { off: 3404, fwd: -140, right: -80,  up: -45 },
        ];
        for (const k of fKeys) {
          b9.writeInt32LE(Math.round(sx + ux * k.fwd + rx * k.right), k.off + 0);
          b9.writeInt32LE(Math.round(p0.y + k.up), k.off + 4);
          b9.writeInt32LE(Math.round(sz + uz * k.fwd + rz * k.right), k.off + 8);
        }
      } else if (s === 0 && g === 1) {
        const rKeys = [
          { off: 3444, fwd: -1100, right: 140, up: -55 },
          { off: 3464, fwd: -1100, right: 140, up: -55 },
          { off: 3484, fwd: 140,   right: 80,  up: -45 },
        ];
        for (const k of rKeys) {
          b9.writeInt32LE(Math.round(sx + ux * k.fwd + rx * k.right), k.off + 0);
          b9.writeInt32LE(Math.round(p0.y + k.up), k.off + 4);
          b9.writeInt32LE(Math.round(sz + uz * k.fwd + rz * k.right), k.off + 8);
        }
      }
    }
  }

  let cursor = 44;
  const blockHeader = Buffer.alloc(44);
  for (let b = 0; b < 11; b++) {
    blockHeader.writeUInt32LE(cursor, b * 4);
    cursor += origBlocks[b].length;
  }
  const outTrackPack = Buffer.concat([blockHeader, ...origBlocks]);

  return {
    outTexPack,
    outTrackPack,
    terrainRmeshBuf,
    courseRmeshBuf,
    roadPoints: rrCount,
    quadCount: quads.length,
    cellCount: activeCells.length,
  };
}

async function importR4Tracks(service, inputPath) {
  service.requireReady();
  return service.operation('Importing all 8 R4 tracks', async signal => {
    const { run } = require('./service.cjs');
    await service.loadProject();
    if (!service.editor) throw Error('Open a source project first');

    const reader = await openR4ArchiveReader(inputPath);
    const staging = path.join(service.root, 'r4-import-' + randomUUID());
    await fs.mkdir(path.join(staging, 'raw'), { recursive: true });
    await fs.mkdir(path.join(staging, 'meshes'), { recursive: true });

    try {
      const tableBuf = await reader.readSectors(0, 2);
      const entryCount = tableBuf.readUInt32LE(0);
      if (entryCount < 16) throw Error('R4 archive does not contain all 8 course entries');
      const offsets = [];
      for (let i = 0; i <= 16; i++) offsets.push(tableBuf.readUInt32LE(4 + i * 4));

      const orig087Path = path.join(service.state.disc.data, 'raw/asset_087.bin');
      const orig088Path = path.join(service.state.disc.data, 'raw/asset_088.bin');
      const [orig087, origPack088, origSceneText] = await Promise.all([
        fs.readFile(orig087Path),
        fs.readFile(orig088Path),
        run(service.tool('rage-mod-cli'), ['--track-scene', orig088Path], { signal, maxOutput: 16 * 1024 * 1024 }),
      ]);
      const origScene088 = JSON.parse(origSceneText);

      const b87Offsets = [];
      for (let i = 0; i < 5; i++) b87Offsets.push(orig087.readUInt32LE(i * 4));
      b87Offsets.push(orig087.length);
      const b87_0 = Buffer.from(orig087.subarray(b87Offsets[0], b87Offsets[1]));
      const b87_1 = filterOrigSecondaryImages(orig087.subarray(b87Offsets[1], b87Offsets[2]));

      const manifestMeshes = {};
      const manifestMaterials = {};
      const imported = [];

      for (const spec of R4_TRACKS) {
        if (signal?.aborted) throw Error('Operation canceled');
        if (service.busy) {
          service.busy.label = `Importing R4 Track ${spec.slot}/8: ${spec.name}`;
          await service.update();
        }
        const texIdx = spec.course * 2;
        const trkIdx = spec.course * 2 + 1;
        const [texEntryBuf, trackEntryBuf] = await Promise.all([
          reader.readSectors(offsets[texIdx], offsets[texIdx + 1] - offsets[texIdx]),
          reader.readSectors(offsets[trkIdx], offsets[trkIdx + 1] - offsets[trkIdx]),
        ]);

        const res = convertSingleR4Track({
          courseSpec: spec,
          texEntryBuf,
          trackEntryBuf,
          b87_0,
          b87_1,
          origPack088,
          origScene088,
        });

        const texRel = `raw/asset_${String(spec.texAsset).padStart(3, '0')}.bin`;
        const trkRel = `raw/asset_${String(spec.trackAsset).padStart(3, '0')}.bin`;
        const txtRel = `raw/course_${String(spec.slot).padStart(2, '0')}.txt`;
        const terrainRel = `meshes/r4_${spec.slot}_terrain.rmesh`;
        const courseRel = `meshes/r4_${spec.slot}_course.rmesh`;

        await Promise.all([
          fs.writeFile(path.join(staging, texRel), res.outTexPack),
          fs.writeFile(path.join(staging, trkRel), res.outTrackPack),
          fs.writeFile(path.join(staging, txtRel), `${spec.name}\n${spec.lengthText}\n`, 'utf8'),
          fs.writeFile(path.join(staging, terrainRel), res.terrainRmeshBuf),
          fs.writeFile(path.join(staging, courseRel), res.courseRmeshBuf),
        ]);

        // Validate converted track pack with rage-mod-cli
        await run(service.tool('rage-mod-cli'), ['--track-pack', path.join(staging, trkRel)], { signal });

        const prefix = `track.custom${spec.slot}`;
        manifestMeshes[`${prefix}.terrain`] = terrainRel;
        manifestMeshes[`${prefix}.course`] = courseRel;
        for (let i = 0; i < SKY_CLUTS.length; i++) {
          manifestMaterials[`${prefix}.course.material.${i}`] = 'unlit mask 1 0 1 1 1 1 0 0 0';
        }

        imported.push({
          slot: spec.slot,
          bank: spec.trackAsset,
          name: spec.name,
          lengthText: spec.lengthText,
          roadPoints: res.roadPoints,
          quadCount: res.quadCount,
          cellCount: res.cellCount,
        });
      }

      const metadataJson = JSON.stringify({
        format: 1,
        name: 'Ridge Racer Type 4 — All 8 Courses',
        region: service.state.disc.region,
        packageId: 'r4-tracks',
        author: 'Namco (R4 Import)',
        version: '1.0.0',
        description: 'All 8 courses imported from Ridge Racer Type 4 (Helter Skelter, Out of Blue, Wonderhill, Heaven and Hell, Edge of the Earth, Brightest Nite, Phantomile, Shooting Hoops).',
      }, null, 2) + '\n';
      await fs.writeFile(path.join(staging, 'rage-mod.json'), metadataJson, 'utf8');

      await run(service.tool('rage-mod-cli'), ['--write-profile-stdin', path.join(staging, 'mod.toml')], {
        signal,
        input: Buffer.from(JSON.stringify({
          textures: {},
          materials: manifestMaterials,
          meshes: manifestMeshes,
          audio: {},
        })),
      });

      // Remove any previous r4-tracks mod (and any stale missing package references) before installing the fresh snapshot
      const existingMods = service.state.mods.filter(m => m.packageId === 'r4-tracks');
      const removeIds = new Set(existingMods.map(m => m.id));
      const validIds = new Set(service.state.mods.filter(m => !removeIds.has(m.id)).map(m => m.id));
      const curSel = structuredClone(service.editor.project.snapshot().selection);
      curSel.packages = (curSel.packages || []).filter(id => validIds.has(id));
      for (const k of Object.keys(curSel.resolutions || {})) {
        if (!validIds.has(curSel.resolutions[k]?.winner) ||
            curSel.resolutions[k]?.candidates?.some(id => !validIds.has(id))) {
          delete curSel.resolutions[k];
        }
      }
      await service.changeModSelection(curSel, service.editor.project.revision);
      if (existingMods.length) {
        service.state.mods = service.state.mods.filter(m => !removeIds.has(m.id));
        for (const m of existingMods) {
          await fs.rm(path.join(service.root, 'mods', m.id), { recursive: true, force: true });
        }
        await service.persist();
      }

      const installedMod = await service.installModFiles(staging, signal, 'Ridge Racer Type 4 — All 8 Courses');
      const nextSel = structuredClone(service.editor.project.snapshot().selection);
      if (!nextSel.packages.includes(installedMod.id)) {
        nextSel.packages.push(installedMod.id);
      }
      await service.changeModSelection(nextSel, service.editor.project.revision);
      await service.editor.project.save(service.editor.file);
      const projectSnapshot = await require('./project-build.cjs').apply(service);
      return {
        modId: installedMod.id,
        firstBank: 136,
        imported,
        project: projectSnapshot,
      };
    } finally {
      await reader.close().catch(() => {});
      await fs.rm(staging, { recursive: true, force: true });
    }
  });
}

module.exports = { R4_TRACKS, importR4Tracks };
