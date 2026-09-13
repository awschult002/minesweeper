# How to build Minesweeper for Android

App id: `org.orx.minesweeper` · native lib: `Minesweeper` · minSdk 23 · NDK r27.3+ (app pin; Orx wiki historically cites NDK 23 — prefer the version in `app/build.gradle`)

## Orx learning path (do in order)

Follow Orx’s Android tutorials **in this order** — this is the contract for the Studio install:

1. **[Getting Android Studio and Orx](https://orx-project.org/wiki/en/tutorials/android/getting_android_tools_and_orx)**  
   - Install Android Studio (Standard Setup). Wiki used Electric Eel / SDK 33; current Studio is fine if SDK installs.  
   - Scripts pull NDK as needed (you shouldn’t hand-pick unless Gradle pins differ).  
   - Set env: `ANDROID_HOME` → SDK root, `JAVA_HOME` → Studio’s bundled JRE/JDK.  
   - Close Studio after first setup if you only need CLI builds.  
   - Orx already cloned here: `/workspace/repos/orx` (`setup.sh` done).
2. **[Compiling Orx + Android demo](https://orx-project.org/wiki/en/tutorials/android/setup_android)**  
   - `cd $ORX/build/android && ./build.sh` → AARs  
   - Optional sanity: `cd orx/code/demo/android && ./build.sh` → demo APKs  
3. **[Using the demo as a template](https://orx-project.org/wiki/en/tutorials/android/using_the_android_demo_as_a_template_for_your_own_projects)**  
   - Our project already adapted under `build/android/` — then steps below.

Also useful: https://orx-project.org/wiki/en/tutorials/android/setup_android · template article above.


## First device target

**Galaxy S22 · Android 16** · `targetSdk` 36 · `applicationId` `org.orx.minesweeper`

Install artifact (debug, signed for sideload):

`build/android/app/build/outputs/apk/debug/app-debug.apk`

### Sideload on the phone

1. Copy `app-debug.apk` to the S22 (USB, Drive, or DM from Chief).
2. On the phone: allow install from that source (Settings → security / “Install unknown apps” for Files/Chrome/etc.).
3. Open the APK → Install. Or with USB debugging: `adb install -r app-debug.apk`
4. Launch **Minesweeper**. After the next APK: start/difficulty → grid (portrait); HUD mode tap; chord on revealed N; pan clamped; no 2-finger flag — see STATUS.

## Prereqs

1. **Orx** at `/workspace/repos/orx` with `./setup.sh` already run; `ORX` → `/workspace/repos/orx/code`
2. **Android Studio** (or cmdline SDK) + **NDK 27.3.13750724** (pinned in `build/android/app/build.gradle`)
3. `ANDROID_HOME` → Android SDK root (Studio Standard Setup)
4. `JAVA_HOME` → Studio’s bundled JDK (or another JDK 11+ Gradle accepts)

```bash
export ORX=/workspace/repos/orx/code
```

## 1. Build Orx Android AARs

```bash
cd "$ORX/build/android" && ./build.sh
```

AARs land under `orx/code/build/android/orx/build/outputs/aar` and are published for Gradle via `$ORX/lib/static/android/repository/`.

## 2. Sync game data into Android assets

Android cannot follow `../` into the desktop `data/` tree. Copy (or rsync) before each APK build:

```bash
PROJ=/workspace/repos/minesweeper
ASSETS="$PROJ/build/android/app/src/main/assets"
mkdir -p "$ASSETS"
rsync -a --delete "$PROJ/data/" "$ASSETS/"
```

Config that the Orx shell loads must exist under that assets tree (same relative names as desktop).

## 3. Build the APK

```bash
cd /workspace/repos/minesweeper/build/android
./build.sh
# → app/build/outputs/apk/
```

`build.sh` is `./gradlew clean assemble`. Debug APK is usually:

`app/build/outputs/apk/debug/app-debug.apk`

Install:

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Touch wiring (landed)

`touch_bridge` forwards Orx `TOUCH_*` into `gesture_touches`; mouse is suppressed while fingers are down (no Android double-fire). `touch_bridge.c` is in `Android.mk`. Tester still gates Android “playable” on `touch_bridge_was_used` / bridge fixtures plus a real APK.

Studio/SDK are up; rebuild APK after UI/gesture locks land.

## Controls (locked 2026-09-08 — phone)

| Control | Action |
| --- | --- |
| Bottom-right **circular HUD** (screen-space) | Cycles Reveal→Flag→Question; icons mine/flag/`?`; camera-independent hit-test |
| Single-tap closed cell | Current HUD mode (reveal / flag / `?`) — flag and `?` exclusive |
| Single-tap **revealed number** | Chord (`mines_chord`) if **flags** == N (`?` ignored in count); may open/clear `?` neighbors |
| One-finger drag | Pan (**clamped** to playfield) |
| Pinch | Zoom (no tear / shadow text) |
| ~~2-finger tap flag~~ | **Removed** |

Portrait via **taller camera frustum** (same pixel pipeline as desktop — **no** nonuniform backbuffer stretch). Zoom min/max must allow zoom-in; default zoom = tappable cells. Cell art: white base + color blend; settings color wheel for tint.

Hit-test uses **world coords after camera**. Never bypass headless `mines_*`.

## Not verified on the agent box

This machine often lacks Android SDK/NDK. Treat APK steps as run-on-dev-machine until CI/SDK is present. Headless logic does **not** need Android:

```bash
cd /workspace/repos/minesweeper && make test
```

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Prefab / Orx AAR missing | Re-run `$ORX/build/android/build.sh`; confirm `$ORX/lib/static/android/repository` |
| Empty / wrong assets | Re-rsync `data/` → `app/src/main/assets` |
| NDK version mismatch | Match `ndkVersion` in `app/build.gradle` (r27.3.13750724) |
| Native lib not found | Manifest `android.app.lib_name` must be `Minesweeper` |
