const { withAndroidManifest } = require('expo/config-plugins');

const META_DATA = 'com.google.ar.core';
const FEATURE = 'android.hardware.camera.ar';

module.exports = function withArCore(config) {
  return withAndroidManifest(config, current => {
    const manifest = current.modResults.manifest;

    manifest['uses-feature'] = manifest['uses-feature'] ?? [];
    if (!manifest['uses-feature'].some(item => item.$?.['android:name'] === FEATURE)) {
      manifest['uses-feature'].push({
        $: { 'android:name': FEATURE, 'android:required': 'false' },
      });
    }

    const application = manifest.application?.[0];
    if (application) {
      application['meta-data'] = application['meta-data'] ?? [];
      const existing = application['meta-data'].find(
        item => item.$?.['android:name'] === META_DATA,
      );
      if (existing) {
        existing.$['android:value'] = 'optional';
      } else {
        application['meta-data'].push({
          $: { 'android:name': META_DATA, 'android:value': 'optional' },
        });
      }
    }

    return current;
  });
};
