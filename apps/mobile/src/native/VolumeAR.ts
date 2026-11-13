import { EventEmitter } from 'expo-modules-core';
import { Dimensions, PermissionsAndroid, Platform } from 'react-native';
import { lookAt, multiply, perspective } from '../geometry/mat4';
import type { Vec3 } from '../geometry/types';
import type { NativeCorner, NativeVolumeModule } from './bridge';
import { getNativeVolume } from './bridge';

export type CornerResult = NativeCorner;

export type FrameEvent = {
  viewProjection: number[];
  tracking: string;
  width: number;
  height: number;
};

export type CornerEvent = {
  id: string;
  point: Vec3;
  confidence: number;
};

export interface VolumeAR {
  isNative: boolean;
  start(): Promise<void>;
  stop(): Promise<void>;
  reset(): void;
  addCorner(px: number, py: number): Promise<CornerResult>;
  pickPoint(px: number, py: number): Promise<CornerResult>;
  onFrame(listener: (event: FrameEvent) => void): () => void;
  onCornerUpdate(listener: (event: CornerEvent) => void): () => void;
}

async function requestCameraPermission(): Promise<void> {
  if (Platform.OS !== 'android') return;
  const result = await PermissionsAndroid.request(
    PermissionsAndroid.PERMISSIONS.CAMERA,
  );
  if (result !== PermissionsAndroid.RESULTS.GRANTED) {
    throw new Error('Camera permission was denied');
  }
}

type VolumeEvents = {
  onFrame: (event: Record<string, unknown>) => void;
  onCornerUpdate: (event: Record<string, unknown>) => void;
};

function createNative(native: NativeVolumeModule): VolumeAR {
  const emitter = new EventEmitter<VolumeEvents>(native as never);
  return {
    isNative: true,
    start: async () => {
      await requestCameraPermission();
      await native.start();
    },
    stop: () => native.stop(),
    reset: () => native.reset(),
    addCorner: (px, py) => native.addCorner(px, py),
    pickPoint: (px, py) => native.pickPoint(px, py),
    onFrame(listener) {
      const sub = emitter.addListener('onFrame', (event: Record<string, unknown>) => {
        const projection = (event.viewProjection ?? []) as number[];
        listener({
          viewProjection: Array.from(projection),
          tracking: String(event.tracking ?? 'normal'),
          width: Number(event.width ?? 0),
          height: Number(event.height ?? 0),
        });
      });
      return () => sub.remove();
    },
    onCornerUpdate(listener) {
      const sub = emitter.addListener('onCornerUpdate', (event: Record<string, unknown>) => {
        listener({
          id: String(event.id),
          point: { x: Number(event.x), y: Number(event.y), z: Number(event.z) },
          confidence: Number(event.confidence ?? 0),
        });
      });
      return () => sub.remove();
    },
  };
}

const EYE = { x: 0, y: 2.2, z: 1.5 };
const TARGET = { x: 2, y: 0, z: -12 };
const UP = { x: 0, y: 1, z: 0 };

const DEMO_CORNERS: Vec3[] = [
  { x: -3, y: 0, z: -8 },
  { x: 7, y: 0, z: -8 },
  { x: 7, y: 0, z: -16 },
  { x: -3, y: 0, z: -16 },
];

const DEMO_PICKS: Vec3[] = [
  { x: 2, y: 0, z: -10 },
  { x: 2, y: 4, z: -10 },
  { x: -3, y: 4.2, z: -8 },
  { x: 7, y: 4.2, z: -8 },
  { x: 7, y: 4.2, z: -16 },
  { x: -3, y: 4.2, z: -16 },
];

function createMock(): VolumeAR {
  let timer: ReturnType<typeof setInterval> | null = null;
  let cornerIndex = 0;
  let pickIndex = 0;
  const frameListeners = new Set<(event: FrameEvent) => void>();
  const cornerListeners = new Set<(event: CornerEvent) => void>();

  const emitFrame = () => {
    const { width, height } = Dimensions.get('window');
    const aspect = height > 0 ? width / height : 1;
    const viewProjection = multiply(
      perspective(Math.PI / 3, aspect, 0.1, 300),
      lookAt(EYE, TARGET, UP),
    );
    for (const listener of frameListeners) {
      listener({ viewProjection, tracking: 'normal', width, height });
    }
  };

  const ensureTimer = () => {
    if (timer) return;
    timer = setInterval(emitFrame, 100);
    emitFrame();
  };

  return {
    isNative: false,
    async start() {
      ensureTimer();
    },
    async stop() {
      if (timer) clearInterval(timer);
      timer = null;
    },
    reset() {
      cornerIndex = 0;
      pickIndex = 0;
    },
    async addCorner() {
      const point = DEMO_CORNERS[cornerIndex % DEMO_CORNERS.length] ?? DEMO_CORNERS[0]!;
      cornerIndex += 1;
      const id = `c${cornerIndex}`;
      const result: CornerResult = {
        id,
        x: point.x,
        y: point.y,
        z: point.z,
        confidence: Math.min(0.95, 0.55 + cornerIndex * 0.1),
        rayCount: 1 + cornerIndex,
      };
      for (const listener of cornerListeners) {
        listener({ id, point, confidence: result.confidence });
      }
      return result;
    },
    async pickPoint() {
      const point = DEMO_PICKS[pickIndex % DEMO_PICKS.length] ?? DEMO_PICKS[0]!;
      pickIndex += 1;
      return {
        id: `p${pickIndex}`,
        x: point.x,
        y: point.y,
        z: point.z,
        confidence: 0.85,
        rayCount: 3,
      };
    },
    onFrame(listener) {
      frameListeners.add(listener);
      ensureTimer();
      return () => frameListeners.delete(listener);
    },
    onCornerUpdate(listener) {
      cornerListeners.add(listener);
      return () => cornerListeners.delete(listener);
    },
  };
}

let instance: VolumeAR | null = null;

export function getVolumeAR(): VolumeAR {
  if (!instance) {
    const native = getNativeVolume();
    instance = native ? createNative(native) : createMock();
  }
  return instance;
}
