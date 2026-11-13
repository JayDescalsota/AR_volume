import { requireNativeModule } from 'expo-modules-core';

export type NativeCorner = {
  id: string;
  x: number;
  y: number;
  z: number;
  confidence: number;
  rayCount: number;
};

export type NativeMeasurement = {
  area: number;
  perimeter: number;
  height: number;
  volume: number;
  relativeUncertainty: number;
  planar: boolean;
  status: string;
};

export interface NativeVolumeModule {
  start(): Promise<void>;
  stop(): Promise<void>;
  reset(): void;
  addCorner(px: number, py: number): Promise<NativeCorner>;
  pickPoint(px: number, py: number): Promise<NativeCorner>;
  getViewProjection(): Promise<number[]>;
  computeMeasurement(
    footprint: number[],
    mode: string,
    manualHeight: number,
    base: number[],
    top: number[],
    roof: number[],
    up: number[],
  ): NativeMeasurement;
  polygonArea(points: number[]): number;
}

let cached: NativeVolumeModule | null | undefined;

export function getNativeVolume(): NativeVolumeModule | null {
  if (cached !== undefined) return cached;
  try {
    cached = requireNativeModule<NativeVolumeModule>('VolumeAR');
  } catch {
    cached = null;
  }
  return cached;
}
