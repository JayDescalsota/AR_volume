import React from 'react';
import { StyleSheet } from 'react-native';
import Svg, {
  Circle,
  Line,
  Polygon,
  Polyline,
  Text as SvgText,
} from 'react-native-svg';
import { project } from '../geometry/mat4';
import type { Corner } from '../geometry/types';
import type { FrameEvent } from '../native/VolumeAR';
import { colors } from '../theme';

type Props = {
  corners: Corner[];
  roofPoints: Corner[];
  frame: FrameEvent | null;
  width: number;
  height: number;
  closed: boolean;
};

export function Overlay({ corners, roofPoints, frame, width, height, closed }: Props) {
  if (!frame || frame.viewProjection.length !== 16) return null;

  const matrix = frame.viewProjection;
  const screen = (points: Corner[]) =>
    points
      .map(corner => {
        const p = project(matrix, corner.point, width, height);
        return p.visible ? `${p.x},${p.y}` : null;
      })
      .filter((value): value is string => value !== null);

  const footprintPoints = screen(corners);
  const roofScreen = screen(roofPoints);

  const vertices = corners
    .map(corner => ({ corner, p: project(matrix, corner.point, width, height) }))
    .filter(v => v.p.visible);

  const first = footprintPoints[0];
  const last = footprintPoints[footprintPoints.length - 1];

  return (
    <Svg style={StyleSheet.absoluteFill} pointerEvents="none">
      {closed && footprintPoints.length >= 3 ? (
        <Polygon
          points={footprintPoints.join(' ')}
          fill="rgba(76, 224, 179, 0.14)"
          stroke={colors.accent}
          strokeWidth={2}
        />
      ) : footprintPoints.length >= 2 ? (
        <Polyline
          points={footprintPoints.join(' ')}
          fill="none"
          stroke={colors.accent}
          strokeWidth={2.5}
          strokeLinejoin="round"
        />
      ) : null}

      {!closed && first && last && footprintPoints.length >= 2 ? (
        <Line
          x1={last.split(',')[0]}
          y1={last.split(',')[1]}
          x2={first.split(',')[0]}
          y2={first.split(',')[1]}
          stroke={colors.accent}
          strokeWidth={1.5}
          strokeDasharray="6,6"
          opacity={0.6}
        />
      ) : null}

      {roofScreen.length >= 2 ? (
        <Polyline
          points={roofScreen.join(' ')}
          fill="none"
          stroke={colors.warn}
          strokeWidth={2}
          strokeDasharray="7,5"
        />
      ) : null}

      {vertices.map(({ corner, p }, index) => (
        <React.Fragment key={corner.id}>
          <Circle
            cx={p.x}
            cy={p.y}
            r={9}
            fill={colors.bg}
            stroke={colors.accent}
            strokeWidth={2.5}
          />
          <Circle
            cx={p.x}
            cy={p.y}
            r={3}
            fill={corner.confidence > 0.5 ? colors.accent : colors.warn}
          />
          <SvgText
            x={p.x + 14}
            y={p.y - 12}
            fill={colors.text}
            fontSize={12}
            fontWeight="600"
          >
            {index + 1}
          </SvgText>
        </React.Fragment>
      ))}

      {roofPoints.map((corner, index) => {
        const p = project(matrix, corner.point, width, height);
        if (!p.visible) return null;
        return (
          <React.Fragment key={corner.id}>
            <Circle cx={p.x} cy={p.y} r={7} fill={colors.bg} stroke={colors.warn} strokeWidth={2} />
            <SvgText x={p.x + 11} y={p.y - 9} fill={colors.warn} fontSize={11}>
              R{index + 1}
            </SvgText>
          </React.Fragment>
        );
      })}
    </Svg>
  );
}
