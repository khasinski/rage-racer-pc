// Draws each frame the bridge built: one dynamic geometry holding this
// frame's world-space triangles, split into groups in the native span order,
// one material per decoded texture. Camera, projection and fog come from the
// bridge's uniform block, so three.js never computes its own view.
import * as THREE from 'three';
import {
  MATERIAL_ENV_CLUT, NO_MATERIAL, Rage, SPAN_FIELDS, TEXTURE_SIZE,
} from './rage';
import { skyFragment, skyVertex, worldFragment, worldVertex } from './shaders';

const VERTEX_CAPACITY = 600_000;
/* Decoding is the expensive part of a palette change; spread it out. */
const DECODES_PER_FRAME = 48;

type Uniform<T> = { value: T };

export class Renderer {
  readonly webgl: THREE.WebGLRenderer;
  private readonly scene = new THREE.Scene();
  private readonly camera = new THREE.Camera();
  private readonly geometry = new THREE.BufferGeometry();
  private readonly vertexData: Float32Array;
  private readonly vertexBuffer: THREE.InterleavedBuffer;
  private readonly materials: THREE.RawShaderMaterial[] = [];
  private readonly materialIndex = new Map<string, number>();
  private readonly untexturedIndex: number;
  private readonly shared = {
    uCameraPosition: { value: new THREE.Vector4() },
    uViewRow0: { value: new THREE.Vector4() },
    uViewRow1: { value: new THREE.Vector4() },
    uViewRow2: { value: new THREE.Vector4() },
    uProjection: { value: new THREE.Vector4() },
    uLightDirection: { value: new THREE.Vector4() },
    uAmbient: { value: new THREE.Vector4() },
    uDiffuse: { value: new THREE.Vector4() },
    uSkyTop: { value: new THREE.Vector4() },
    uSkyHorizon: { value: new THREE.Vector4() },
    uSkyBottom: { value: new THREE.Vector4() },
  };
  private paletteHash = 0;
  /* Materials whose image depends on the palette, re-decoded after it changes. */
  private readonly paletteMaterials = new Map<number, string>();
  private stale: number[] = [];

  constructor(canvas: HTMLCanvasElement, private readonly rage: Rage) {
    this.webgl = new THREE.WebGLRenderer({
      canvas, antialias: false, powerPreference: 'high-performance',
      // Only the automated check (scripts/e2e.mjs) reads pixels back.
      preserveDrawingBuffer: location.hash === '#e2e',
    });
    this.webgl.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    this.webgl.autoClear = true;
    this.webgl.outputColorSpace = THREE.LinearSRGBColorSpace;

    const floats = rage.packedFloats;
    this.vertexData = new Float32Array(VERTEX_CAPACITY * floats);
    this.vertexBuffer = new THREE.InterleavedBuffer(this.vertexData, floats);
    this.vertexBuffer.setUsage(THREE.DynamicDrawUsage);
    const attribute = (size: number, offset: number) =>
      new THREE.InterleavedBufferAttribute(this.vertexBuffer, size, offset);
    this.geometry.setAttribute('position', attribute(3, 0));
    this.geometry.setAttribute('uv', attribute(2, 3));
    this.geometry.setAttribute('color', attribute(4, 5));
    this.geometry.setAttribute('normal', attribute(3, 9));
    this.geometry.setAttribute('fog', attribute(4, 12));
    this.geometry.setAttribute('lighting', attribute(1, 16));
    this.geometry.setAttribute('environmentLight', attribute(3, 17));
    this.geometry.setAttribute('depthBias', attribute(1, 20));
    this.geometry.boundingSphere = new THREE.Sphere(new THREE.Vector3(), Infinity);

    this.untexturedIndex = this.addMaterial(null);
    const world = new THREE.Mesh(this.geometry, this.materials);
    world.frustumCulled = false;

    const sky = new THREE.Mesh(
      new THREE.BufferGeometry().setAttribute(
        'position', new THREE.Float32BufferAttribute([-1, -1, 0, 3, -1, 0, -1, 3, 0], 3)),
      new THREE.RawShaderMaterial({
        glslVersion: THREE.GLSL3, vertexShader: skyVertex, fragmentShader: skyFragment,
        uniforms: this.shared, depthTest: false, depthWrite: false,
      }));
    sky.frustumCulled = false;
    sky.renderOrder = -1;
    this.scene.add(sky, world);
  }

  private addMaterial(texture: THREE.Texture | null): number {
    const material = new THREE.RawShaderMaterial({
      glslVersion: THREE.GLSL3,
      vertexShader: worldVertex,
      fragmentShader: worldFragment,
      uniforms: {
        ...this.shared,
        uMaterial: { value: texture } as Uniform<THREE.Texture | null>,
        uTextured: { value: texture ? 1 : 0 },
      },
      side: THREE.DoubleSide,
      blending: THREE.NormalBlending,
      transparent: false,
      depthWrite: true,
    });
    this.materials.push(material);
    return this.materials.length - 1;
  }

  private static texture(rgba: Uint8Array): THREE.DataTexture {
    const texture = new THREE.DataTexture(rgba, TEXTURE_SIZE, TEXTURE_SIZE, THREE.RGBAFormat);
    texture.magFilter = THREE.NearestFilter;
    texture.minFilter = THREE.NearestMipmapLinearFilter;
    texture.generateMipmaps = true;
    texture.colorSpace = THREE.NoColorSpace;
    texture.needsUpdate = true;
    return texture;
  }

  private materialFor(spans: Uint32Array, span: number): number {
    const f = span * SPAN_FIELDS;
    const material = spans[f + 2];
    if (material === NO_MATERIAL) return this.untexturedIndex;
    const palette = (spans[f + 11] & MATERIAL_ENV_CLUT) !== 0;
    const key = `${spans[f + 3]}:${spans[f + 4]}:${spans[f + 5]}:${material}:${spans[f + 6]}:` +
                `${spans[f + 7]}:${spans[f + 8]}:${spans[f + 9]}`;
    let index = this.materialIndex.get(key);
    if (index === undefined) {
      const rgba = this.rage.decodeTexture(span);
      index = rgba ? this.addMaterial(Renderer.texture(rgba)) : this.untexturedIndex;
      this.materialIndex.set(key, index);
      if (rgba && palette) this.paletteMaterials.set(index, key);
    } else if (palette && this.stale.includes(index)) {
      this.refresh(index, span);
    }
    return index;
  }

  private refresh(index: number, span: number) {
    const rgba = this.rage.decodeTexture(span);
    const uniform = this.materials[index].uniforms.uMaterial as Uniform<THREE.Texture | null>;
    if (rgba) {
      uniform.value?.dispose();
      uniform.value = Renderer.texture(rgba);
    }
    this.stale = this.stale.filter((stale) => stale !== index);
  }

  /** Uploads the frame the bridge just built with `vertexCount` vertices. */
  update(vertexCount: number) {
    const hash = this.rage.paletteHash();
    if (hash !== this.paletteHash) {
      this.paletteHash = hash;
      this.stale = [...this.paletteMaterials.keys()];
    }
    const packed = this.rage.packedVertices(vertexCount);
    this.vertexData.set(packed);
    this.vertexBuffer.clearUpdateRanges();
    this.vertexBuffer.addUpdateRange(0, packed.length);
    this.vertexBuffer.needsUpdate = true;

    const spans = this.rage.spans();
    const spanCount = spans.length / SPAN_FIELDS;
    this.geometry.clearGroups();
    let decodes = 0;
    for (let span = 0; span < spanCount; span++) {
      const f = span * SPAN_FIELDS;
      const known = this.materialIndex.has(this.keyOf(spans, span));
      if (!known && decodes >= DECODES_PER_FRAME && spans[f + 2] !== NO_MATERIAL) continue;
      if (!known) decodes++;
      this.geometry.addGroup(spans[f], spans[f + 1], this.materialFor(spans, span));
    }
    this.geometry.setDrawRange(0, vertexCount);

    const c = this.rage.camera();
    const s = this.shared;
    s.uCameraPosition.value.fromArray(c, 0);
    s.uViewRow0.value.fromArray(c, 4);
    s.uViewRow1.value.fromArray(c, 8);
    s.uViewRow2.value.fromArray(c, 12);
    s.uProjection.value.fromArray(c, 16);
    const l = this.rage.light();
    s.uLightDirection.value.fromArray(l, 0);
    s.uAmbient.value.fromArray(l, 4);
    s.uDiffuse.value.fromArray(l, 8);
    s.uSkyTop.value.fromArray(l, 12);
    s.uSkyHorizon.value.fromArray(l, 16);
    s.uSkyBottom.value.fromArray(l, 20);
  }

  private keyOf(spans: Uint32Array, span: number): string {
    const f = span * SPAN_FIELDS;
    return `${spans[f + 3]}:${spans[f + 4]}:${spans[f + 5]}:${spans[f + 2]}:${spans[f + 6]}:` +
           `${spans[f + 7]}:${spans[f + 8]}:${spans[f + 9]}`;
  }

  resize(width: number, height: number) {
    this.webgl.setSize(width, height, false);
  }

  get aspect(): number {
    const canvas = this.webgl.domElement;
    return canvas.clientWidth / Math.max(1, canvas.clientHeight);
  }

  render() {
    this.webgl.render(this.scene, this.camera);
  }
}
