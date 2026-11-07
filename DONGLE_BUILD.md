# Sofle 동글 모드 빌드와 설치

동글 모드는 왼쪽과 오른쪽을 BLE Peripheral로, XIAO nRF52840을 USB Central로 사용한다. 일반 모드와 전환하려면 각 장치를 해당 모드의 펌웨어로 다시 플래시해야 한다. 동글은 키보드와 BLE로 통신하며, 컴퓨터로는 USB HID를 사용한다.

## Windows 네이티브 빌드

먼저 [Windows 빌드 환경](WINDOWS_BUILD.md)을 준비하고 그 문서의 `$repo`, `$ws`, `$zephyr`, Python 가상환경과 SDK 설정을 사용한다. 각 명령이 성공했는지 확인하고 별도 이름으로 복사한다. 아래 명령은 기존 Windows LVGL 전처리기 충돌을 동일하게 우회한다.

```powershell
Set-Location $ws
west build -p always -s zmk/app -d "$ws/build-dongle-left" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=eyelash_sofle_dongle_left nice_view' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
west build -p always -s zmk/app -d "$ws/build-dongle-right" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=eyelash_sofle_dongle_right nice_view' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
west build -p always -s zmk/app -d "$ws/build-dongle-central" -b seeeduino_xiao_ble -- "-DZephyr_DIR=$zephyr" '-DSHIELD=sofle_dongle' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
Copy-Item "$ws/build-dongle-left/zephyr/zmk.uf2" "$ws/dongle-left.uf2"
Copy-Item "$ws/build-dongle-right/zephyr/zmk.uf2" "$ws/dongle-right.uf2"
Copy-Item "$ws/build-dongle-central/zephyr/zmk.uf2" "$ws/dongle-central-xiao.uf2"
Get-FileHash "$ws/dongle-left.uf2", "$ws/dongle-right.uf2", "$ws/dongle-central-xiao.uf2" -Algorithm SHA256
```

빌드 로그에서 좌우 shield와 Central shield가 이 저장소에서 선택됐는지 확인한다. 컴파일 성공만으로 무선 통신이나 HID 동작이 확인되는 것은 아니다.

## 설치와 최초 연결

1. 각 장치를 해당 부트로더 모드로 진입시킨 뒤 `dongle-left.uf2`는 왼쪽, `dongle-right.uf2`는 오른쪽, `dongle-central-xiao.uf2`는 XIAO 동글에 복사한다. 장치를 잘못 선택하지 않도록 부트로더 드라이브를 하나씩 연결한다.
2. 동글을 USB 호스트에 연결하고 왼쪽과 오른쪽을 차례로 켠다. 각각 연결된 후 대표 키를 입력해 확인한다. 한쪽만 연결한 경우와 다른 쪽을 나중에 켠 경우도 확인한다.
3. 양쪽 문자, 수정키 누름과 해제, 레이어 전환, 마우스와 왼쪽 엔코더 양방향을 확인한다. 동글 모드의 기존 Bluetooth 호스트 선택·삭제와 BLE 출력 전환 위치는 무동작이다.

기존 본딩 때문에 연결이 되지 않으면 저장된 설정 초기화가 필요할 수 있다. 초기화는 사용자 설정도 지울 수 있다. 완성된 복구 및 모드 전환 절차는 #8에서 다루며, 기존 일반 모드 설치와 초기화 빌드는 [Windows 빌드 안내](WINDOWS_BUILD.md)를 참고한다. 동글 모드에서 기존 깊은 절전 바인딩은 동글 종료를 막기 위해 임시 무동작이며, 양쪽의 깊은 절전 및 복귀는 #7에서 다룬다. OLED 표시와 DYA 동작은 각각 #5, #4 범위다.

이 절차의 실제 기기 플래시, 페어링, USB HID, 연결 해제·복귀 및 엔코더 동작은 아직 검증되지 않았다.
