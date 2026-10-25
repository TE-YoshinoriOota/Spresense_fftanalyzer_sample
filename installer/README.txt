SpreMachineScope インストーラ / Installer
==========================================

[日本語]

■ 事前準備（初回だけ）
1. ブートローダ v3.4.3
   Spresense に、このファームウェアと同じ版のブートローダが必要です。
   Arduino IDE で「ボード: Spresense」（ボードパッケージ 3.4.7）を選び、
   「ツール」>「ブートローダを書き込む」を実行してください（Sony の使用許諾に同意します）。
   すでに Arduino IDE 3.4.7 環境で書き込んだことがあれば不要です。
2. USB ドライバ（Windows のみ）
   Silicon Labs の CP210x USB to UART ドライバを入れてください。
3. シリアルポートの権限（Linux のみ）
   sudo usermod -aG dialout $USER を実行し、ログインし直してください。
4. DSP（SRC）
   16 kHz と 192 kHz で使うには、SD カードの BIN/SRC が必要です。
   Arduino IDE の「スケッチ例」>「Audio」>「dsp_installer」>「src_installer」で入れてください。
   48 kHz だけで使う場合は不要です。

■ インストール
1. Spresense メインボードを USB でつなぎ、SD カードをパソコンに挿します。
2. Windows: install.bat をダブルクリックします。
   Linux:   ./install.sh を実行します。
3. 表示されたシリアルポートを選ぶと、Maincore と Subcore1 を書き込みます。
4. SD カードの場所（Windows はドライブ文字、Linux はフォルダ）を入力すると、
   設定ファイルを config フォルダにコピーします。
   すでに config フォルダがあるときは config_backup_日時 に名前を変えて残します。
5. SD カードを Spresense に挿し、リセットボタンを押すと起動します。

[English]

- Before the first install
1. Bootloader v3.4.3
   Spresense needs the bootloader of the same version as this firmware.
   In the Arduino IDE, select the board "Spresense" (board package 3.4.7)
   and run "Tools" > "Burn Bootloader" (you accept the license of Sony).
   Not needed if you have uploaded sketches with the board package 3.4.7.
2. USB driver (Windows only)
   Install the Silicon Labs CP210x USB to UART driver.
3. Permission of the serial port (Linux only)
   Run sudo usermod -aG dialout $USER and log in again.
4. DSP (SRC)
   16 kHz and 192 kHz need BIN/SRC on the SD card. Install it with
   "Examples" > "Audio" > "dsp_installer" > "src_installer" of the Arduino IDE.
   Not needed if you use only 48 kHz.

- Install
1. Connect the Spresense main board with USB, and insert the SD card into your PC.
2. Windows: double-click install.bat.
   Linux:   run ./install.sh.
3. Select the serial port: Maincore and Subcore1 are written.
4. Enter the SD card (a drive letter on Windows, a folder on Linux):
   the setting files are copied to the config folder. An existing config
   folder is renamed to config_backup_<date and time>.
5. Insert the SD card into Spresense and press the reset button.

- Files
  firmware/   Maincore.spk, Subcore1.spk
  config/     setting files for the SD card
  tools/      flash_writer (Sony Semiconductor Solutions, LGPL v2.1: LICENSE-flash_writer.txt)
