# 통합 후보 Windows 빌드 기록 (2026-11-08)

펌웨어 소스 revision `b9ff897c066d086a1af8aa6d34161cb15e209c3a`에서 Windows 네이티브 clean 빌드를 각각 별도 디렉터리에 실행했다. 아래는 성공 종료(모두 exit 0)한 산출물이다. 재현 명령은 [일반 모드](WINDOWS_BUILD.md)와 [동글 모드 및 초기화](DONGLE_BUILD.md)에 있다. 실제 동글 및 좌우 부트로더는 모두 `nRF52840-nicenano`로 식별됐으므로 이 세 장치는 같은 nice!nano 초기화 파일을 사용한다. XIAO 초기화 파일은 다른 XIAO 하드웨어용이며 이 동글에 사용하지 않는다.

| 파일 | 보드 / 용도 | Flash / RAM bytes | SHA-256 |
| --- | --- | --- | --- |
| normal-left.uf2 | nice_nano_v2 / 일반 Central | 409664 / 102706 | E82FF12FB5F1038C5B9A8422551859989197BF1705754E326BDD18E51B6F84E8 |
| normal-right.uf2 | nice_nano_v2 / 일반 Peripheral | 276952 / 63660 | 88F3B0B602B2E6A30D7E1523E4A7003DB59A0E0BAC03937BF513AE2BCA3D2629 |
| dongle-left.uf2 | nice_nano_v2 / 동글 왼쪽 Peripheral | 277704 / 63692 | 340A1083E7BC583D896EAA1C6E68E8931FBE90CBF72452DD7F98E1352FF145F3 |
| dongle-right.uf2 | nice_nano_v2 / 동글 오른쪽 Peripheral | 276952 / 63660 | 88F3B0B602B2E6A30D7E1523E4A7003DB59A0E0BAC03937BF513AE2BCA3D2629 |
| dongle-central-nicenano.uf2 | nice_nano_v2 / 동글 USB Central | 358260 / 81018 | 1815D1D96906EE162FFFC948905020CC67D0C9E3EB7061212AE2BC4C9BC708E3 |
| settings-reset-nicenano.uf2 | nice_nano_v2 / 설정 삭제 전용 | 46292 / 11560 | 283C873FFF1D0EE7A6F1D4F72C33BD468DB4E5EAD68F0224942EB15F34EBC5B0 |
| settings-reset-xiao.uf2 | seeeduino_xiao_ble / 설정 삭제 전용 | 49632 / 11648 | 4A0F89ED7AE7A2D5800625562190436CD52860C16CD3220C0C2F1B9AC74612E7 |

원본 UF2와 `<이름>.log`, 빌드별 `build-<이름>/zephyr/zephyr.dts`, `build-<이름>/zephyr/.config`, 요약 `config-summary.txt`, 해시 및 종료 코드 `results.txt`, `west-list.txt`, `tools.txt`는 `C:/temp/issue8-final/`에 있다. 이 경로는 빌드한 Windows 시스템의 로컬 증거이며 배포 URL이 아니다. 오른쪽 일반·동글 파일은 동일 해시다. 생성 설정은 두 Central의 split Central 및 Studio, 세 Peripheral의 split, 두 초기화 빌드의 설정 초기화를 보여준다. 동글 DTS는 SH1106 호환과 열 오프셋 2를 사용한다.

Python 3.12.10, west 1.5.0, CMake 3.27.9, Ninja 1.13.2.git.kitware.jobserver-pipe-1, Zephyr SDK 0.16.8; ZMK `4493783ef88ce2e653bf8217c92ee17140df71e3`, Zephyr `dacab4875df72109b96cc8977547a0dc04875bcd`. 나머지 고정 의존성은 [lock](requirements/west-active-lock.yml)과 위 west 목록을 확인한다.

동글 좌우 및 Central의 해시는 #7에서 실제 검증한 후보와 같다. #7의 Soft Off·복귀 관찰은 [#7 기록](https://github.com/kalstein00/zmk-sofle/issues/7#issuecomment-6721019614)에 있으며 배터리 알림 도착 시점은 미검증이다. 사용자의 명시적 결정으로 세 장치 설정 초기화는 수행하지 않으며 전체 동글 초기화 후 재페어링은 실기 미검증으로 남긴다. 기존 본딩이 없는 최초 설치도 미검증이다. 동일 후보에서 DYA 편집·저장부터 절전·복귀까지 아래 순차 실기 여정을 확인했다. 일반 모드 실기와 모드 전환은 [#9](https://github.com/kalstein00/zmk-sofle/issues/9)에서 검증한다. 빌드 성공만으로 실기 동작을 입증하지 않는다.


| 기능 | 상태와 근거 |
| --- | --- |
| 빌드·역할 분리 | 7개 빌드 통과. 초기 역할·구성 확인은 [#2](https://github.com/kalstein00/zmk-sofle/issues/2#issuecomment-6701544848), [#3](https://github.com/kalstein00/zmk-sofle/issues/3#issuecomment-6707478897). 일반 모드 실기 #9 미검증. |
| 양쪽 입력·페어링 | #7과 동일한 3개 UF2에서 실제 양쪽 입력 및 재연결 확인. 전체 초기화 후 재페어링은 미검증. [#3](https://github.com/kalstein00/zmk-sofle/issues/3#issuecomment-6707478897), [#7](https://github.com/kalstein00/zmk-sofle/issues/7#issuecomment-6721019614). |
| DYA 키맵·레이어·엔코더 | [#4](https://github.com/kalstein00/zmk-sofle/issues/4#issuecomment-6719994558) 이전 후보에서 실기 편집·입력·저장 확인. 최종 후보에서 Q→W 즉시 입력·Save·동글 USB 재연결 후 W 유지와 Q 복구·Save·입력 확인. settings RPC 쓰기 및 input processor 저장은 미검증. |
| OLED·배터리·슬롯 | [#5](https://github.com/kalstein00/zmk-sofle/issues/5#issuecomment-6720598584) 이전 후보에서 사용자 화면 관찰. 0% 및 미수신은 호스트 시험, 실제 잔량과 독립 비교 미검증. 최종 후보에서 화면 및 복귀는 #7 실기 확인. |
| Idle | [#6](https://github.com/kalstein00/zmk-sofle/issues/6#issuecomment-6720658066) 이전 후보에서 소등·키/엔코더 복귀 실기 확인. 최종 후보 순차 여정에서 소등과 키 입력·화면 복귀 확인. 배터리 알림의 복귀 전 도착 시점 미검증은 사용자 수용. |
| Soft Off | [#7](https://github.com/kalstein00/zmk-sofle/issues/7#issuecomment-6721019614)의 같은 3개 UF2에서 Q/S/Z, MO2+점, 양쪽 리셋 순서, 한쪽 연결 및 입력 복귀 확인. |
| 복구·순차 통합 | 세 장치의 실제 초기화·재페어링과 최초 설치는 사용자 결정에 따라 미검증. DYA 편집 → 저장 → 전원 재연결 → OLED → Idle·Soft Off 및 복귀는 아래와 같이 실기 확인. |

사진과 자동 E2E는 수행하지 않았으며 사용자 지시에 따라 실제 장치의 수동 관찰을 사용했다. 과거 후보와 최종 후보가 다른 항목은 과거 결과를 최종 후보 통과로 간주하지 않는다.

## 최종 후보 통합 실기 여정

사용자가 DYA COM3에서 LAYER0의 물리 Q가 `q`임을 확인하고 `W`로 편집한 뒤 Save 전에 물리 Q에서 `w` 입력을 확인했다. Save 후 동글 USB를 뺐다 다시 연결해도 `w`가 입력됐으며, Q를 `q`로 되돌려 Save한 뒤 실제 `q` 입력을 확인했다. 이후 OLED 양 슬롯 잔량과 `USB: OK`, 입력 중단 후 자동 소등 및 키 입력과 화면 복귀를 확인했다. Q/S/Z를 2초 눌러 Soft Off 후 좌우를 각각 리셋해 OLED와 양쪽 입력이 돌아왔다. 이 여정에서 Soft Off 중 USB 유지의 별도 관찰은 없으며 동일 바이너리의 이전 [#7 PnP 확인](https://github.com/kalstein00/zmk-sofle/issues/7#issuecomment-6721019614)을 재사용한다. 이 여정은 설정 초기화나 새 본딩 상태를 포함하지 않았다. 자동 E2E와 사진은 사용자 수용에 따라 수행하지 않았고 사용자의 직접 관찰을 기록했다.
