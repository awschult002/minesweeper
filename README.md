# Minesweeper (Orx / Android)

Orx-based Minesweeper scaffold. Pure grid rules are headless and unit-tested;
Orx shell + Android template are ready for Senior (pan/zoom + gestures).

## Layout

| Path | Role |
|------|------|
| `/workspace/repos/orx` | Cloned Orx engine (`setup.sh` already run) |
| `src/game/` | Pure C grid API (`mines_reveal` / `mines_flag` / `mines_chord`) |
| `src/view/` | Dumb ortho camera + world-space cell pick (outside grid) |
| `src/minesweeper.c` | Lean Orx shell (hooks camera stub; gestures TBD) |
| `tests/` | Headless fixtures (no Orx / Android) |
| `build/android/` | **Currently missing** — will be restored from the Orx Android demo template (`orx/code/demo/android`) |
| `data/config/` | Desktop Orx `.ini` |
| `docs/STATUS.md` | Living locks / gates / milestone |
| `docs/ANDROID.md` | How to build the APK |

## Headless tests (no Orx)

```bash
cd /workspace/repos/minesweeper
gcc -std=c99 -Wall -Wextra -O2 -Isrc/game -Isrc/view \
  -o tests/test_mines tests/test_mines.c src/game/mines.c src/view/board_camera.c
./tests/test_mines
```

Or: `make test` (if `make` is installed).

Expected: `mines tests: N passed, 0 failed` covering Tester gates:
1. zero-flood stops at numbers/flags
2. chord no-op when \|F\|≠N
3. chord lose on misflag
4. chord into zero continues flood
5. corner/edge neighbor counts &lt; 8
6. win = all safes open
7. flag toggle never opens

## Orx clone / setup (done on this box)

```bash
git clone https://github.com/orx/orx.git /workspace/repos/orx
cd /workspace/repos/orx && ./setup.sh   # downloads extern, sets ORX, generates IDE projects
```

`ORX` should point at `/workspace/repos/orx/code`. Project created with:

```bash
/workspace/repos/orx/init.sh /workspace/repos/minesweeper -bundle -scroll -c++ ...
# (C project, extensions off)
```

## Desktop Orx build (stubs)

```bash
export ORX=/workspace/repos/orx/code
# Build Orx libs first (needs system deps: libgl1-mesa-dev, libxrandr-dev, …)
cd "$ORX/build/linux/gmake" && make config=release64   # also debug64 / profile64

cd /workspace/repos/minesweeper/build
./premake4 gmake
cd linux/gmake && make config=release64
# binary → ../../bin/minesweeper
```

## Android build (not verified on this machine)

Full how-to: [docs/ANDROID.md](docs/ANDROID.md).

**Note:** `build/android/` is currently missing from this tree. It will be restored from the Orx Android demo template (`orx/code/demo/android`). APK steps below will not run until that template is restored.

Prereqs: Android Studio / SDK / NDK r27+, `ORX` env set.

1. Build Orx Android AARs:

```bash
cd /workspace/repos/orx/code/build/android && ./build.sh
# AARs → orx/code/build/android/orx/build/outputs/aar
# Published for Gradle via $ORX/lib/static/android/repository/
```

2. Sync desktop `data/` into `build/android/app/src/main/assets/` (Android cannot use `../` parent paths).

3. Build APK:

```bash
cd /workspace/repos/minesweeper/build/android && ./build.sh
# APK → app/build/outputs/apk/
```

App id: `org.orx.minesweeper`, native module: `Minesweeper`.

Wiki refs:
- https://orx-project.org/wiki/en/tutorials/android/setup_android
- https://orx-project.org/wiki/en/tutorials/android/using_the_android_demo_as_a_template_for_your_own_projects

## Input contract (Senior)

| Platform | Closed-cell tap/click | Chord | Camera / HUD |
|----------|----------------------|-------|----------------|
| Android (product) | HUD mode: Reveal / Flag / Question (bottom-right cycle) | Tap **revealed** N when flags==N | Portrait; pan clamped; pinch zoom; **no** 2-finger flag |
| Desktop | TBD remap (was L reveal / R flag) | TBD (was L+R / L on open N) | Drag pan / wheel zoom; clamp required |

Grid API never sees gestures — only `mines_reveal` / `mines_flag` / question / `mines_chord`. See [docs/STATUS.md](docs/STATUS.md).
