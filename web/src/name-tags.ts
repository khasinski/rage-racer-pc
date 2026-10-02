// The meshes for nameplates.ts: one camera-facing quad per named car, drawn
// with the world so the depth test hides a plate behind nearer geometry.
import * as THREE from 'three';
import { NAMEPLATE_HEIGHT, type Nameplate } from './nameplates';
import { nameplateFragment, nameplateVertex } from './shaders';

/** The world camera's uniforms, shared with the renderer's materials. */
export interface CameraUniforms {
  uCameraPosition: { value: THREE.Vector4 };
  uViewRow0: { value: THREE.Vector4 };
  uViewRow1: { value: THREE.Vector4 };
  uViewRow2: { value: THREE.Vector4 };
  uProjection: { value: THREE.Vector4 };
}

interface NameTag {
  mesh: THREE.Mesh;
  material: THREE.RawShaderMaterial;
  texture: THREE.CanvasTexture | null;
  name: string;
}

export class NameTags {
  private readonly geometry = quad();
  private readonly tags = new Map<number, NameTag>();
  private readonly scene: THREE.Scene;
  private readonly camera: CameraUniforms;

  constructor(scene: THREE.Scene, camera: CameraUniforms) {
    this.scene = scene;
    this.camera = camera;
  }

  /** Shows these plates and hides every other. */
  set(plates: readonly Nameplate[]): void {
    const live = new Set<number>();
    for (const plate of plates) {
      live.add(plate.seat);
      let tag = this.tags.get(plate.seat);
      if (!tag) {
        tag = this.create();
        this.tags.set(plate.seat, tag);
      }
      if (tag.name !== plate.name || tag.texture === null) {
        tag.texture?.dispose();
        tag.texture = paintName(plate.name);
        tag.material.uniforms.uName.value = tag.texture;
        tag.name = plate.name;
      }
      tag.material.uniforms.uAnchor.value.set(plate.anchor[0], plate.anchor[1], plate.anchor[2]);
      const image = tag.texture.image as HTMLCanvasElement;
      const height = plate.length * NAMEPLATE_HEIGHT;
      tag.material.uniforms.uSize.value.set(height * (image.width / image.height), height);
      tag.material.uniforms.uAlpha.value = plate.alpha;
      tag.mesh.position.set(plate.anchor[0], plate.anchor[1], plate.anchor[2]);
      tag.mesh.visible = true;
    }
    for (const [seat, tag] of this.tags) if (!live.has(seat)) tag.mesh.visible = false;
  }

  private create(): NameTag {
    const material = new THREE.RawShaderMaterial({
      glslVersion: THREE.GLSL3,
      vertexShader: nameplateVertex,
      fragmentShader: nameplateFragment,
      uniforms: {
        ...this.camera,
        uAnchor: { value: new THREE.Vector3() },
        uSize: { value: new THREE.Vector2(1, 1) },
        uAlpha: { value: 1 },
        uName: { value: null },
      },
      transparent: true,
      depthTest: true,
      depthWrite: false,
      depthFunc: THREE.LessEqualDepth,
      side: THREE.DoubleSide,
      blending: THREE.NormalBlending,
    });
    const mesh = new THREE.Mesh(this.geometry, material);
    mesh.frustumCulled = false;
    mesh.renderOrder = 2;
    mesh.visible = false;
    this.scene.add(mesh);
    return { mesh, material, texture: null, name: '' };
  }
}

function quad(): THREE.BufferGeometry {
  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('corner', new THREE.Float32BufferAttribute([-1, 0, 1, 0, 1, 1, -1, 1], 2));
  geometry.setIndex([0, 1, 2, 0, 2, 3]);
  geometry.boundingSphere = new THREE.Sphere(new THREE.Vector3(), Infinity);
  return geometry;
}

function paintName(name: string): THREE.CanvasTexture {
  const canvas = document.createElement('canvas');
  const ctx = canvas.getContext('2d');
  if (!ctx) throw new Error('name canvas unavailable');
  const font = '700 32px "Helvetica Neue", Arial, sans-serif';
  ctx.font = font;
  canvas.width = Math.max(4, Math.ceil(ctx.measureText(name).width) + 24);
  canvas.height = 48;
  ctx.font = font;
  ctx.textBaseline = 'middle';
  ctx.lineJoin = 'round';
  ctx.miterLimit = 2;
  ctx.lineWidth = 6;
  ctx.strokeStyle = 'rgba(0, 0, 0, 0.9)';
  ctx.fillStyle = '#ffffff';
  ctx.strokeText(name, 12, 26);
  ctx.fillText(name, 12, 26);
  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.NoColorSpace;
  texture.generateMipmaps = false;
  texture.minFilter = THREE.LinearFilter;
  texture.magFilter = THREE.LinearFilter;
  texture.needsUpdate = true;
  return texture;
}
