import type { Vec3 } from './types';

export type Mat4 = number[];

export function multiply(a: Mat4, b: Mat4): Mat4 {
  const out = new Array<number>(16).fill(0);
  for (let r = 0; r < 4; r++) {
    for (let c = 0; c < 4; c++) {
      let s = 0;
      for (let k = 0; k < 4; k++) {
        s += (a[r * 4 + k] ?? 0) * (b[k * 4 + c] ?? 0);
      }
      out[r * 4 + c] = s;
    }
  }
  return out;
}

export function perspective(
  fovYRad: number,
  aspect: number,
  near: number,
  far: number,
): Mat4 {
  const f = 1 / Math.tan(fovYRad / 2);
  return [
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (far + near) / (near - far), (2 * far * near) / (near - far),
    0, 0, -1, 0,
  ];
}

export function lookAt(eye: Vec3, target: Vec3, up: Vec3): Mat4 {
  const f = normalize(sub(target, eye));
  const r = normalize(cross(f, up));
  const u = cross(r, f);
  return [
    r.x, r.y, r.z, -dot(r, eye),
    u.x, u.y, u.z, -dot(u, eye),
    -f.x, -f.y, -f.z, dot(f, eye),
    0, 0, 0, 1,
  ];
}

export type Projected = { x: number; y: number; visible: boolean };

export function project(
  m: Mat4,
  p: Vec3,
  width: number,
  height: number,
): Projected {
  const cx = (m[0] ?? 0) * p.x + (m[1] ?? 0) * p.y + (m[2] ?? 0) * p.z + (m[3] ?? 0);
  const cy = (m[4] ?? 0) * p.x + (m[5] ?? 0) * p.y + (m[6] ?? 0) * p.z + (m[7] ?? 0);
  const cw = (m[12] ?? 0) * p.x + (m[13] ?? 0) * p.y + (m[14] ?? 0) * p.z + (m[15] ?? 0);
  if (!Number.isFinite(cw) || cw <= 1e-6) {
    return { x: 0, y: 0, visible: false };
  }
  const nx = cx / cw;
  const ny = cy / cw;
  return {
    x: (nx * 0.5 + 0.5) * width,
    y: (1 - (ny * 0.5 + 0.5)) * height,
    visible: Number.isFinite(nx) && Number.isFinite(ny),
  };
}

export function add(a: Vec3, b: Vec3): Vec3 {
  return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z };
}

export function sub(a: Vec3, b: Vec3): Vec3 {
  return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
}

export function scale(a: Vec3, s: number): Vec3 {
  return { x: a.x * s, y: a.y * s, z: a.z * s };
}

export function dot(a: Vec3, b: Vec3): number {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

export function cross(a: Vec3, b: Vec3): Vec3 {
  return {
    x: a.y * b.z - a.z * b.y,
    y: a.z * b.x - a.x * b.z,
    z: a.x * b.y - a.y * b.x,
  };
}

export function length(a: Vec3): number {
  return Math.sqrt(dot(a, a));
}

export function normalize(a: Vec3): Vec3 {
  const n = length(a);
  return n > 1e-9 ? scale(a, 1 / n) : { x: 0, y: 0, z: 0 };
}
