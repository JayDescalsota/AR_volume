import { computeMeasurement, polygonArea, toFlat } from '../geometry/measure';
import type { HeightMode, Measurement, Vec3 } from '../geometry/types';
import type { NativeMeasurement } from './bridge';
import { getNativeVolume } from './bridge';

export interface VolumeCore {
  isNative: boolean;
  compute(
    footprint: ReadonlyArray<Vec3>,
    mode: HeightMode,
    manualHeight: number,
    base: Vec3 | null,
    top: Vec3 | null,
    roof: ReadonlyArray<Vec3>,
    up: Vec3,
  ): Measurement;
  area(points: ReadonlyArray<Vec3>): number;
}

function fromNative(result: NativeMeasurement): Measurement {
  return {
    area: result.area,
    perimeter: result.perimeter,
    height: result.height,
    volume: result.volume,
    relativeUncertainty: result.relativeUncertainty,
    planar: result.planar,
    status: result.status,
  };
}

export function getVolumeCore(): VolumeCore {
  const native = getNativeVolume();
  if (native) {
    return {
      isNative: true,
      compute(footprint, mode, manualHeight, base, top, roof, up) {
        return fromNative(
          native.computeMeasurement(
            toFlat(footprint),
            mode,
            manualHeight,
            base ? toFlat([base]) : [],
            top ? toFlat([top]) : [],
            toFlat(roof),
            toFlat([up]),
          ),
        );
      },
      area(points) {
        return native.polygonArea(toFlat(points));
      },
    };
  }

  return {
    isNative: false,
    compute(footprint, mode, manualHeight, base, top, roof, up) {
      return computeMeasurement({
        footprint,
        mode,
        manualHeight,
        base,
        top,
        roof,
        up,
      });
    },
    area(points) {
      return polygonArea(points);
    },
  };
}
