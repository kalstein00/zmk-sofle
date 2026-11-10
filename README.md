# ZMK Sofle 펌웨어

이 저장소는 [a741725193/zmk-sofle](https://github.com/a741725193/zmk-sofle)를 fork해 Sofle 키보드의 동글 모드와 사용자 설정을 추가한 프로젝트입니다. GitHub에 표시된 최초 원본은 [tokyo2006/zmk-config-sofle](https://github.com/tokyo2006/zmk-config-sofle)입니다.

## 사용 모드

| 모드 | 구성 | 컴퓨터 연결 |
| --- | --- | --- |
| 일반 모드 | 왼쪽 키보드가 Central, 오른쪽 키보드가 BLE Peripheral | 왼쪽 키보드의 USB 또는 Bluetooth |
| 동글 모드 | OLED 동글이 Central, 양쪽 키보드가 BLE Peripheral | 동글의 USB |

모드를 바꾸려면 각 장치에 해당 모드의 펌웨어를 설치해야 합니다. 동글 모드에서는 컴퓨터로 Bluetooth 출력을 제공하지 않습니다. 자세한 구성과 복구 방법은 [동글 빌드와 설치](DONGLE_BUILD.md)를 참고하세요.

## 펌웨어 설치

[GitHub Releases](https://github.com/kalstein00/zmk-sofle/releases)에서 동글 모드용 펌웨어를 받을 수 있습니다. 릴리즈의 세 UF2 파일은 각각 동글, 왼쪽 키보드, 오른쪽 키보드용입니다. 장치를 부트로더 모드로 전환한 뒤 해당 파일을 복사하세요.

현재 양쪽 키보드가 동글에 정상 연결되어 있다면, 릴리즈에 좌우 펌웨어가 추가됐다는 이유만으로 다시 설치할 필요는 없습니다. 설정 초기화 펌웨어는 일반 사용 펌웨어와 별도이며 저장된 설정과 BLE 본딩을 지웁니다. 설치 순서와 복구 절차는 [동글 빌드와 설치](DONGLE_BUILD.md)를 확인하세요.

## 키맵과 탭·홀드

기본 키맵은 [`config/eyelash_sofle.keymap`](config/eyelash_sofle.keymap)에 있습니다. DYA Studio에서 저장한 실제 키맵은 기본 키맵과 다를 수 있습니다.

[DYA Studio](https://studio.dya.cormoran.works/)에서 키맵을 변경할 수 있습니다. 동글 모드에서는 동글을 USB로 연결하고, 일반 모드에서는 왼쪽 Central을 연결합니다. 편집한 키맵은 `Save`로 저장하며, 두 모드의 저장 설정은 자동으로 이전되지 않습니다.

키맵에 mod-tap 또는 layer-tap을 지정하면 [`config/hold_tap.dtsi`](config/hold_tap.dtsi)의 설정을 사용합니다.

| 동작 | 탭 판정 시간 | 판정 방식 | quick-tap |
| --- | --- | --- | --- |
| Mod-tap | 170ms | `tap-preferred` | 0ms |
| Layer-tap | 170ms | `balanced` | 0ms |

Mod-tap은 빠른 타이핑 중 수정키 오인식을 줄이도록 설정되어 있습니다. 따라서 QMK의 `PERMISSIVE_HOLD`와 모든 상황에서 동일하게 판정되지는 않습니다. 기본 키맵의 일반 키와 `&mo` 레이어 키에는 이중 기능 판정이 적용되지 않습니다.

## 동글 OLED와 절전

- 동글 OLED는 좌우 배터리 잔량, USB 상태, 현재 레이어와 고양이 애니메이션을 표시합니다. 배터리가 20% 이하이면 해당 줄의 `!`가 깜박입니다.
- BLE 연결 슬롯은 좌우에 고정되지 않습니다. 연결 후 양쪽에서 키를 각각 한 번 누르면 화면이 입력 위치로 좌우를 식별합니다. 식별 전이나 잔량 정보가 없을 때는 `--`로 표시합니다.
- 기본적으로 동글 OLED는 30초 동안 입력이 없으면 꺼지고, 키 또는 지원 엔코더 입력으로 다시 켜집니다. 좌우 키보드는 기본적으로 60분 동안 입력이 없으면 Deep Sleep에 들어갑니다. 저장된 DYA 설정에 따라 기본값과 실제 동작이 다를 수 있습니다.

동글 화면, 절전 및 복구의 상세 설명과 검증 범위는 [동글 빌드와 설치](DONGLE_BUILD.md)를 참고하세요.

## 직접 빌드와 릴리즈

- [Windows 일반 모드 빌드](WINDOWS_BUILD.md)
- [동글 모드 빌드와 설치](DONGLE_BUILD.md)
- [활성 의존성 버전 목록](requirements/west-active-lock.yml)

펌웨어 릴리즈에는 저장소 전용 [`$sofle-release`](.agents/skills/sofle-release/SKILL.md) 스킬을 사용합니다. 같은 소스 버전의 동글·왼쪽 키보드·오른쪽 키보드 UF2 세 개와 SHA-256 체크섬을 함께 준비합니다.
