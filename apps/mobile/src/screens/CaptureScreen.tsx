import React, { useCallback, useEffect, useMemo, useReducer, useRef, useState } from 'react';
import type { GestureResponderEvent } from 'react-native';
import {
  ActivityIndicator,
  Pressable,
  StyleSheet,
  Text,
  TextInput,
  View,
  useWindowDimensions,
} from 'react-native';
import { ArBackground } from '../components/ArBackground';
import { Overlay } from '../components/Overlay';
import type { HeightMode, Vec3 } from '../geometry/types';
import { isExpoGo } from '../native/env';
import type { FrameEvent } from '../native/VolumeAR';
import { getVolumeAR } from '../native/VolumeAR';
import { getVolumeCore } from '../native/VolumeCore';
import { initialState, reducer } from '../state/measurement';
import { colors, radius } from '../theme';
import { ResultSheet } from './ResultSheet';

const UP: Vec3 = { x: 0, y: 1, z: 0 };

const MODES: Array<{ key: HeightMode; label: string }> = [
  { key: 'manual', label: 'Type it' },
  { key: 'taps', label: 'Tap base + top' },
  { key: 'roof', label: 'Tap roof' },
];

export default function CaptureScreen() {
  const ar = useMemo(getVolumeAR, []);
  const core = useMemo(getVolumeCore, []);
  const [state, dispatch] = useReducer(reducer, initialState);
  const [frame, setFrame] = useState<FrameEvent | null>(null);
  const [busy, setBusy] = useState(false);
  const tapLock = useRef(false);
  const { width, height } = useWindowDimensions();

  useEffect(() => {
    let mounted = true;
    ar.start().catch(error => {
      if (mounted) {
        dispatch({
          type: 'message',
          text: `Camera unavailable: ${String((error as Error)?.message ?? error)}`,
        });
      }
    });
    const offFrame = ar.onFrame(event => {
      if (mounted) setFrame(event);
    });
    const offCorner = ar.onCornerUpdate(event => {
      if (mounted) dispatch({ type: 'cornerRefine', id: event.id, point: event.point });
    });
    return () => {
      mounted = false;
      offFrame();
      offCorner();
      ar.stop().catch(() => undefined);
    };
  }, [ar]);

  const onTap = useCallback(
    async (event: GestureResponderEvent) => {
      if (tapLock.current || busy) return;
      if (state.phase === 'result') return;
      if (state.phase === 'height' && state.heightMode === 'manual') return;

      tapLock.current = true;
      const { locationX, locationY } = event.nativeEvent;
      try {
        if (state.phase === 'capturing' && !state.closed) {
          const result = await ar.addCorner(locationX, locationY);
          dispatch({
            type: 'cornerAdd',
            corner: {
              id: result.id,
              point: { x: result.x, y: result.y, z: result.z },
              confidence: result.confidence,
            },
          });
        } else if (state.phase === 'height') {
          const result = await ar.pickPoint(locationX, locationY);
          const point: Vec3 = { x: result.x, y: result.y, z: result.z };
          if (state.heightMode === 'taps') {
            dispatch({ type: 'heightTap', point });
          } else if (state.heightMode === 'roof') {
            dispatch({
              type: 'roofAdd',
              corner: { id: result.id, point, confidence: result.confidence },
            });
          }
        }
      } finally {
        tapLock.current = false;
      }
    },
    [ar, busy, state.closed, state.heightMode, state.phase],
  );

  const closeLoop = useCallback(() => dispatch({ type: 'closeLoop' }), []);
  const backToCapture = useCallback(() => dispatch({ type: 'backToCapture' }), []);
  const reset = useCallback(() => {
    ar.reset();
    dispatch({ type: 'reset' });
  }, [ar]);

  const compute = useCallback(() => {
    setBusy(true);
    try {
      const result = core.compute(
        state.corners.map(c => c.point),
        state.heightMode,
        state.manualHeight,
        state.heightBase,
        state.heightTop,
        state.roofPoints.map(c => c.point),
        UP,
      );
      if (result.status !== 'Ok') {
        dispatch({ type: 'message', text: `Cannot compute yet: ${result.status}` });
        return;
      }
      dispatch({ type: 'measured', measurement: result });
    } finally {
      setBusy(false);
    }
  }, [core, state]);

  const canCompute =
    (state.heightMode === 'manual' && state.manualHeight > 0) ||
    (state.heightMode === 'taps' && state.heightBase !== null && state.heightTop !== null) ||
    (state.heightMode === 'roof' && state.roofPoints.length >= 3);

  const tracking = frame?.tracking ?? 'starting';
  const viewWidth = frame?.width && frame.width > 0 ? frame.width : width;
  const viewHeight = frame?.height && frame.height > 0 ? frame.height : height;

  const instruction = (() => {
    if (state.phase === 'capturing') {
      if (state.corners.length === 0) return 'Point at a building corner and tap to pin it.';
      return 'Walk to the next corner, point, and tap. Lines stay anchored in 3D.';
    }
    if (state.phase === 'height') {
      if (state.heightMode === 'manual') return 'Enter the building height, then compute.';
      if (state.heightMode === 'taps') {
        if (!state.heightBase) return 'Tap the base of a wall.';
        if (!state.heightTop) return 'Now tap the top of the same wall.';
        return 'Height captured. You can re-tap to change it.';
      }
      return state.roofPoints.length < 3
        ? `Tap roof corners in footprint order (${state.roofPoints.length}/3 minimum).`
        : 'Roof loop captured. Compute when ready.';
    }
    return 'Measurement complete.';
  })();

  return (
    <View style={styles.root}>
      <ArBackground active={state.phase !== 'result'} hasNative={ar.isNative} />

      <Pressable style={StyleSheet.absoluteFill} onPress={onTap} />

      <Overlay
        corners={state.corners}
        roofPoints={state.roofPoints}
        frame={frame}
        width={viewWidth}
        height={viewHeight}
        closed={state.closed}
      />

      <View style={styles.topBar} pointerEvents="box-none">
        <View>
          <Text style={styles.title}>Volume Scanner</Text>
          <Text style={styles.subtitle}>
            {state.corners.length} corner{state.corners.length === 1 ? '' : 's'}
            {ar.isNative ? '' : ' · demo data'}
          </Text>
        </View>
        <View style={styles.topRight}>
          <View style={styles.chip}>
            <View style={[styles.dot, tracking === 'normal' ? styles.dotOk : styles.dotWarn]} />
            <Text style={styles.chipText}>{tracking}</Text>
          </View>
          <Pressable style={styles.ghost} onPress={reset}>
            <Text style={styles.ghostText}>Reset</Text>
          </Pressable>
        </View>
      </View>

      {!ar.isNative ? (
        <View style={styles.notice} pointerEvents="none">
          <Text style={styles.noticeTitle}>
            {isExpoGo ? 'Expo Go has no AR camera' : 'AR module not loaded'}
          </Text>
          <Text style={styles.noticeText}>
            {isExpoGo
              ? 'Create a development build to measure with the live camera. Demo data is shown for now.'
              : 'Rebuild the development app so the VolumeAR native module is included.'}
          </Text>
        </View>
      ) : null}

      {state.phase === 'capturing' ? (
        <View style={styles.sheet}>
          <Text style={styles.instruction}>{instruction}</Text>
          {state.message ? <Text style={styles.error}>{state.message}</Text> : null}
          <View style={styles.row}>
            <Pressable
              style={[styles.primary, state.corners.length < 3 && styles.disabled]}
              disabled={state.corners.length < 3}
              onPress={closeLoop}
            >
              <Text style={styles.primaryText}>Close loop</Text>
            </Pressable>
            <Pressable style={styles.secondary} onPress={reset}>
              <Text style={styles.secondaryText}>Start over</Text>
            </Pressable>
          </View>
        </View>
      ) : null}

      {state.phase === 'height' ? (
        <View style={styles.sheet}>
          <View style={styles.segment}>
            {MODES.map(mode => {
              const active = state.heightMode === mode.key;
              return (
                <Pressable
                  key={mode.key}
                  style={[styles.segmentItem, active && styles.segmentItemActive]}
                  onPress={() => dispatch({ type: 'setHeightMode', mode: mode.key })}
                >
                  <Text style={[styles.segmentText, active && styles.segmentTextActive]}>
                    {mode.label}
                  </Text>
                </Pressable>
              );
            })}
          </View>

          <Text style={styles.instruction}>{instruction}</Text>
          {state.message ? <Text style={styles.error}>{state.message}</Text> : null}

          {state.heightMode === 'manual' ? (
            <View style={styles.inputRow}>
              <TextInput
                style={styles.input}
                keyboardType="decimal-pad"
                value={String(state.manualHeight)}
                onChangeText={text => {
                  const parsed = Number.parseFloat(text.replace(',', '.'));
                  dispatch({
                    type: 'setManualHeight',
                    value: Number.isFinite(parsed) ? parsed : 0,
                  });
                }}
              />
              <Text style={styles.inputUnit}>metres</Text>
            </View>
          ) : null}

          <View style={styles.row}>
            <Pressable
              style={[styles.primary, (!canCompute || busy) && styles.disabled]}
              disabled={!canCompute || busy}
              onPress={compute}
            >
              {busy ? (
                <ActivityIndicator color="#07110D" />
              ) : (
                <Text style={styles.primaryText}>Compute volume</Text>
              )}
            </Pressable>
            <Pressable style={styles.secondary} onPress={backToCapture}>
              <Text style={styles.secondaryText}>Back</Text>
            </Pressable>
          </View>
        </View>
      ) : null}

      {state.phase === 'result' && state.measurement ? (
        <View style={styles.sheet}>
          <ResultSheet measurement={state.measurement} onNewScan={reset} />
        </View>
      ) : null}
    </View>
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: colors.bg },
  topBar: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    paddingTop: 58,
    paddingHorizontal: 20,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
  },
  title: { color: colors.text, fontSize: 20, fontWeight: '800', letterSpacing: -0.3 },
  subtitle: { color: colors.muted, fontSize: 12, marginTop: 3 },
  notice: {
    position: 'absolute',
    top: 118,
    left: 14,
    right: 14,
    backgroundColor: colors.panel,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: 'rgba(255, 176, 32, 0.45)',
    paddingHorizontal: 14,
    paddingVertical: 11,
  },
  noticeTitle: { color: colors.warn, fontSize: 13, fontWeight: '700' },
  noticeText: { color: colors.muted, fontSize: 12, lineHeight: 17, marginTop: 3 },
  topRight: { alignItems: 'flex-end' },
  chip: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: colors.panel,
    borderRadius: radius.pill,
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderWidth: 1,
    borderColor: colors.border,
  },
  dot: { width: 8, height: 8, borderRadius: 4, marginRight: 6 },
  dotOk: { backgroundColor: colors.accent },
  dotWarn: { backgroundColor: colors.warn },
  chipText: { color: colors.muted, fontSize: 11, fontWeight: '600' },
  ghost: {
    marginTop: 8,
    paddingHorizontal: 12,
    paddingVertical: 7,
    borderRadius: radius.pill,
    borderWidth: 1,
    borderColor: colors.border,
    backgroundColor: colors.panel,
  },
  ghostText: { color: colors.text, fontSize: 12, fontWeight: '600' },
  sheet: {
    position: 'absolute',
    left: 14,
    right: 14,
    bottom: 22,
    backgroundColor: colors.panel,
    borderRadius: radius.lg,
    borderWidth: 1,
    borderColor: colors.border,
    padding: 16,
  },
  instruction: { color: colors.text, fontSize: 14, lineHeight: 20, marginBottom: 12 },
  error: { color: colors.danger, fontSize: 12, marginBottom: 10 },
  row: { flexDirection: 'row', alignItems: 'center' },
  primary: {
    flex: 1,
    backgroundColor: colors.accent,
    borderRadius: radius.md,
    paddingVertical: 15,
    alignItems: 'center',
  },
  primaryText: { color: '#07110D', fontSize: 15, fontWeight: '700' },
  disabled: { opacity: 0.4 },
  secondary: {
    marginLeft: 10,
    paddingVertical: 15,
    paddingHorizontal: 16,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
  },
  secondaryText: { color: colors.text, fontSize: 14, fontWeight: '600' },
  segment: {
    flexDirection: 'row',
    backgroundColor: 'rgba(255,255,255,0.05)',
    borderRadius: radius.md,
    padding: 4,
    marginBottom: 14,
  },
  segmentItem: {
    flex: 1,
    paddingVertical: 9,
    alignItems: 'center',
    borderRadius: radius.sm,
  },
  segmentItemActive: { backgroundColor: colors.accentDim },
  segmentText: { color: colors.muted, fontSize: 12, fontWeight: '600' },
  segmentTextActive: { color: colors.accent },
  inputRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 14 },
  input: {
    flex: 1,
    backgroundColor: 'rgba(255,255,255,0.06)',
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
    color: colors.text,
    fontSize: 20,
    fontWeight: '600',
    paddingHorizontal: 14,
    paddingVertical: 12,
  },
  inputUnit: { color: colors.muted, marginLeft: 12, fontSize: 14 },
});
