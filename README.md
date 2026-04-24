# 3D Scan Viewer

STM32で作った3DスキャナーからUART経由でレーザースキャンデータを受信し、リアルタイムでPhongシェーディング付きの3D点群（ポイントクラウド）を表示するPCアプリケーションです。

## 依存ライブラリ

- **GLFW3** - ウィンドウ管理と入力処理
- **OpenGL 3.3** - グラフィックスAPI
- **GLAD** - OpenGL関数ローダー（`third_party/` に同梱）
- **linmath.h** - ヘッダー1つの線形代数ライブラリ（`third_party/` に同梱）
- Cコンパイラ（C11）

## ビルド方法

```bash
cmake -B build -G Ninja
cmake --build build
```

ビルドすると `build/3d_scan_viewer` という実行ファイルができます。

## コンパイルフラグ

フラグは `src/ov7670.h` で定義されており、ビルド時に上書きできます：

```bash
cmake -B build -G Ninja -DCMAKE_C_FLAGS="-DDISABLE_FRAME_BUFFER=1 -DDISABLE_2D_RENDERER=1"
cmake --build build
```

- `DISABLE_FRAME_BUFFER` (デフォルト: `1`) - 有効にすると、153KBのフレームバッファを確保しません。RGB565のピクセルデータは受け取りますが使いません。メモリの節約になります。最も明るいピクセル位置とステッパーのステップ数だけを使います。
- `DISABLE_2D_RENDERER` (デフォルト: `1`) - 有効にすると、2Dカメラ映像の代わりに3D点群レンダラーを使います。

**おすすめの設定：**

- **スキャンモード**（デフォルト）: 両方のフラグを `1`。フレームデータをスキップし、3D点群を表示します。
- **デバッグ・キャリブレーションモード**: 両方のフラグを `0`。2Dカメラ映像とレーザーラインの重ね合わせ表示で、レーザーの調整や検出の確認ができます。

## 使い方

STM32スキャナーを `/dev/ttyACM0` に接続し（`src/main.c` で変更可能）、次を実行：

```bash
./build/3d_scan_viewer
```

**3Dレンダラーの操作：**
- **左クリックドラッグ** - カメラの回転
- **マウスホイール** - ズームイン/ズームアウト

## アーキテクチャ

### データフロー

```
STM32 Scanner → UART 115200 baud → /dev/ttyACM0
                                           |
                                           v
                                      ov7670.c (4状態FSMパーサー)
                                           |
                     +---------------------+---------------------+
                     |                     |                     |
               frame (RGB565)       brightest[240]          step (uint16_t)
               (有効な場合のみ)      (各行のレーザーX座標)   (ターンテーブル位置)
                     |                     |                     |
                     v                     v                     |
            renderer-image.c      triangulation-math.c ----------+
            (2Dテクスチャ +        calculate_xyz()
             レーザー重ね合わせ)    各行 → (x, y, z)
                                          |
                                          v
                                   renderer-3d.c
                              (GL_POINTS点群
                               Phongシェーディング付き)
```

### UARTプロトコル

STM32とPCは、起動時に1回だけ同期ハンドシェイクを行い、その後はACKベースの順次通信を行います：

**同期フェーズ**（起動時のみ）：
1. STM32が同期マーカー `0xAD 0xEE 0xEE 0xDE` を繰り返し送信
2. PCが同期を検出し、ACKバイト `0x17`（23）を送信してUARTをフラッシュ
3. 両方がデータフェーズに移行

**データフェーズ**（各フレーム）：

```
FRAME (任意)         BRIGHTEST (480 B)      STEP (2 B)
153,600 B            240 × uint16_t         uint16_t
RGB565ピクセル       各行のレーザーX座標     ステッパー位置

PC → MCU: ACKバイト 0x17（次フレームの送信をトリガー）
```

フレームデータは、STM32側で `DISABLE_FRAME_BUFFER` が `0` のときのみ送信されます。

### ソースファイル

- `main.c` - エントリーポイント、メインループ、コンパイルフラグによるレンダラー選択
- `window.c` - GLFWウィンドウ作成とOpenGL 3.3コンテキスト
- `uart.c` - POSIXシリアルポートI/O（raw、ノンブロッキング、8N1）
- `ov7670.c` - UARTフレームパーサー（同期ステートマシン付き）
- `triangulation-math.c` - レーザー三角測量：ピクセル位置 → 3D座標
- `renderer-image.c` - 2Dレンダラー：RGB565カメラ映像をOpenGLテクスチャとして表示＋レーザー重ね合わせ
- `renderer-3d.c` - 3Dレンダラー：Phongシェーディング付き点群とオービットカメラ

### 三角測量の計算

検出された各レーザー位置は、レーザー三角測量で3D点に変換されます：

```
camera_depth = (f × b) / ((brightest_x - x_center) × p + f × tan(α))
radial_depth = (d - camera_depth) / cos(α)
θ = (step / 4096) × 2π

x = radial_depth × cos(θ)
y = -(row - 120) × p × (camera_depth / f)
z = radial_depth × sin(θ)
```

- `f` = 3.6 mm - カメラレンズの焦点距離
- `p` = 0.0072 mm - OV7670のピクセルピッチ
- `b` = 100 mm - カメラからレーザーまでの垂直距離
- `d` = 274.75 mm - カメラからターンテーブル中心までの距離
- `α` = 20° - カメラ軸に対するレーザー角度
- `x_center` = 160 - 幅320ピクセルセンサーの中心列

### 3Dレンダラー

OpenGLのポイントスプライト（`GL_POINTS`）と `gl_PointSize` を使い、遠近感に応じたサイズ調整を行っています。フラグメントシェーダーは：

1. 円の外側のフラグメントを破棄（丸い球にする）
2. ポイントスプライト座標から疑似法線を計算
3. Phongライティングを適用：環境光（0.15）＋拡散光（0.7）＋鏡面反射（0.4）

カメラは球面座標を使って原点の周りを回ります。マウスドラッグとホイールで操作します。

---

A PC application that receives laser scan data from an STM32-powered 3D scanner over UART and renders a real-time 3D point cloud with Phong-shaded point sprites.

## Dependencies

- **GLFW3** - window and input handling
- **OpenGL 3.3** - graphics API
- **GLAD** - OpenGL function loader (bundled in `third_party/`)
- **linmath.h** - single-file linear algebra library (bundled in `third_party/`)
- C compiler (C11)

## Building

```bash
cmake -B build -G Ninja
cmake --build build
```

The build produces a single executable: `build/3d_scan_viewer`.

## Compile Flags

Flags are defined in `src/ov7670.h` and can be overridden at build time:

```bash
cmake -B build -G Ninja -DCMAKE_C_FLAGS="-DDISABLE_FRAME_BUFFER=1 -DDISABLE_2D_RENDERER=1"
cmake --build build
```

- `DISABLE_FRAME_BUFFER` (default: `1`) - When enabled, the 153KB frame buffer is not allocated. Raw RGB565 pixel data is consumed but discarded, saving memory. Only brightest-pixel positions and stepper step are used.
- `DISABLE_2D_RENDERER` (default: `1`) - When enabled, uses the 3D point cloud renderer instead of the 2D camera feed renderer.

**Recommended configurations:**

- **Scan mode** (default): Both flags `1`. Skips frame data, renders 3D point cloud.
- **Debug/calibration mode**: Both flags `0`. Shows live 2D camera feed with laser line overlay for aligning the laser and verifying detection.

## Usage

Connect the STM32 scanner to `/dev/ttyACM0` (configurable in `src/main.c`) and run:

```bash
./build/3d_scan_viewer
```

**3D renderer controls:**
- **Left mouse drag** - orbit camera
- **Scroll wheel** - zoom in/out

## Architecture

### Data Flow

```
STM32 Scanner → UART 115200 baud → /dev/ttyACM0
                                           |
                                           v
                                      ov7670.c (4-state FSM parser)
                                           |
                     +---------------------+---------------------+
                     |                     |                     |
               frame (RGB565)       brightest[240]          step (uint16_t)
               (when enabled)       (laser X per row)     (turntable position)
                     |                     |                     |
                     v                     v                     |
            renderer-image.c      triangulation-math.c ----------+
            (2D texture +         calculate_xyz()
             laser overlay)       per row → (x, y, z)
                                          |
                                          v
                                   renderer-3d.c
                              (GL_POINTS point cloud
                               with Phong shading)
```

### UART Protocol

The STM32 and PC use a one-time sync handshake followed by ACK-based lockstep:

**Sync phase** (startup only):
1. STM32 blasts sync marker `0xAD 0xEE 0xEE 0xDE` repeatedly
2. PC detects sync, sends ACK byte `0x17` (23), flushes UART
3. Both enter data phase

**Data phase** (each frame):

```
FRAME (optional)    BRIGHTEST (480 B)     STEP (2 B)
 153,600 B           240 × uint16_t        uint16_t
 RGB565 pixels       laser X per row       stepper position

PC → MCU: ACK byte 0x17 (triggers next frame)
```

Frame data is only sent when `DISABLE_FRAME_BUFFER` is `0` on the STM32 side.

### Source Files

- `main.c` - Entry point, main loop, renderer selection via compile flags
- `window.c` - GLFW window creation and OpenGL 3.3 context
- `uart.c` - POSIX serial port I/O (raw, non-blocking, 8N1)
- `ov7670.c` - UART frame parser with sync state machine
- `triangulation-math.c` - Laser triangulation: pixel position → 3D coordinates
- `renderer-image.c` - 2D renderer: RGB565 camera feed as OpenGL texture with laser overlay
- `renderer-3d.c` - 3D renderer: point cloud with Phong-shaded point sprites and orbit camera

### Triangulation Math

Each detected laser position is converted to a 3D point using laser triangulation:

```
camera_depth = (f × b) / ((brightest_x - x_center) × p + f × tan(α))
radial_depth = (d - camera_depth) / cos(α)
θ = (step / 4096) × 2π

x = radial_depth × cos(θ)
y = -(row - 120) × p × (camera_depth / f)
z = radial_depth × sin(θ)
```

- `f` = 3.6 mm - Camera lens focal length
- `p` = 0.0072 mm - OV7670 pixel pitch
- `b` = 100 mm - Perpendicular distance from camera to laser
- `d` = 274.75 mm - Distance from camera to turntable center
- `α` = 20° - Laser angle from camera axis
- `x_center` = 160 - Center column of 320-wide sensor

### 3D Renderer

Uses OpenGL point sprites (`GL_POINTS`) with `gl_PointSize` for perspective-correct sizing. The fragment shader:

1. Discards fragments outside a circle (makes round spheres)
2. Computes a pseudo-normal from the point sprite coordinates
3. Applies Phong lighting: ambient (0.15) + diffuse (0.7) + specular (0.4)

The camera orbits the origin using spherical coordinates, controlled by mouse drag and scroll wheel.
