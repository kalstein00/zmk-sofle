# Windows native build (normal mode)

Run in PowerShell on Windows, from this repository. This builds the existing left Central, right Peripheral, and keyboard settings-reset firmware. It does not build dongle firmware. The steps below were checked with Python 3.12.10, west 1.5.0, CMake 3.27.9, Ninja 1.13.2 and Zephyr SDK 0.16.8. Git and Python 3.12 must be available on PATH; allow disk space and network access. Use a fresh workspace, outside this repository:

```powershell
$repo = (Get-Location).Path
$ws = 'C:\temp\zmk-normal'
$venv = 'C:\temp\zmk-normal-venv'
$sdkRoot = 'C:\temp\zmk-sdk'
python -m venv $venv
& "$venv\Scripts\python.exe" -m pip install -r "$repo\requirements\windows-build.txt"
$env:PATH = "$venv\Scripts;$env:PATH"
$env:PYTHONPATH = "$venv\Lib\site-packages"
New-Item -ItemType Directory -Force "$ws\config", $sdkRoot | Out-Null
Copy-Item "$repo\config\west.yml" "$ws\config\west.yml"
curl.exe -L --fail -o "$ws\sdk.7z" https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/zephyr-sdk-0.16.8_windows-x86_64_minimal.7z
curl.exe -L --fail -o "$ws\arm.7z" https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/toolchain_windows-x86_64_arm-zephyr-eabi.7z
python -m py7zr x "$ws\sdk.7z" $sdkRoot
python -m py7zr x "$ws\arm.7z" "$sdkRoot\zephyr-sdk-0.16.8"
$env:ZEPHYR_SDK_INSTALL_DIR = "$sdkRoot\zephyr-sdk-0.16.8"
Set-Location $ws
west init -l config
west update
```

Run each build in a separate clean directory. Stop if any command returns a nonzero exit code; a stale UF2 is not proof of success. These commands pass the repository explicitly as the local module and config. The C flag works around a Windows drive-letter macro collision while using Zephyr's same `lv_conf.h` through its include path.

```powershell
$zephyr = "$ws/zephyr/share/zephyr-package/cmake"
west build -p always -s zmk/app -d "$ws/build-left" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=eyelash_sofle_left nice_view' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH' '-DSNIPPET=studio-rpc-usb-uart' '-DCONFIG_ZMK_STUDIO=y' '-DCONFIG_ZMK_STUDIO_LOCKING=n'
west build -p always -s zmk/app -d "$ws/build-right" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=eyelash_sofle_right nice_view' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
west build -p always -s zmk/app -d "$ws/build-reset" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=settings_reset' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
Get-FileHash "$ws/build-left/zephyr/zmk.uf2", "$ws/build-right/zephyr/zmk.uf2", "$ws/build-reset/zephyr/zmk.uf2" -Algorithm SHA256
```

Copy each `zmk.uf2` to a distinct name (for example `normal-left.uf2`, `normal-right.uf2`, `keyboard-settings-reset.uf2`). Reset firmware clears settings; do not treat it as normal firmware. Inspect the build log's `Found devicetree overlay` lines: left/right must use this checkout's `boards/shields/eyelash_sofle/...overlay`, and nice_view must come from ZMK. Both the external `eyelash_sofle` module and this checkout contain shield definitions, so merely seeing a shield name is insufficient. Check `zephyr/zephyr.dts` and `zephyr/.config` for the actual merged result. The manifest pins Zephyr, nanopb and the direct projects; imported dependencies are listed in the active lock snapshot. Record `west list -f '{name} {revision} {path}'`, project commit IDs, tool versions, logs and UF2 hashes with a build.

The existing normal mode uses the left as Central and right as Peripheral. Existing keymap controls select Bluetooth profiles and USB/BLE output; Q/S/Z held for two seconds enters deep sleep and requires reset to wake. DYA connects to the left via USB. Build success alone does not verify key input, encoder, USB/BLE selection, sleep, OLED or DYA editing/saving on physical devices. Check these on both keyboards before relying on a firmware change.

## 실기기 회귀 확인 (미수행)

일반 모드 UF2를 각각 해당 좌우 기기에 설치하고, 왼쪽을 USB로 PC에 연결한 상태에서 다음을 기준 펌웨어와 새 펌웨어 양쪽에 반복해 결과를 비교한다. 설정 초기화는 저장된 설정을 지울 수 있으므로 백업 여부를 확인하고 필요한 경우에만 수행한다.

1. 양쪽에서 대표 문자 키와 수정키를 입력하고 각 레이어로 전환해 해당 문자가 예상대로 나오는지 확인한다. 엔코더를 양방향으로 돌려 기존 동작과 비교한다.
2. 기존 키맵의 Bluetooth 프로필 선택 및 USB/BLE 출력 전환을 이용해 선택한 호스트에만 입력이 전달되는지 확인한다.
3. 활동을 멈춰 자동 절전 후 키 입력으로 복귀하는지 확인한다. Q/S/Z를 2초 누른 뒤 깊은 절전이 시작되고 리셋 버튼으로 복귀하는지도 확인한다.
4. 왼쪽 USB에 DYA Studio를 연결해 키맵을 읽고 시험 바인딩을 편집한다. 재부팅 전 실제 입력의 변경을 확인한 후 Save하고 전원을 다시 연결해 저장 결과를 확인한다.

실기기가 없어서 위 항목과 OLED, 무선 안정성은 확인하지 않았다. 컴파일 결과와 별도로 기록해야 한다.
