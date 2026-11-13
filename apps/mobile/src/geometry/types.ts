export type Vec3 = { x: number; y: number; z: number };

export type Measurement = {
  area: number;
  perimeter: number;
  height: number;
  volume: number;
  relativeUncertainty: number;
  planar: boolean;
  status: string;
};

export type Corner = {
  id: string;
  point: Vec3;
  confidence: number;
};

export type HeightMode = 'manual' | 'taps' | 'roof';

export type Phase = 'capturing' | 'height' | 'result';
