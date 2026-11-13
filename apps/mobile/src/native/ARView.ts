import { requireNativeViewManager } from 'expo-modules-core';
import type { ComponentType } from 'react';
import { getNativeVolume } from './bridge';

type ARViewProps = { style?: unknown; active?: boolean };

let cached: ComponentType<ARViewProps> | null | undefined;

export function getNativeARView(): ComponentType<ARViewProps> | null {
  if (cached !== undefined) return cached;

  if (!getNativeVolume()) {
    cached = null;
    return cached;
  }

  try {
    cached = requireNativeViewManager('VolumeAR') as ComponentType<ARViewProps>;
  } catch {
    cached = null;
  }
  return cached;
}
