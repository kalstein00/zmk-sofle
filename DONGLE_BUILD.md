# Sofle 동글 모드 빌드와 설치

> 실기 확인: 사용 중인 동글의 부트로더는 nice!nano로 식별됐으며, 이전 XIAO 대상 빌드는 해당 장치에서 USB로 인식되지 않았다. nice!nano 대상 후보에서 양쪽 문자 입력과 DYA 연결을 확인했다. 장치 식별과 이전 시험의 한계는 [왼쪽 설정 기록](LEFT_KEYBOARD_SETUP_RECORD.md)과 [오른쪽 설정 기록](RIGHT_KEYBOARD_SETUP_RECORD.md)을 참고한다.

동글 모드는 왼쪽과 오른쪽을 BLE Peripheral로, nice!nano 대상으로 식별된 동글을 USB Central로 사용한다. 일반 모드와 전환하려면 각 장치를 해당 모드의 펌웨어로 다시 플래시해야 한다. 동글은 키보드와 BLE로 통신하며, 컴퓨터로는 USB HID와 Studio 직렬 연결을 사용한다.

## Windows 네이티브 빌드

먼저 [Windows 빌드 환경](WINDOWS_BUILD.md)을 준비하고 그 문서의 `$repo`, `$ws`, `$zephyr`, Python 가상환경과 SDK 설정을 사용한다. 각 명령이 성공했는지 확인하고 별도 이름으로 복사한다. 아래 명령은 기존 Windows LVGL 전처리기 충돌을 동일하게 우회한다.

```powershell
Set-Location $ws
west build -p always -s zmk/app -d "$ws/build-dongle-left" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=eyelash_sofle_dongle_left nice_view' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
west build -p always -s zmk/app -d "$ws/build-dongle-right" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=eyelash_sofle_dongle_right nice_view' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
west build -p always -s zmk/app -d "$ws/build-dongle-central" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=sofle_dongle' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH' '-DSNIPPET=studio-rpc-usb-uart'
Copy-Item "$ws/build-dongle-left/zephyr/zmk.uf2" "$ws/dongle-left.uf2"
Copy-Item "$ws/build-dongle-right/zephyr/zmk.uf2" "$ws/dongle-right.uf2"
Copy-Item "$ws/build-dongle-central/zephyr/zmk.uf2" "$ws/dongle-central-nicenano.uf2"
Get-FileHash "$ws/dongle-left.uf2", "$ws/dongle-right.uf2", "$ws/dongle-central-nicenano.uf2" -Algorithm SHA256
```

빌드 로그에서 좌우 shield와 Central shield가 이 저장소에서 선택됐는지 확인한다. 컴파일 성공만으로 무선 통신이나 HID 동작이 확인되는 것은 아니다.

## 설치와 최초 연결

1. 각 장치를 해당 부트로더 모드로 진입시킨 뒤 `dongle-left.uf2`는 왼쪽, `dongle-right.uf2`는 오른쪽, `dongle-central-nicenano.uf2`는 부트로더가 `nRF52840-nicenano`로 식별되는 동글에 복사한다. 장치를 잘못 선택하지 않도록 부트로더 드라이브를 하나씩 연결한다.
2. 동글을 USB 호스트에 연결하고 왼쪽과 오른쪽을 차례로 켠다. 각각 연결된 후 대표 키를 입력해 확인한다. 한쪽만 연결한 경우와 다른 쪽을 나중에 켠 경우도 확인한다.
3. 양쪽 문자, 수정키 누름과 해제, 레이어 전환, 마우스와 왼쪽 엔코더 양방향을 확인한다. 동글 모드의 기존 Bluetooth 호스트 선택·삭제와 BLE 출력 전환 위치는 무동작이다.

기존 본딩이 없는 최초 설치에는 설정 초기화가 필요하지 않다. 연결 실패 시 장치 전원과 올바른 UF2를 먼저 확인하고, 필요할 때만 아래 복구를 수행한다. 일반 모드와의 전환 및 복구 실기 검증은 [#9](https://github.com/kalstein00/zmk-sofle/issues/9)에 남아 있다.

## 동글 모드 연결 복구

**설정 초기화는 실행한 장치의 저장 설정 전체와 BLE 본딩을 지운다.** 특히 동글에 저장한 DYA 키맵과 관련 사용자 설정도 삭제될 수 있다. 초기화 전에 DYA의 각 레이어 바인딩과 엔코더 등 필요한 설정을 기록한다. 완전한 내보내기·가져오기나 자동 백업·복원은 검증되지 않았다. 초기화 후 기록한 내용을 DYA에서 다시 입력하고 `Save`한다. 일반 모드 왼쪽의 설정은 동글에 자동 이전되지 않는다.

1. 아래 빌드 명령으로 초기화 UF2를 준비한다. 실제 세 장치가 `nRF52840-nicenano` 부트로더로 식별된 경우 같은 `settings-reset-nicenano.uf2`를 사용한다. XIAO 대상은 실제 XIAO 보드에만 사용하며, 관찰된 nice!nano 동글에는 사용하지 않는다.
2. 동글과 좌우를 한 번에 하나씩 부트로더에 넣고 식별을 확인한 후 각각 초기화 UF2를 복사한다. 복사 후 부트로더 드라이브가 사라지면 10초 기다려 초기화 실행 시간을 확보한다. 세 장치 모두 초기화가 끝날 때까지 일반 펌웨어를 다시 설치하지 않는다. 초기화 펌웨어는 BLE가 꺼져 있으며 일반 입력용이 아니다.
3. 동글에 `dongle-central-nicenano.uf2`, 왼쪽에 `dongle-left.uf2`, 오른쪽에 `dongle-right.uf2`를 하나씩 다시 설치한다. 동글을 USB에 연결하고 좌우를 켜거나 리셋해 재페어링한다. 양쪽을 하나씩 연결해 각 슬롯과 입력을 확인한다.
4. DYA에 동글 USB 직렬 포트로 연결해 기록한 설정을 복원하고 Save한다. 전원을 다시 연결해 저장된 키맵과 양쪽 입력을 확인한다.

이번 통합 검증에서는 사용자 결정으로 설정 초기화를 수행하지 않았으므로 세 장치를 모두 초기화하고 재페어링하는 동글 모드 전체 복구와 본딩 없는 최초 설치는 실기 검증하지 않았다. 연결 실패의 원인을 본딩으로 단정하지 않는다.

```powershell
Set-Location $ws
west build -p always -s zmk/app -d "$ws/build-reset-nicenano" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=settings_reset' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
west build -p always -s zmk/app -d "$ws/build-reset-xiao" -b seeeduino_xiao_ble -- "-DZephyr_DIR=$zephyr" '-DSHIELD=settings_reset' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH'
Copy-Item "$ws/build-reset-nicenano/zephyr/zmk.uf2" "$ws/settings-reset-nicenano.uf2"
Copy-Item "$ws/build-reset-xiao/zephyr/zmk.uf2" "$ws/settings-reset-xiao.uf2"
Get-FileHash "$ws/settings-reset-nicenano.uf2", "$ws/settings-reset-xiao.uf2" -Algorithm SHA256
```


## 동글 화면

동글 화면은 `LEFT`, `RIGHT`의 잔량을 숫자와 막대로 표시한다. BLE 연결 슬롯은 물리적인 좌우가 아니므로, 각 반쪽에서 키를 처음 누를 때 입력 위치로 해당 연결의 좌우를 식별한다. 연결 후 양쪽 키를 각각 한 번씩 누른다. 아직 식별하지 못했거나 연결되지 않았거나 배터리 값을 받지 못한 쪽은 `--`로 표시한다. 연결이 끊기면 식별과 이전 값은 지워지며 재연결 후 다시 키를 눌러야 한다. 숫자는 마지막으로 받은 유효 잔량이며 `0%`도 표시한다. 잔량이 20% 이하인 쪽의 `!`가 깜박인다. 입력 시 고양이가 앞발을 움직이고, 입력이 없을 때에도 가끔 모습이 바뀐다. `USB: OK`는 USB HID 준비, `USB: --`는 준비되지 않은 상태이고 `Layer`는 현재 활성 레이어 번호다. OLED 화면은 시뮬레이터로 확인했고, 사용자는 최종 동글 펌웨어를 설치해 정상 작동한다고 확인했다. 20% 이하 실기기 경고는 별도 확인하지 않았다.

2026-11-10 실기 비교에서 최초 OLED 후보는 일반 오른쪽 Shift와 `MT RShift J`가 동작하지 않았다. 키 입력 관찰을 BLE 콜백에서 제거한 비교 후보에서는 일반 오른쪽 Shift가 정상 입력됐고, 입력 전달 후 화면 상태를 갱신하도록 바꾼 수정 후보에서는 사용자가 두 Shift의 대문자 입력을 확인했다. 수정 후보의 UF2 SHA-256은 `090DB51BB7432289768D0C69DF364251ADA684E679E83F3BA26FD791F026F5A6`이다. 정확한 내부 실패 기전은 확인하지 못했다.

화면 구성은 실제 장치에서 동작한 nice!nano 호환 I²C SDA P0.17/D2, SCL P0.20/D3, 0x3c, 128×64 및 SH1106 호환 경로(열 오프셋 2)를 사용한다. 이는 관찰된 호환 설정이며 실제 칩의 표기나 다른 장치의 배선을 보증하지 않는다. Bongo Cat 이미지는 `englmaxi/zmk-dongle-display`의 LVGL 8 호환 revision `c6c4be11c5ede722086a2a1f355cec9c101b9390`에서 가져왔다. 연결·배터리 상태는 실제 수신과 연결 해제를 구분하기 위해 고정 ZMK의 BLE callback에서 별도 snapshot으로 읽는다.

기본 idle 시간인 30초 동안 입력이 없으면 OLED가 꺼진다. 화면 소등 중에도 USB와 키보드 BLE 연결은 동작하며 키 또는 지원 엔코더 입력으로 화면과 입력이 돌아온다. 배터리나 연결 상태 갱신은 화면을 깨우지 않는다. DYA 전원 설정에 저장된 idle 값에 따라 시간은 달라질 수 있으며, 0이면 자동 idle이 꺼진다. OLED 소등은 동글 종료나 키보드 Deep Sleep을 뜻하지 않는다.

OLED 갱신은 별도 작업 큐에서 수행한다. 화면과 키 입력을 같은 시스템 작업 큐에서 처리하던 후보와 비교했을 때, 사용자는 별도 큐 시험 후보의 입력 반응이 확실히 좋아졌고 OLED도 정상이라고 확인했다. 이는 개별 장치의 체감 비교이며 지연 시간의 정량 측정은 아니다.

기본 키맵에서 Q/S/Z 조합에는 별도 동작이 없고, 레이어 2의 `.` 위치는 무동작이다. 키보드 좌우는 기본적으로 60분 동안 입력이 없으면 Deep Sleep에 들어가며 키로 깨울 수 있다. 복귀와 BLE 재연결 중 첫 입력은 전달되지 않을 수 있다. 저장된 DYA 설정에 따라 기본값과 실제 동작이 다를 수 있다. 15초 시험 빌드에서 좌우 연결 해제와 키 입력 후 복귀를 확인했으며, 60분 정상 빌드의 진입·복귀는 별도 확인하지 않았다. 자세한 내용은 [실기 기록](DEEP_SLEEP_TEST_RECORD.md)을 참고한다.

## DYA Studio

[DYA Studio](https://studio.dya.cormoran.works/)에서 USB 연결을 선택하고 동글의 직렬 포트를 선택한다. 동글 모드의 설정은 동글에, 일반 모드의 설정은 왼쪽 Central에 저장되며 자동 이전되지 않는다. 키맵 편집은 즉시 적용되며 `Save`가 영구 저장이다. 시험 전 원래 바인딩을 기록하고 시험 후 복구한다. 설정 초기화는 별도 동의 없이 수행하지 않는다.

| 기능 | 동글 구성 및 저장 계약 | 검증 상태 |
| --- | --- | --- |
| 키맵·레이어 | Studio USB RPC, 편집 즉시 적용, `Save`로 저장 | 이전 #4 후보에서 키 편집·재연결과 LAYER1 편집·실제 입력 확인 |
| 런타임 엔코더 | Studio RPC 활성화; 방향 설정은 선택 시 별도 저장 | 이전 #4 후보에서 양 방향 편집·입력과 원래 음량 기능 복구 확인 |
| 런타임 input processor | Studio RPC 활성화 | 서브시스템 응답 확인; 장치에 processor 없음, 편집·저장 미검증 |
| settings RPC | Studio RPC 활성화; idle/sleep 쓰기는 키맵 Save와 별개로 지연 저장 | 서브시스템 및 전원 설정 읽기 확인; 쓰기 제한은 아래 참고 |
| BLE 호스트 관리 | USB 전용 동글에서 비활성 | 일반 모드 유지, 실기 회귀 미검증 |
| 배터리 이력 | 비활성 | 미지원 |

Studio BLE 연결과 동글의 Bluetooth 호스트 출력은 제공하지 않는다. 위의 확인 상태는 빌드 설정만으로 실기 동작을 보장하지 않는다.

현재 동글 빌드에서는 sleep이 꺼져 있다. 고정된 settings RPC 구현은 idle을 먼저 변경한 뒤 sleep도 변경하려 하므로 요청이 실패해도 idle은 이미 적용되고 60초 뒤 저장될 수 있다. DYA에서 이 전원 설정 쓰기를 사용하지 말고, 실패 응답만으로 기존 값이 유지됐다고 가정하지 않는다. 이 경로의 편집·저장 및 Peripheral 전파는 실기에서 검증하지 않았다.

최종 후보 빌드와 검증 범위는 [통합 빌드 기록](ISSUE8_BUILD_EVIDENCE.md)을 참고한다.
