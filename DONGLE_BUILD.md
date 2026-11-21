# Sofle 동글 모드 빌드와 설치

> 실기 확인: 사용 중인 동글의 부트로더는 nice!nano로 식별됐으며, 이전 XIAO 대상 빌드는 해당 장치에서 USB로 인식되지 않았다. nice!nano 대상 후보에서 양쪽 문자 입력과 DYA 연결을 확인했다. 장치 식별과 이전 시험의 한계는 [왼쪽 설정 기록](LEFT_KEYBOARD_SETUP_RECORD.md)과 [오른쪽 설정 기록](RIGHT_KEYBOARD_SETUP_RECORD.md)을 참고한다.

> 2026-11-20 사용자 확인: 사용 중인 동글에는 배터리가 이미 장착되어 있다. 무선 동글 모드의 전원 검증에는 이 배터리를 사용하며, 배터리 장착 자체가 USB 없는 PC 연결이나 사용 시간의 검증을 뜻하지는 않는다.

동글 모드는 왼쪽과 오른쪽을 BLE Peripheral로, nice!nano 대상으로 식별된 동글을 Central로 사용한다. 일반 모드와 전환하려면 각 장치를 해당 모드의 펌웨어로 다시 플래시해야 한다. USB 전용 동글 `sofle_dongle`은 컴퓨터로 USB HID와 Studio 직렬 연결을 제공한다. 무선 동글 `sofle_dongle_wireless`의 Phase 1 조작과 실기 검증 절차는 다음과 같다. [설계 결정과 단계별 범위](docs/adr/0002-wireless-dongle-output-selection.md)

## 무선 동글 Phase 1

아래 Windows 빌드 환경에서 무선 동글 UF2를 별도로 생성한다.

```powershell
Set-Location $ws
west build -p always -s zmk/app -d "$ws/build-dongle-wireless" -b nice_nano_v2 -- "-DZephyr_DIR=$zephyr" '-DSHIELD=sofle_dongle_wireless' "-DZMK_CONFIG=$repo/config" "-DZMK_EXTRA_MODULES=$repo" '-DCMAKE_C_FLAGS=-ULV_CONF_PATH' '-DSNIPPET=studio-rpc-usb-uart'
Get-FileHash "$ws/build-dongle-wireless/zephyr/zmk.uf2" -Algorithm SHA256
```

USB 전용 동글 모드에서 좌우가 이미 연결돼 있으면 동글만 무선 UF2로 교체해 시험한다. 복구할 때는 같은 동글에 기존 USB 전용 UF2를 다시 적용할 수 있다. 기본 키맵의 레이어 2에서 다음 키를 사용한다. DYA에서 저장한 바인딩이 해당 위치를 덮으면 그 바인딩이 우선하므로 필요한 위치만 DYA로 수정한다.

| 기본 키 위치 | 동작 |
| --- | --- |
| `1`~`4` | PC 프로필 `0`~`3` 선택 및 BLE 출력 전환 |
| `5`, `E` | 다음, 이전 PC 프로필 선택 및 BLE 출력 전환 |
| `A`, `S` | 각각 USB 출력, 선택한 PC 프로필의 BLE 출력으로 전환 |
| `Q`, `W`를 각각 2초 유지 | 각각 현재 PC 본딩 삭제, 전체 PC 본딩 삭제 |

저장된 선택이 없는 동글은 USB 출력으로 시작한다. 빈 PC 프로필을 선택하면 120초 동안 Windows 11의 Bluetooth 장치 추가에서 `Sofle Dongle`을 등록할 수 있다. 시간이 지나면 같은 프로필을 다시 선택해 재시도한다. PC 본딩을 삭제한 뒤 재등록할 때는 Windows에서도 이전 장치를 제거하고 빈 프로필을 다시 선택한다. 전체 PC 본딩 삭제는 좌우 키보드 본딩을 보존한다.

무선 동글은 PC 프로필과 USB/BLE 출력을 함께 저장해 재부팅 후 복원한다. 기존 펌웨어에서 올린 경우 새 형식으로 처음 저장하기 전까지 기존 선택을 읽는다. BLE 전환 중 이전 PC의 해제 보고서 전송을 확인할 수 없으면 전환을 중단한다. 출력이 유지되면 키를 모두 놓고 프로필을 다시 선택한다.

리뷰 수정 UF2(SHA-256 `9B7AC0A2890D47643AE1B93A945FC4A1F9C539B9077AC73AE643350EE94D832E`) 설치 직후 왼쪽 키보드가 인식되지 않았다. 왼쪽 키보드 리셋 뒤에도 증상이 유지됐으나, 동글을 리셋한 뒤 사용자가 좌우 키보드 모두 정상 인식된 것을 확인했다. 설치 직후 증상의 원인은 확인되지 않았다.

이후 왼쪽 입력 문제가 재발했고 OLED는 `L --`를 표시했다. 고정 ZMK는 분할 키보드 스캔 시작에 실패해도 스캔 중으로 기록해 다음 시도를 건너뛰는 경로가 있었다. 동글 빌드에서는 실패 시 상태를 되돌리고 500ms 뒤 재시도하도록 수정했다. 호스트 시험은 스캔 시작 실패와 재시도를 재현한다. 실제 왼쪽 증상과 이 경로의 연관성은 실기에서 검증해야 한다.

무선 동글은 아래 이벤트의 최신 256건을 플래시에 보관한다. `boot`가 새 부팅의 시작이고 `ms`는 그 부팅의 경과 시간이다. 스캔이 계속 실패할 때는 최초 실패와 이후 분당 한 건을 보관한다. `events_dropped`는 로그 큐에서 누락된 개수이며, 덤프 머리글에 현재 부팅의 플래시 쓰기 오류 건수가 표시된다. 키 입력 내용과 BLE 주소는 기록하지 않는다.

- `usb_state`는 USB 없음 0, 전원 감지 1, HID 상태 2를 기록한다. USB 없이 부팅해 증상을 재현한 뒤, 동글을 리셋하지 않고 USB를 연결해 덤프를 읽는다.
- `host_restored` 또는 `host_default`는 활성 PC 프로필과 출력(USB 0, BLE 1)을, `host_boot_bond`는 그 프로필의 등록 여부(빈 프로필 0, 등록됨 1)를 보여 준다.
- PC 쪽은 광고 모드 변경·오류, 연결 수락·거부·해제, 암호화 수준·실패, 페어링 대기·만료·결과, HID 알림 설정 변경을 기록한다. `host_hid_notify_result`는 키보드 0, 소비자 키 1, 마우스 2별로 첫 알림 제출 결과와 오류 상태 변화를 기록한다. `host_disconnected`의 `code`는 BLE 사유 코드다.
- `host_command_queued`, `host_command_started`, `host_command_result`의 `slot`은 명령 종류다. 0은 프로필 선택, 1·2는 다음·이전 프로필, 3은 전체 삭제, 4는 출력 선택, 5는 PC 연결 처리, 6은 메뉴 열기, 7은 단일 프로필 삭제다. 결과의 `code`는 성공 0 또는 오류 코드다. `host_gate_wait`의 `code`는 입력 및 동작 종료 대기 1, 입력 큐 처리 대기 2, 기존 PC 연결 해제 대기 3이다. `host_gate_open`은 전환 처리가 끝났다는 의미이며, `code`가 1이면 열린 메뉴가 계속 입력을 차단한다.
- 좌우 키보드 쪽은 스캔, BLE 링크, 서비스·특성 탐색 실패, 키 입력 알림 구독 결과, 연결 해제와 물리적 좌우 식별을 기록한다. `split_connected` 이후 `key_ccc_result`가 0이면 해당 슬롯에서 키 입력 알림 구독이 성공했다.

스캔이 8초 뒤에도 진행 중이면 `scan_reports`에 동글이 받은 전체 광고 콜백 수를, `scan_slot`에 미연결 슬롯별 관찰 결과를 한 번 기록한다. `scan_slot`의 `code`는 일반 연결 가능 광고 1, 지향성 연결 광고 2, 기타 광고 4, 슬롯 예약 거부 8을 비트로 합친 값이다. 0이면 그 구간에 동글이 해당 슬롯의 저장된 BLE 주소와 일치하는 광고를 관찰하지 못했다. 슬롯 번호는 기존 `split_side` 기록과 대조한다.

2026-11-20 실기 확인. 로그 UF2(SHA-256 `590CFAA7AF8201AC817FFB0776C407102267F8D51BFD6E8C49CF015EDFE4BB14`) 설치 직후에는 슬롯 1의 오른쪽만 연결됐고, 두 번째 반쪽을 위한 스캔 시작 뒤 왼쪽 연결 시도는 기록되지 않았다. 왼쪽 전원을 껐다 켜자 슬롯 0 연결과 왼쪽 식별 기록이 생겼다. 두 반쪽이 연결된 상태에서 왼쪽만 다시 껐다 켜니 로그용 `COM3`에서 슬롯 0의 끊김(10번, 코드 8), 스캔(11번), 연결 시도(12번), 재연결(13번)을 읽었다. 동글 재부팅 뒤 새 부팅(15번)이 추가됐고, 이전 10~13번은 그대로 남았다. 두 덤프 모두 플래시 쓰기 오류 0건이었다.

무선 동글을 Windows PC에 USB로 연결하면 DYA Studio 포트 외에 로그용 USB 직렬 포트가 표시된다. Windows에서 포트 번호를 확인한 뒤 저장소 루트에서 아래 명령을 실행한다. `COM4`는 예시다. 로그는 최근 기록을 오래된 순서로 읽으며 조회 후에도 보관된다.

```powershell
Get-PnpDevice -PresentOnly -Class Ports
./scripts/read-dongle-log.ps1 -Port COM4
```

사용자가 Shift를 누른 채 P0에서 P1로 전환한 뒤 두 PC의 입력 상태를 확인했다. P0에 Shift가 남지 않았으며, 동글을 재부팅한 뒤 `BLE P1` 표시와 P1 입력이 함께 복원됐다. 첫 삭제 경합 시험에서는 P0이 `Paired`로 남았다. 비활성 프로필 삭제를 현재 PC의 HID 전송과 분리한 UF2(SHA-256 `9AF9EB73F3CC6FD2A18656D747C43E2780C51D974CA40482D504BA8722273908`)로 재시험했다. 사용자가 P1에서 삭제 키 `Q`를 누른 채 P0 전환을 요청하고 모든 키를 놓은 뒤, OLED에서 P1은 `Empty`, P0은 `Connected`로 표시되는 것을 확인했다. 전환과 삭제가 겹쳐도 처음 지정한 P1의 본딩만 삭제되는 실기 시험을 통과했다.

실기 인수에서는 Windows 11 PC 두 대에 각각 등록한 뒤 다음을 확인한다.

1. 좌우 키보드를 동시에 연결하고 양쪽 문자·수정키·마우스 입력을 확인한다. Shift 또는 마우스 버튼을 누른 채 프로필을 전환하고, 해제 후 새 입력부터 선택한 PC에 전달되는지 확인한다.
2. 두 PC를 여러 번 전환하고 재부팅 뒤 마지막 선택을 확인한다. USB 연결 중 선택 PC의 BLE가 끊겨도 USB로 입력이 이동하지 않는지 확인한다. USB 출력을 명시적으로 선택한 상태에서는 PC의 BLE 재연결 중 USB 입력이 계속되는지 확인한다.
3. 빈 프로필에서 페어링 시간 초과와 재시도를 확인한다. PC 본딩 한 개 및 전체 삭제 후 좌우가 계속 동작하는지 확인한다. 전체 삭제는 시험용 PC 본딩을 등록한 상태에서 수행한다.
4. BLE 출력으로 키를 입력하는 상태에서 동글을 USB로 연결한다. DYA Studio에서 USB 직렬 포트를 열어 키맵을 읽고, 양쪽 키가 계속 선택한 BLE PC에 전달되는지 확인한다. Studio의 USB 직렬 연결은 HID 출력 선택과 독립적으로 동작한다.

2026-11-20 실기 확인. 수정 UF2(SHA-256 `20AF767BAF598CADB7095458C128FCE997968C02F5C6DC962194AC1E21411357`)를 nice!nano 동글에 설치했다. DYA Studio는 USB 직렬 `COM3`로 연결해 키맵과 레이어 0~4를 읽었다. 사용자가 다른 PC로 BLE 키 입력이 전달되는 것을 확인했고, 입력 뒤에도 DYA Studio 연결이 유지됐다.

자동 시험의 범위와 명령은 [Phase 1 호스트 시험](tests/dongle-host/README.md)에 있다.

## USB 전용 동글과 좌우 키보드 빌드

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
3. 양쪽 문자, 수정키 누름과 해제, 레이어 전환, 마우스와 왼쪽 엔코더 양방향을 확인한다. USB 전용 동글에서는 Bluetooth 호스트 선택·삭제와 BLE 출력 전환 위치가 무동작이다.

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

무선 동글의 출력 메뉴를 열려면 레이어 2를 누른 채 오른쪽 엄지 영역의 위치 59를 누르고 두 키를 놓는다. 현재 DYA 키맵에서 이 키는 `]`이며, 레이어 2 키(`LT LAYER2 Space`, 위치 60)의 바로 왼쪽이다. 레이어 2의 `.` 위치는 기본 레이어의 입력을 따른다. 저장된 DYA의 레이어 2 바인딩이 위치 59를 덮으면 DYA에서 레이어 2의 해당 위치에 `host_menu` 동작을 지정한다.

- 메인 메뉴에 프로필 0~3과 각각의 상태가 표시된다. 화면의 `Center U/D/OK`는 Sofle 두 반쪽 사이의 중앙 방향키(위·아래)와 그 가운데 버튼을 뜻한다. 위·아래로 프로필을 고르고 가운데 버튼으로 선택한다. `Center L: Back`은 중앙 왼쪽 방향키다. 왼쪽 `W/S/E`도 같은 이동·선택에 사용할 수 있다.
- 등록된 프로필은 `Connect`, 빈 프로필은 `Start pairing`을 선택한다. 선택 후 상태 화면으로 돌아간다.
- `Delete bond`는 선택한 프로필의 PC 본딩, `Clear all PCs`는 모든 PC 본딩을 삭제한다. 두 동작 모두 확인 화면의 기본 선택은 `Cancel`이다. 실행하려면 `Delete`를 선택한 뒤 Enter를 누른다.
- Esc 또는 `Q`는 한 단계 뒤로 가고 프로필 목록에서 누르면 메뉴를 닫는다. 30초 동안 키 입력이 없어도 메뉴가 닫힌다. 메뉴 조작키는 Sofle의 물리 위치를 사용하며 메뉴가 열려 있는 동안 PC로 키나 엔코더 입력이 전달되지 않는다.

실기 확인. 중앙 키 안내 UF2(SHA-256 `17EF2E5F13AC7F7C687989124D3CC6D35007D4F4761E7C390964F6CB8ADD92B8`)를 설치했다. DYA에서 레이어 2의 위치 59만 `host_menu`로 저장해 재연결 후 읽어 확인했다. 사용자가 레이어 2 키 바로 왼쪽 키로 메뉴를 열고, 중앙 위·아래 키로 이동, 가운데 버튼으로 선택, 중앙 왼쪽 키로 목록 복귀가 되는 것을 확인했다.

출력 메뉴에서 가운데 위·아래 키로 `USB output`에 이동하고 가운데 버튼으로 선택하면 USB로 전환한다. USB를 선택해도 현재 PC 프로필은 유지된다. 메뉴를 열거나 USB로 전환할 때 이전 BLE 연결에서 키 해제 보고서가 실패하면 그 연결을 먼저 끊는다. 연결 해제를 요청할 수 없거나 4.5초 안에 확인하지 못하면 명령을 종료하고 현재 출력을 유지한다. USB에서 BLE로 바꿀 때는 USB 키 해제 전송이 완료돼야 전환한다. USB가 절전 상태라면 깨운 다음 다시 시도한다. BLE로 돌아가려면 같은 메뉴에서 사용할 PC 프로필을 선택한 뒤 `Connect`를 누른다.

무선 동글의 Phase 2 화면은 `USB P0` 또는 `BLE P0`처럼 선택된 HID 출력과 현재 PC 프로필을 표시한다. `Switching`은 출력 전환 중이라는 뜻이며, USB 출력 선택 상태에서 USB HID를 사용할 수 없으면 상태 줄에 `/U-`가 붙는다.

| PC 프로필 상태 | 의미 |
| --- | --- |
| `Empty` | 등록 정보가 없는 빈 프로필 |
| `Pairing` | 빈 프로필을 선택해 페어링을 기다리는 중 |
| `Paired` | 등록 정보는 있으나 보안 BLE 연결이 아직 준비되지 않음 |
| `Connected` | 등록한 PC와 보안 BLE 연결이 준비됨 |
| `Pair failed` | 페어링 시도가 실패했으며 남은 시간 동안 다시 시도할 수 있음 |
| `Timeout` | 120초가 지나 페어링이 종료됨. 빈 프로필을 다시 선택해 재시도 |

무선 화면의 `D`는 동글 배터리, `L`과 `R`은 좌우 키보드 배터리다. `D`의 퍼센트는 nice!nano의 전압 기반 잔량 추정치다. 배터리의 정격 용량(mAh)은 이 센서로 확인할 수 없다. 동글 첫 측정이나 좌우 잔량 수신을 기다리는 중에는 `--`로 표시한다.

아래의 `LEFT`, `RIGHT`, `USB: OK`, `Layer` 문구는 USB 전용 화면 기준이다. 무선 화면도 좌우 배터리 막대와 저전압 경고, 활성 레이어, 고양이 애니메이션 및 화면 절전을 유지한다.

2026-11-20 Phase 2 실기 확인. 무선 UF2(SHA-256 `47A1073DFB1EAA653668AC2182A343E5F8A098AF8CF44C6E3E29D57214EF3461`) 설치 뒤 Windows에서 `COM3` 재인식을 확인했다. 설치 직후 왼쪽 키보드가 인식되지 않았으나 이후 사용자가 정상 동작을 확인했다. 사용자가 USB/BLE 출력 및 프로필과 연결 상태, 동글·좌우 잔량, 레이어 변화, 타이핑 애니메이션이 정상이라고 보고했다. 빈 프로필 2를 선택했을 때 `Pairing`이 표시됐고, 대기 중 OLED가 자동으로 꺼졌다. 키 입력 후 OLED가 다시 켜지면서 `BLE P2 Timeout`이 표시된 것을 사용자가 확인했다. 별도의 `Pair failed` 표시 경로는 OLED 시뮬레이터에서 확인했다.

동글 화면은 `LEFT`, `RIGHT`의 잔량을 숫자와 막대로 표시한다. BLE 연결 슬롯은 물리적인 좌우가 아니므로, 각 반쪽에서 키를 처음 누를 때 입력 위치로 해당 연결의 좌우를 식별한다. 연결 후 양쪽 키를 각각 한 번씩 누른다. 아직 식별하지 못했거나 연결되지 않았거나 배터리 값을 받지 못한 쪽은 `--`로 표시한다. 연결이 끊기면 식별과 이전 값은 지워지며 재연결 후 다시 키를 눌러야 한다. 숫자는 마지막으로 받은 유효 잔량이며 `0%`도 표시한다. 잔량이 20% 이하인 쪽의 `!`가 깜박인다. 입력 시 고양이가 앞발을 움직이고, 입력이 없을 때에도 가끔 모습이 바뀐다. `USB: OK`는 USB HID 준비, `USB: --`는 준비되지 않은 상태이고 `Layer`는 현재 활성 레이어 번호다. OLED 화면은 시뮬레이터로 확인했고, 사용자는 최종 동글 펌웨어를 설치해 정상 작동한다고 확인했다. 20% 이하 실기기 경고는 별도 확인하지 않았다.

2026-11-10 실기 비교에서 최초 OLED 후보는 일반 오른쪽 Shift와 `MT RShift J`가 동작하지 않았다. 키 입력 관찰을 BLE 콜백에서 제거한 비교 후보에서는 일반 오른쪽 Shift가 정상 입력됐고, 입력 전달 후 화면 상태를 갱신하도록 바꾼 수정 후보에서는 사용자가 두 Shift의 대문자 입력을 확인했다. 수정 후보의 UF2 SHA-256은 `090DB51BB7432289768D0C69DF364251ADA684E679E83F3BA26FD791F026F5A6`이다. 정확한 내부 실패 기전은 확인하지 못했다.

화면 구성은 실제 장치에서 동작한 nice!nano 호환 I²C SDA P0.17/D2, SCL P0.20/D3, 0x3c, 128×64 및 SH1106 호환 경로(열 오프셋 2)를 사용한다. 이는 관찰된 호환 설정이며 실제 칩의 표기나 다른 장치의 배선을 보증하지 않는다. Bongo Cat 이미지는 `englmaxi/zmk-dongle-display`의 LVGL 8 호환 revision `c6c4be11c5ede722086a2a1f355cec9c101b9390`에서 가져왔다. 연결·배터리 상태는 실제 수신과 연결 해제를 구분하기 위해 고정 ZMK의 BLE callback에서 별도 snapshot으로 읽는다.

기본 idle 시간인 30초 동안 입력이 없으면 OLED가 꺼진다. 화면 소등 중에도 USB와 키보드 BLE 연결은 동작하며 키 또는 지원 엔코더 입력으로 화면과 입력이 돌아온다. 배터리나 연결 상태 갱신은 화면을 깨우지 않는다. DYA 전원 설정에 저장된 idle 값에 따라 시간은 달라질 수 있으며, 0이면 자동 idle이 꺼진다. OLED 소등은 동글 종료나 키보드 Deep Sleep을 뜻하지 않는다.

OLED 갱신은 별도 작업 큐에서 수행한다. 화면과 키 입력을 같은 시스템 작업 큐에서 처리하던 후보와 비교했을 때, 사용자는 별도 큐 시험 후보의 입력 반응이 확실히 좋아졌고 OLED도 정상이라고 확인했다. 이는 개별 장치의 체감 비교이며 지연 시간의 정량 측정은 아니다.

기본 키맵에서 Q/S/Z 조합에는 별도 동작이 없다. USB 전용 동글의 레이어 2 `.` 위치는 무동작이고, 무선 동글의 같은 위치는 기본 레이어 입력을 따른다. 키보드 좌우는 기본적으로 60분 동안 입력이 없으면 Deep Sleep에 들어가며 키로 깨울 수 있다. 복귀와 BLE 재연결 중 첫 입력은 전달되지 않을 수 있다. 저장된 DYA 설정에 따라 기본값과 실제 동작이 다를 수 있다. 15초 시험 빌드에서 좌우 연결 해제와 키 입력 후 복귀를 확인했으며, 60분 정상 빌드의 진입·복귀는 별도 확인하지 않았다. 자세한 내용은 [실기 기록](DEEP_SLEEP_TEST_RECORD.md)을 참고한다.

## DYA Studio

[DYA Studio](https://studio.dya.cormoran.works/)에서 USB 연결을 선택하고 동글의 직렬 포트를 선택한다. 동글 모드의 설정은 동글에, 일반 모드의 설정은 왼쪽 Central에 저장되며 자동 이전되지 않는다. 키맵 편집은 즉시 적용되며 `Save`가 영구 저장이다. 시험 전 원래 바인딩을 기록하고 시험 후 복구한다. 설정 초기화는 별도 동의 없이 수행하지 않는다.

| 기능 | 동글 구성 및 저장 계약 | 검증 상태 |
| --- | --- | --- |
| 키맵·레이어 | Studio USB RPC, 편집 즉시 적용, `Save`로 저장 | 이전 #4 후보에서 키 편집·재연결과 LAYER1 편집·실제 입력 확인 |
| 런타임 콤보 | 무선 동글에서 최대 8개, `Save`로 동글에 저장 | DYA Studio 2026.09.20.0에서 추가와 삭제를 각각 저장한 뒤 동글을 재부팅해 확인 |
| 런타임 엔코더 | Studio RPC 활성화; 방향 설정은 선택 시 별도 저장 | 이전 #4 후보에서 양 방향 편집·입력과 원래 음량 기능 복구 확인 |
| 런타임 input processor | Studio RPC 활성화 | 서브시스템 응답 확인; 장치에 processor 없음, 편집·저장 미검증 |
| settings RPC | Studio RPC 활성화; idle/sleep 쓰기는 키맵 Save와 별개로 지연 저장 | 서브시스템 및 전원 설정 읽기 확인; 쓰기 제한은 아래 참고 |
| BLE 호스트 관리 | USB 전용 동글에서 비활성 | 일반 모드 유지, 실기 회귀 미검증 |
| 배터리 이력 | 비활성 | 미지원 |

Studio는 두 동글 모드에서 USB 직렬로 연결한다. PC에 Bluetooth로 키를 입력하는 기능은 무선 동글에서 사용할 수 있다.

Runtime Combo를 포함한 펌웨어를 설치한 무선 동글에 연결한 뒤, DYA Studio의 **Macro&Combo → Combos**에서 `+`로 콤보를 추가한다. 키보드 미리보기에서 두 개 이상의 키를 고르고 실행할 동작을 지정한 다음 `Save`를 누른다. 추가한 콤보를 삭제할 때는 해당 항목을 열어 `Delete`를 누른 뒤 `Save`를 누른다.

현재 동글 빌드에서는 sleep이 꺼져 있다. 고정된 settings RPC 구현은 idle을 먼저 변경한 뒤 sleep도 변경하려 하므로 요청이 실패해도 idle은 이미 적용되고 60초 뒤 저장될 수 있다. DYA에서 이 전원 설정 쓰기를 사용하지 말고, 실패 응답만으로 기존 값이 유지됐다고 가정하지 않는다. 이 경로의 편집·저장 및 Peripheral 전파는 실기에서 검증하지 않았다.

최종 후보 빌드와 검증 범위는 [통합 빌드 기록](ISSUE8_BUILD_EVIDENCE.md)을 참고한다.
