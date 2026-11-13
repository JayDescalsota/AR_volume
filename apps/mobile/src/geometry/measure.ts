import { cross, dot, length, normalize, sub } from './mat4';
import type { Measurement, Vec3 } from './types';

export function toFlat(points: ReadonlyArray<Vec3>): number[] {
  const out: number[] = [];
  for (const p of points) out.push(p.x, p.y, p.z);
  return out;
}

export function fromFlat(values: ReadonlyArray<number>): Vec3[] {
  const out: Vec3[] = [];
  for (let i = 0; i + 2 < values.length; i += 3) {
    out.push({
      x: values[i] ?? 0,
      y: values[i + 1] ?? 0,
      z: values[i + 2] ?? 0,
    });
  }
  return out;
}

export function areaVector(points: ReadonlyArray<Vec3>): Vec3 {
  let x = 0;
  let y = 0;
  let z = 0;
  for (let i = 0; i < points.length; i++) {
    const a = points[i];
    const b = points[(i + 1) % points.length];
    if (!a || !b) continue;
    const c = cross(a, b);
    x += c.x;
    y += c.y;
    z += c.z;
  }
  return { x: x * 0.5, y: y * 0.5, z: z * 0.5 };
}

export function polygonArea(points: ReadonlyArray<Vec3>): number {
  return points.length < 3 ? 0 : length(areaVector(points));
}

export function centroid(points: ReadonlyArray<Vec3>): Vec3 {
  if (points.length === 0) return { x: 0, y: 0, z: 0 };
  let x = 0;
  let y = 0;
  let z = 0;
  for (const p of points) {
    x += p.x;
    y += p.y;
    z += p.z;
  }
  const n = points.length;
  return { x: x / n, y: y / n, z: z / n };
}

export function perimeter(points: ReadonlyArray<Vec3>, closed: boolean): number {
  if (points.length < 2) return 0;
  let total = 0;
  for (let i = 0; i + 1 < points.length; i++) {
    const a = points[i];
    const b = points[i + 1];
    if (a && b) total += length(sub(a, b));
  }
  if (closed && points.length > 2) {
    const first = points[0];
    const last = points[points.length - 1];
    if (first && last) total += length(sub(first, last));
  }
  return total;
}

export type Plane = { normal: Vec3; d: number };

export function planeFromLoop(points: ReadonlyArray<Vec3>): Plane {
  const n = normalize(areaVector(points));
  if (length(n) < 0.5) return { normal: { x: 0, y: 1, z: 0 }, d: 0 };
  const c = centroid(points);
  return { normal: n, d: -dot(n, c) };
}

function signedDistance(plane: Plane, p: Vec3): number {
  return dot(plane.normal, p) + plane.d;
}

function projectToPlane(plane: Plane, p: Vec3): Vec3 {
  const dist = signedDistance(plane, p);
  return {
    x: p.x - plane.normal.x * dist,
    y: p.y - plane.normal.y * dist,
    z: p.z - plane.normal.z * dist,
  };
}

export type ComputeInput = {
  footprint: ReadonlyArray<Vec3>;
  mode: 'manual' | 'taps' | 'roof';
  manualHeight: number;
  base: Vec3 | null;
  top: Vec3 | null;
  roof: ReadonlyArray<Vec3>;
  up: Vec3;
};

export function computeMeasurement(input: ComputeInput): Measurement {
  const empty: Measurement = {
    area: 0,
    perimeter: 0,
    height: 0,
    volume: 0,
    relativeUncertainty: 0,
    planar: false,
    status: 'DegeneratePolygon',
  };

  if (input.footprint.length < 3) return empty;

  const area = polygonArea(input.footprint);
  if (!(area > 1e-6)) return empty;

  const perimeterValue = perimeter(input.footprint, true);
  const up = normalize(input.up);
  const ground = planeFromLoop(input.footprint);

  let height = 0;
  let status = 'Ok';

  if (input.mode === 'manual') {
    height = input.manualHeight;
  } else if (input.mode === 'taps') {
    if (!input.base || !input.top) {
      status = 'DegenerateHeight';
    } else {
      height = Math.abs(dot(sub(input.top, input.base), up));
    }
  } else {
    if (input.roof.length < 3) {
      status = 'DegeneratePlane';
    } else {
      const roofPlane = planeFromLoop(input.roof);
      const c = centroid(input.footprint);
      const onGround = projectToPlane(ground, c);
      const denom = dot(roofPlane.normal, up);
      if (Math.abs(denom) < 1e-6) {
        status = 'DegeneratePlane';
      } else {
        height = -(dot(roofPlane.normal, onGround) + roofPlane.d) / denom;
      }
    }
  }

  if (!(height > 1e-6)) {
    return { ...empty, area, perimeter: perimeterValue, status: status === 'Ok' ? 'DegenerateHeight' : status };
  }

  const rms = planarityRms(input.footprint, ground);
  return {
    area,
    perimeter: perimeterValue,
    height,
    volume: area * height,
    relativeUncertainty: 0,
    planar: rms <= 0.1,
    status: 'Ok',
  };
}

function planarityRms(points: ReadonlyArray<Vec3>, plane: Plane): number {
  if (points.length === 0) return 0;
  let sum = 0;
  for (const p of points) {
    const d = signedDistance(plane, p);
    sum += d * d;
  }
  return Math.sqrt(sum / points.length);
}
