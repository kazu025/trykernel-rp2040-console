# TryKernel UART Console for RP2040

RP2040 / Raspberry Pi Pico 上で動作する TryKernel 学習用プロジェクトです。

現在は、TryKernel 上で以下の機能を実装しています。

* LED制御タスク
* UART受信タスク
* UART受信リングバッファ
* 1行入力コンソール
* コマンドテーブル方式のコマンド処理
* UARTコンソールからのLED制御

今後、I2Cセンサーなどを追加しやすくするため、UARTドライバ、コンソール処理、コマンド処理、タスク処理を分離した構成にしています。

---

## Based on

This project is based on **Try Kernel** introduced in CQ Publishing **Interface 2023年7月号** 特集「ラズパイPicoで1500行 ゼロから作るOS」.

Try Kernel 本体、および Interface 2023年7月号の配布プログラムに由来するコードについては、元の Try Kernel / Interface 配布プログラムのライセンスに従います。

本リポジトリでは、Try Kernel を学習用に使用し、UARTコンソール、コマンドテーブル、タスク構成整理などを追加・変更しています。

---

## Target

* Board: Raspberry Pi Pico / RP2040
* Base: CQ Publishing Interface 2023年7月号 Try Kernel
* Kernel: TryKernel
* Language: C
* UART: UART0
* LED: Pico onboard LED GPIO25

---

## Current Features

### UART Console

UART経由で簡易コンソールを操作できます。

UART受信タスクは、UARTのRX FIFOをポーリングし、受信データをソフトウェアリングバッファへ格納します。
その後、リングバッファから1文字ずつ取り出し、コンソール入力処理へ渡します。

処理の流れは以下です。

```text
UART RX FIFO
    ↓
UART software ring buffer
    ↓
console_input_char()
    ↓
line buffer
    ↓
command dispatcher
```

---

### Command Table

コマンド処理は、コマンドテーブル方式で実装しています。

現在対応しているコマンドは以下です。

| Command     | Description                 |
| ----------- | --------------------------- |
| `help`      | コマンド一覧を表示                   |
| `status`    | TryKernelの簡易ステータスとLEDモードを表示 |
| `echo`      | 入力文字列をそのまま表示                |
| `led on`    | LEDを点灯                      |
| `led off`   | LEDを消灯                      |
| `led blink` | LEDを点滅                      |

実行例:

```text
> help
commands:
   help - show command list
   status - show system status
   echo - echo arguments
   led - led on/off/blink

> status
TryKernel status: running
LED mode: OFF

> echo hello
hello

> led on
led on

> led off
led off

> led blink
led blink
```

---

## Task Structure

現在の主なタスク構成は以下です。

```text
usermain()
  ├─ task_led1
  │    └─ LED mode に応じて GPIO25 を制御
  │
  └─ task_uartrx
       ├─ UART RX FIFO をポーリング
       ├─ UARTリングバッファへ格納
       └─ console_input_char() へ入力文字を渡す
```

---

## Source Structure

主なファイルの役割は以下です。

```text
gpio.c / gpio.h
  RP2040 GPIO制御
  Pico onboard LED GPIO25 制御

uart.c / uart.h
  UART0 初期化
  UART送信
  UART受信
  UART受信リングバッファ

console.c / console.h
  1行入力バッファ
  Enter / Backspace 処理
  エコーバック
  プロンプト表示

command.c / command.h
  コマンドテーブル
  コマンド分解
  help/status/echo/led コマンド

task_led.c / task_led.h
  LED制御タスク
  LEDモード管理

task_uartrx.c
  UART受信タスク

usermain.c
  タスク生成と起動
```

---

## Design Policy

このプロジェクトでは、以下の方針で構成を整理しています。

### 1. UARTドライバはコマンド処理を知らない

`uart.c` は、UARTの送受信と受信リングバッファに集中します。
コンソール入力やコマンド処理は `console.c` / `command.c` に分離します。

### 2. UART受信タスクは1文字入力を渡すだけにする

`task_uartrx.c` は、UART受信とコンソールへの入力受け渡しだけを担当します。
コマンドの内容は知りません。

### 3. console.c は1行入力を作る

`console.c` は、1文字ずつ受け取った入力を行バッファに蓄積し、Enter入力でコマンド処理へ渡します。

### 4. command.c はコマンドテーブルで処理する

`command.c` は、入力された1行を引数に分解し、コマンドテーブルを検索して対応する関数を実行します。

### 5. センサー追加を見据えた構成にする

今後、センサータスクを追加する場合は、以下のような構成にする予定です。

```text
task_sensor.c
  センサーを周期的に取得

sensor_manager.c
  最新のセンサー値を保持

command.c
  sensor コマンドで最新値を表示
```

---

## Build

プロジェクトのMakefileがあるディレクトリでビルドします。

```bash
make
```

生成物は `.gitignore` により Git 管理対象外にすることを推奨します。

例:

```gitignore
build/
*.o
*.elf
*.bin
*.uf2
*.map
*.lst
*.d
.vscode/
```

---

## Serial Console

UART接続後、シリアルターミナルから操作します。

例:

```bash
minicom
```

または、環境に応じて `screen` などを使用します。

```bash
screen /dev/ttyACM0 115200
```

シリアルデバイス名は環境によって異なります。

---

## License

This repository is released under the MIT License for the original code added or modified in this repository.

This project is based on Try Kernel, which was created for CQ Publishing Interface magazine, July 2023 issue, feature article "ゼロから作るOS".

Try Kernel and the source code derived from the original Interface 2023年7月号 sample program are subject to the original Try Kernel license.

Please also refer to the original Try Kernel / Interface Try Kernel license files.

Note: Some boot code in the original Try Kernel distribution may include code or object code derived from the Raspberry Pi Pico C/C++ SDK. Such files are subject to the corresponding Pico SDK license described in the original source distribution.


---

## Future Work

今後の予定です。

* センサー管理モジュールの追加
* ダミーセンサータスクの追加
* `sensor` コマンドの追加
* I2Cドライバの追加
* 実センサーの接続
* READMEの構成図追加
* Note記事化
