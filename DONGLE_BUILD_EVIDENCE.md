# 동글 모드 빌드 증거 (2026-11-07)

기준은 `fcff109b604ac54397ab13a324f62653726b8aa1`이며 후보는 이 문서와 함께 제출한 #3 변경이다. Windows 네이티브 PowerShell에서 [빌드 안내](DONGLE_BUILD.md)의 명령을 사용했다. Python 3.12.10, west 1.5.0, CMake 3.27.9, Ninja 1.13.2 및 Zephyr SDK 0.16.8을 사용했으며 고정 의존성은 [기존 빌드 증거](WINDOWS_BUILD_EVIDENCE.md)와 같다. 다섯 빌드는 각각 깨끗한 별도 디렉터리에서 종료 코드 0과 새 UF2를 확인했다.

| 대상 | 보드 / 역할 | Flash / RAM | UF2 SHA-256 |
| --- | --- | --- | --- |
| `dongle-left.uf2` | nice_nano_v2 / Peripheral | 277704 / 63692 B | `340A1083E7BC583D896EAA1C6E68E8931FBE90CBF72452DD7F98E1352FF145F3` |
| `dongle-right.uf2` | nice_nano_v2 / Peripheral | 276952 / 63660 B | `88F3B0B602B2E6A30D7E1523E4A7003DB59A0E0BAC03937BF513AE2BCA3D2629` |
| `dongle-central-xiao.uf2` | seeeduino_xiao_ble / Central | 225476 / 51564 B | `8C83E3FA03F3BED8C822EEFDDC5C5E673EECCFD5A1A50920F60BC08BCD347013` |
| 일반 왼쪽 | nice_nano_v2 / Central | 409664 / 102706 B | `E82FF12FB5F1038C5B9A8422551859989197BF1705754E326BDD18E51B6F84E8` |
| 일반 오른쪽 | nice_nano_v2 / Peripheral | 276952 / 63660 B | `88F3B0B602B2E6A30D7E1523E4A7003DB59A0E0BAC03937BF513AE2BCA3D2629` |

로그는 각각 `C:/temp/zmk3-left-final.log`, `C:/temp/zmk3-right-final.log`, `C:/temp/zmk3-central-final.log`, `C:/temp/zmk3-normal-left.log`, `C:/temp/zmk3-normal-right.log`에 있다. 로그는 세 동글 shield와 일반 좌우 shield가 이 저장소에서, `nice_view`가 ZMK에서 선택됐음을 보여준다. 생성된 `.config`에서 동글 좌우는 split Peripheral이고 Central은 split Central, 두 Peripheral 및 USB를 사용한다. 일반 왼쪽은 기존 Central과 단일 Peripheral 설정이고 일반 오른쪽은 Peripheral이다. 동글의 생성된 devicetree에서 지정된 호스트 관리 바인딩은 `&none`이고, 일반 모드에는 원래 바인딩이 남는다. ELF 역어셈블리에서 동글의 광고 시작 호출이 `__wrap_bt_le_adv_start`로 연결된 것을 확인했다. 이는 실제 무선 광고 부재나 USB 전송을 관찰한 결과가 아니다.

[입력 시뮬레이션](tests/sofle-input/README.md)은 좌우의 Q/Y 위치를 겹쳐 누르고 각각 해제했을 때의 HID 상태를 확인했다. 용량 1의 기준 시험은 두 번째 입력이 빠져 실패했고 (`/home/kalstein/zmk3-evidence/sofle-red-final.log`, exit 1), 동글 키맵과 Central 설정을 포함하는 시험은 통과했다 (`/home/kalstein/zmk3-evidence/checked-in-green.log`, exit 0). 시뮬레이션 호스트는 BLE이므로 동글의 USB 출력이나 광고 차단을 증명하지 않는다. 센서, 연결 해제·재연결 및 실기기 테스트는 수행하지 않았다.

준비 과정에서 POSIX 시험은 pinned ZMK의 settings 조건부 컴파일 및 초기화 순서 때문에 `CONFIG_SETTINGS=y`, `CONFIG_SETTINGS_NONE=y`, `CONFIG_BT_SETTINGS=y`가 필요했다. 이러한 설정은 시험 fixture에만 적용했다. Central 첫 빌드는 physical layout의 필수 kscan 부재로 실패했으며, ZMK의 settings-reset 선례처럼 입력 이벤트 없는 kscan을 추가한 뒤 성공했다. 좌우 빌드에는 기존 `ext_power` devicetree 매크로 재정의 경고가 남는다.
