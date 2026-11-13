import React from 'react';
import { StatusBar, StyleSheet, View } from 'react-native';
import CaptureScreen from './screens/CaptureScreen';
import { colors } from './theme';

export default function App() {
  return (
    <View style={styles.root}>
      <StatusBar barStyle="light-content" />
      <CaptureScreen />
    </View>
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: colors.bg },
});
