import React from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';
import type { Measurement } from '../geometry/types';
import { colors, radius } from '../theme';

type Props = {
  measurement: Measurement;
  onNewScan: () => void;
};

function format(value: number, digits = 2): string {
  return value.toLocaleString(undefined, {
    minimumFractionDigits: digits,
    maximumFractionDigits: digits,
  });
}

function Stat({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.stat}>
      <Text style={styles.statLabel}>{label}</Text>
      <Text style={styles.statValue}>{value}</Text>
    </View>
  );
}

export function ResultSheet({ measurement, onNewScan }: Props) {
  const uncertainty = Math.round(measurement.relativeUncertainty * 100);
  return (
    <View style={styles.card}>
      <Text style={styles.eyebrow}>ESTIMATED VOLUME</Text>
      <View style={styles.heroRow}>
        <Text style={styles.hero}>{format(measurement.volume, 1)}</Text>
        <Text style={styles.unit}>m³</Text>
      </View>

      <View style={styles.grid}>
        <Stat label="Footprint" value={`${format(measurement.area, 2)} m²`} />
        <Stat label="Perimeter" value={`${format(measurement.perimeter, 2)} m`} />
        <Stat label="Height" value={`${format(measurement.height, 2)} m`} />
        <Stat label="Planar" value={measurement.planar ? 'Yes' : 'No'} />
      </View>

      {uncertainty > 0 ? (
        <Text style={styles.note}>Estimated uncertainty ±{uncertainty}%</Text>
      ) : null}

      <Pressable style={styles.primary} onPress={onNewScan}>
        <Text style={styles.primaryText}>New scan</Text>
      </Pressable>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.panel,
    borderColor: colors.border,
    borderWidth: 1,
    borderRadius: radius.lg,
    padding: 20,
  },
  eyebrow: {
    color: colors.muted,
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 1.4,
  },
  heroRow: {
    flexDirection: 'row',
    alignItems: 'flex-end',
    marginTop: 4,
    marginBottom: 16,
  },
  hero: {
    color: colors.text,
    fontSize: 52,
    fontWeight: '800',
    letterSpacing: -1.5,
  },
  unit: {
    color: colors.muted,
    fontSize: 20,
    fontWeight: '600',
    marginBottom: 10,
    marginLeft: 6,
  },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    borderTopColor: colors.border,
    borderTopWidth: 1,
    paddingTop: 14,
  },
  stat: {
    width: '50%',
    marginBottom: 12,
  },
  statLabel: {
    color: colors.muted,
    fontSize: 12,
    marginBottom: 2,
  },
  statValue: {
    color: colors.text,
    fontSize: 18,
    fontWeight: '600',
  },
  note: {
    color: colors.warn,
    fontSize: 12,
    marginTop: 2,
    marginBottom: 14,
  },
  primary: {
    backgroundColor: colors.accent,
    borderRadius: radius.md,
    paddingVertical: 15,
    alignItems: 'center',
    marginTop: 6,
  },
  primaryText: {
    color: '#07110D',
    fontSize: 16,
    fontWeight: '700',
  },
});
