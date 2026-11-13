import React from 'react';
import { StyleSheet, View } from 'react-native';
import Svg, { Line } from 'react-native-svg';
import { getNativeARView } from '../native/ARView';
import { colors } from '../theme';

type Props = { active: boolean; hasNative: boolean };

export function ArBackground({ active, hasNative }: Props) {
  const NativeARView = hasNative ? getNativeARView() : null;

  if (NativeARView) {
    return <NativeARView style={StyleSheet.absoluteFill} active={active} />;
  }

  const horizon = 360;
  return (
    <View style={[StyleSheet.absoluteFill, styles.scene]}>
      <Svg style={StyleSheet.absoluteFill}>
        <Line
          x1="0"
          y1={horizon}
          x2="100%"
          y2={horizon}
          stroke="rgba(255,255,255,0.08)"
          strokeWidth={1}
        />
        {Array.from({ length: 9 }).map((_, index) => {
          const t = (index + 1) / 10;
          const y = horizon + Math.pow(t, 1.8) * 420;
          return (
            <Line
              key={`h-${index}`}
              x1="0"
              y1={y}
              x2="100%"
              y2={y}
              stroke="rgba(255,255,255,0.05)"
              strokeWidth={1}
            />
          );
        })}
        {Array.from({ length: 11 }).map((_, index) => {
          const x = (index / 10) * 100;
          return (
            <Line
              key={`v-${index}`}
              x1={`${x}%`}
              y1={horizon}
              x2={`${(x - 50) * 3 + 50}%`}
              y2="100%"
              stroke="rgba(255,255,255,0.04)"
              strokeWidth={1}
            />
          );
        })}
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  scene: {
    backgroundColor: colors.bg,
  },
});
