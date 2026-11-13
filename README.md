# AR Volume

Estimate the volume and dimensions of a building or structure by tapping its corners with a phone camera. Corners are pinned as metric 3D world points, connected into a footprint loop, then combined with a height (typed, tapped, or roof-tapped) to estimate volume.

Built with **Expo / React Native**, a shared **C++ geometry core**, and native **ARKit** (iOS) / **ARCore** (Android) modules.

> Status: core engine and JS app are implemented and tested. The native AR modules are written but still need a first on-device build. ARCore is not available on the current test phone (Samsung SM-A075F), so development is targeting the Android emulator.

---

## How it works

```
guided camera sweep
      │
      ├─ ARKit / ARCore world tracking ── metric camera pose + intrinsics
      │
      ├─ tap a corner ─────────────────── unproject pixel → world ray
      │                                  hit-test planes/depth → 3D point
      │
      ├─ walk, tap next corner ────────── 3D polyline, anchored in the world
      │
      ├─ close the loop ───────────────── polygon, planarity check, area
      │
      └─ height: type it / tap base+top / tap roof plane
                                         → volume + confidence
```

Points are world-space anchors, so the line stays in place when you look away and reappears when you point back.

### Height modes

| Mode | Input | Volume method |
|---|---|---|
| `manual` | typed height | footprint area × height |
| `taps` | tap wall base, then top | vertical extent × area |
| `roof` | tap roof corners | mean height of fitted roof plane × area |

Sloped roofs are supported both by the plane-bounded method and by the general polyhedron volume in the core.

---

## Repository layout

```
.
├── apps/mobile/                     Expo app
│   ├── src/                         UI, state machine, projection, TS math
│   ├── modules/volume-ar/           local Expo module
│   │   ├── ios/                     ARKit + ObjC++ core bridge
│   │   ├── android/                 ARCore + JNI core bridge + GL preview
│   │   └── core/                    shared C++17 geometry engine
│   ├── plugins/withArCore.js        injects ARCore manifest entries
│   ├── scripts/start-lan.ps1        LAN/tether host detection for Metro
│   └── eas.json                     development build profile
├── build/                           local CMake output (gitignored)
└── package.json                     workspace + core build/test scripts
```

The C++ core lives inside the module so `eas build` uploads everything it needs.

---

## Requirements

- Node.js 20+ (tested on 24)
- JDK 17+ (tested on 21)
- Android SDK with:
  - platform-tools / adb
  - `platforms;android-36`
  - `build-tools;36.0.0`
  - `ndk;27.1.12297006`
  - `cmake;3.22.1`
- An ARCore-capable Android device or an x86_64 emulator
- For iOS: macOS + Xcode, or EAS Build

---

## Install

```powershell
npm install
```

---

## Run

### 1. Demo mode (no native build)

Works in Expo Go. The AR module is absent, so the app shows a synthetic scene and uses demo points — useful for the UI and measurement flow.

```powershell
cd apps\mobile
npx expo start
```

The app displays a banner explaining that the camera requires a development build.

### 2. Development build (real camera + AR)

```powershell
cd apps\mobile
npx expo prebuild --clean --platform android
npm run android
```

First Gradle build takes a while. Subsequent JS-only changes only need a Metro reload.

iOS:

```powershell
npx eas-cli login
npx eas-cli build --profile development --platform ios
```

### Metro over a different network

If the PC and phone are on different interfaces (e.g. PC on Ethernet, phone on Wi-Fi or USB tether), force the advertised host:

```powershell
cd apps\mobile
npm run start:lan
```

The script prefers a USB-tether interface, then Wi-Fi, then the default route. Override with:

```powershell
powershell -File scripts\start-lan.ps1 -HostIp 192.168.1.50
```

Fallback when LAN is blocked:

```powershell
npm run start:tunnel
```

---

## ARCore on the Android emulator

Google supports ARCore on x86/x86_64 AVDs with the VirtualScene camera.

1. Use an x86_64 AVD (e.g. `Pixel_10a`), API 27+.
2. Ensure `hw.camera.back=virtualscene` in the AVD's `config.ini`.
3. Install Google Play Services for AR for the emulator:

```powershell
# download the x86 emulator APK from
# https://github.com/google-ar/arcore-android-sdk/releases
adb -s emulator-5554 install -r Google_Play_Services_for_AR_1.57.0_x86_for_emulator.apk
```

4. Build and run on the emulator:

```powershell
cd apps\mobile
$env:ANDROID_SERIAL="emulator-5554"
$env:ORG_GRADLE_PROJECT_reactNativeArchitectures="x86_64"   # faster
npx expo run:android --device emulator-5554
```

Virtual camera controls: hold **Shift** and use `W/A/S/D`; Shift-drag the mouse to look around.

---

## Core engine

The C++17 engine has no platform dependencies.

```powershell
# configure + build (Windows: run from a VS developer shell)
npm run core:build

# run tests
npm run core:test
```

It contains:

- multi-ray triangulation (`triangulate`) with parallax/residual confidence
- polygon area, centroid, planarity
- plane fitting (Jacobi eigen decomposition)
- extrusion, plane-bounded, and general polyhedron volume
- height from taps
- a stable C ABI (`volume_c.h`) consumed by Swift, Kotlin/JNI, and the JS fallback

---

## Type checking and bundling

```powershell
npm run typecheck
cd apps\mobile
npx expo export --platform android
```

---

## Measurement limits

- Footprint corners should be tapped from two or more viewpoints for best depth; a single ray against the sky has no depth.
- Long walks accumulate VIO drift; current code relies on ARKit/ARCore tracking and plane hit-tests.
- Typical accuracy on simple, boxy structures: roughly ±3–8% volume when corners are well observed; worse at long range or with partial occlusion.
- Volume is an estimate, not a survey-grade measurement.

---

## Device support

- Android requires an **ARCore-certified** device. The app checks at runtime and reports unsupported hardware instead of failing silently.
- The Samsung Galaxy SM-A075F used during development is **not** ARCore-certified (`android.hardware.camera.ar` absent), so AR runs on the emulator or another supported phone.
- iOS requires a device supported by ARKit.

---

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `adb: device offline` | Restart adb (`adb kill-server`, `adb start-server`), re-authorize USB debugging |
| Phone not in `adb devices` | Enable Developer Options → USB debugging; choose File Transfer mode; accept the RSA prompt |
| `Computation failed: no dev build` | Install a development build; Expo Go cannot load `VolumeAR` |
| "Expo Go has no AR camera" banner | Expected in Expo Go; build a dev client |
| Metro cannot be reached | Use `npm run start:lan`, check Windows Firewall for port 8081, or use `start:tunnel` |
| `ARCore is not supported on this device` | Not ARCore-certified; use an emulator or a supported device |
| CMake/NDK not found | Install `ndk;27.1.12297006` and `cmake;3.22.1` via SDK Manager |
| Metro watches CMake output | Core build output goes to `build/` at the repo root; `metro.config.js` also blocks it |

---

## Roadmap

- First on-device build of the native modules
- Tracked-corner refinement: re-observe a tapped corner across frames and triangulate to reduce error
- Loop closure / drift correction for large structures
- Export measurements (CSV/PDF) and scan history
- Non-AR fallback using a user-supplied reference length
