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

기존 본딩 때문에 연결이 되지 않으면 저장된 설정 초기화가 필요할 수 있다. 초기화는 사용자 설정도 지울 수 있다. 완성된 복구 및 모드 전환 절차는 #8에서 다루며, 기존 일반 모드 설치와 초기화 빌드는 [Windows 빌드 안내](WINDOWS_BUILD.md)를 참고한다.


## 동글 화면

동글 화면의 `S1`, `S2`는 BLE 연결 슬롯이며 물리적인 왼쪽과 오른쪽을 뜻하지 않는다. 본딩 순서와 재연결에 따라 어느 쪽이 어느 슬롯인지 달라질 수 있으므로 한쪽씩 켜고 해당 줄을 확인한다. `OFF`는 연결되지 않은 슬롯이고 `--`는 연결은 됐지만 배터리 값을 아직 받지 못한 상태다. 숫자는 마지막으로 받은 유효 잔량이며 `0%`도 표시한다. 연결이 끊기면 이전 잔량 대신 `OFF`를 표시하고 재연결 후 새 값이 올 때까지 `--`를 표시한다. `USB: OK`는 USB HID 준비, `USB: --`는 준비되지 않은 상태이고 `Layer`는 현재 활성 레이어 번호다.

화면 구성은 실제 장치에서 동작한 nice!nano 호환 I²C SDA P0.17/D2, SCL P0.20/D3, 0x3c, 128×64 및 SH1106 호환 경로(열 오프셋 2)를 사용한다. 이는 관찰된 호환 설정이며 실제 칩의 표기나 다른 장치의 배선을 보증하지 않는다. Bongo Cat 이미지는 `englmaxi/zmk-dongle-display`의 LVGL 8 호환 revision `c6c4be11c5ede722086a2a1f355cec9c101b9390`에서 가져왔다. 연결·배터리 상태는 실제 수신과 연결 해제를 구분하기 위해 고정 ZMK의 BLE callback에서 별도 snapshot으로 읽는다.

기본 idle 시간인 30초 동안 입력이 없으면 OLED가 꺼진다. 화면 소등 중에도 USB와 키보드 BLE 연결은 동작하며 키 또는 지원 엔코더 입력으로 화면과 입력이 돌아온다. 배터리나 연결 상태 갱신은 화면을 깨우지 않는다. DYA 전원 설정에 저장된 idle 값에 따라 시간은 달라질 수 있으며, 0이면 자동 idle이 꺼진다. OLED 소등은 동글 종료나 키보드 깊은 절전을 뜻하지 않는다.

Q/S/Z를 동시에 2초 누르면 연결된 키보드가 깊은 절전에 들어가고 동글 OLED가 꺼진다. 기본 키맵에서 오른쪽 엄지 줄의 `MO2`를 누른 채 `.` 위치를 2초 누르는 레이어 2의 기존 깊은 절전 바인딩도 같다. `MO2`를 누른 동안 OLED에 `Layer: 2`가 표시된다. 동글은 USB와 BLE를 유지한다. 깊은 절전은 자동 절전과 달리 일반 키만으로 깨울 수 없다. 돌아오려면 원하는 키보드의 리셋 버튼을 누르고 연결된 뒤 키 또는 지원 엔코더를 조작한다. 한쪽을 먼저 복귀시킨 뒤 다른 쪽을 나중에 리셋해도 된다. 배터리 및 연결 상태 갱신만으로 화면이 켜지지 않는다.

## DYA Studio

[DYA Studio](https://studio.dya.cormoran.works/)에서 USB 연결을 선택하고 동글의 직렬 포트를 선택한다. 동글 모드의 설정은 동글에, 일반 모드의 설정은 왼쪽 Central에 저장되며 자동 이전되지 않는다. 키맵 편집은 즉시 적용되며 `Save`가 영구 저장이다. 시험 전 원래 바인딩을 기록하고 시험 후 복구한다. 설정 초기화는 별도 동의 없이 수행하지 않는다.

| 기능 | 동글 구성 및 저장 계약 | 검증 상태 |
| --- | --- | --- |
| 키맵·레이어 | Studio USB RPC, 편집 즉시 적용, `Save`로 저장 | 최종 후보에서 키 편집·재연결과 LAYER1 편집·실제 입력 확인 |
| 런타임 엔코더 | Studio RPC 활성화; 방향 설정은 선택 시 별도 저장 | 최종 후보에서 양 방향 편집·입력과 원래 음량 기능 복구 확인 |
| 런타임 input processor | Studio RPC 활성화 | 서브시스템 응답 확인; 장치에 processor 없음, 편집·저장 미검증 |
| settings RPC | Studio RPC 활성화; idle/sleep 쓰기는 키맵 Save와 별개로 지연 저장 | 서브시스템 및 전원 설정 읽기 확인; 쓰기 제한은 아래 참고 |
| BLE 호스트 관리 | USB 전용 동글에서 비활성 | 일반 모드 유지, 실기 회귀 미검증 |
| 배터리 이력 | 비활성 | 미지원 |

Studio BLE 연결과 동글의 Bluetooth 호스트 출력은 제공하지 않는다. 위의 확인 상태는 빌드 설정만으로 실기 동작을 보장하지 않는다.

현재 동글 빌드에서는 sleep이 꺼져 있다. 고정된 settings RPC 구현은 idle을 먼저 변경한 뒤 sleep도 변경하려 하므로 요청이 실패해도 idle은 이미 적용되고 60초 뒤 저장될 수 있다. DYA에서 이 전원 설정 쓰기를 사용하지 말고, 실패 응답만으로 기존 값이 유지됐다고 가정하지 않는다. 이 경로의 편집·저장 및 Peripheral 전파는 실기에서 검증하지 않았다.
