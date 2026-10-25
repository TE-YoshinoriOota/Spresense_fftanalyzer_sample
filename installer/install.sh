#!/bin/sh
# SpreMachineScope installer for Linux
#   1. writes the firmware of Maincore and Subcore1 to Spresense
#   2. copies the setting files to the SD card
cd "$(dirname "$0")" || exit 1
FW=./tools/flash_writer
chmod +x "$FW" 2>/dev/null

echo "=== 1. Write the firmware / ファームウェアの書き込み ==="
ports=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null)
if [ -z "$ports" ]; then
  echo "Spresense is not found. Connect the main board with USB."
  echo "Spresense が見つかりません。メインボードを USB でつないでください。"
else
  echo "Serial ports / シリアルポート:"
  for p in $ports; do echo "  $p"; done
  def=$(echo "$ports" | head -n 1)
  printf "Port [%s] (s: skip): " "$def"
  read port
  [ -z "$port" ] && port=$def
  if [ "$port" != "s" ]; then
    if [ ! -w "$port" ]; then
      echo "No permission to $port. Run: sudo usermod -aG dialout \$USER (then log in again)"
      echo "$port に書き込めません。上のコマンドを実行し、ログインし直してください。"
      exit 1
    fi
    if "$FW" -s -c "$port" -d -n firmware/Maincore.spk firmware/Subcore1.spk; then
      echo "Done / 書き込み完了"
    else
      echo "Failed. Check the bootloader (README.txt) and the USB cable."
      echo "失敗しました。ブートローダ（README.txt）と USB ケーブルを確認してください。"
      exit 1
    fi
  fi
fi

echo
echo "=== 2. Prepare the SD card / SD カードの準備 ==="
for d in /media/"$USER"/* /run/media/"$USER"/*; do [ -d "$d" ] && echo "  $d"; done
printf "Folder of the SD card (empty: skip) / SD カードのフォルダ（空欄で省略）: "
read sd
if [ -n "$sd" ]; then
  if [ ! -d "$sd" ]; then
    echo "$sd is not found / $sd が見つかりません"
    exit 1
  fi
  if [ -d "$sd/config" ]; then
    backup="$sd/config_backup_$(date +%Y%m%d%H%M%S)"
    mv "$sd/config" "$backup" || exit 1
    echo "The old settings are kept in / 以前の設定の保存先: $backup"
  fi
  cp -r config "$sd/config" || exit 1
  sync
  echo "Copied config / config をコピーしました"
  if [ ! -f "$sd/BIN/SRC" ]; then
    echo "Note: BIN/SRC is not on the SD card. 16 kHz and 192 kHz need it (README.txt)."
    echo "注意: SD カードに BIN/SRC がありません。16 kHz と 192 kHz で必要です（README.txt）。"
  fi
fi
echo
echo "Finished / 終了しました"
