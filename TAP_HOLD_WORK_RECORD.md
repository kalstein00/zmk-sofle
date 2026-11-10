# 동글 화면과 타건감 작업 기록

기록일: 2026-11-10. 아래 내용은 당시 작업 기록이다. 최신 상태는 바로 아래 갱신을 참고한다.

## 2026-11-11 갱신

- 사용자가 일반 키 입력도 밀리는 느낌을 보고했다. BLE 이벤트 처리와 OLED 갱신이 시스템 작업 큐를 공유하는 것을 확인하고, 다른 설정은 같은 상태에서 화면만 별도 큐로 옮긴 시험 펌웨어 `C:\temp\zmk-display-queue-probe\zephyr\zmk.uf2`와 직전 `C:\temp\zmk-layer-quick-probe\zephyr\zmk.uf2`를 비교했다. 사용자는 별도 큐 버전의 반응이 확실히 좋아지고 OLED도 정상이라고 확인했다. 정량 지연 측정은 하지 않았으므로 내부 지연의 정확한 원인이나 크기는 확정하지 않았다.
- 이 결과에 따라 `config/sofle_dongle.conf`에 전용 디스플레이 작업 큐를 적용했다. 후보 `C:\temp\zmk-dongle-display-dedicated\zephyr\zmk.uf2`의 SHA-256은 `0665FDE5D0526D2B808D6B8B5376BDDC3563F6099609AA1621614B4F77C6500`이다. 이 후보는 시험 후보와 달리 QMK 백업에 맞춰 layer-tap quick-tap 0을 사용하며, 사용자는 키 입력이 정상이라고 확인했다. 이후 사용자의 요청으로 OLED `LEFT` 뒤 공백을 늘려 양쪽 잔량 숫자를 정렬했다. 정렬 후보는 `C:\temp\zmk-dongle-aligned-final\zephyr\zmk.uf2`, SHA-256 `14038D0C20159904D9C91564711BBF487BD46C1B08002C667464FCC88717E145`이며 빌드와 LVGL 시뮬레이터에서 확인했다. 사용자는 이 정렬 후보를 동글에 설치하고 정상 작동한다고 확인했다.
- 빠른 Space 단독 탭이 누락된다는 보고에 대해 ZMK native 테스트 `tests/hold-tap/layer-rapid-taps/`는 30ms 누름·30ms 간격 10회를 모두 출력했다. 이 테스트는 물리 스캔과 BLE를 포함하지 않으며 실기기 누락이 없다는 뜻은 아니다.
- 이후 사용자가 QMK에 맞춘 후보에서도 빠른 타이핑 중 Ctrl 오인식을 보고하여 mod-tap만 `tap-preferred`로 바꿨다. mod-tap 170ms 및 quick-tap 0과 layer-tap의 `balanced` / 170ms / quick-tap 0은 유지했다. 이 mod-tap은 의도적으로 QMK permissive hold와 다르다. 동글 후보: `C:\temp\zmk-vial-tap-preferred-170\zephyr\zmk.uf2`, SHA-256 `465DC25F2FF8ED4EA7DCD55961A2975BFA12B5B4E710883412AC88B4946E4EE9`. ZMK native 테스트에서 빠른 중첩 탭, 170ms 이후의 홀드, 롤 입력, mod-tap 및 layer-tap의 연속 탭 후 홀드를 확인했다. 실기기 시험은 요청했으나 아직 결과를 받지 못했다.
- 사용자가 최신 Vial 백업을 제공했다. 그 백업의 설정은 tapping term 170ms, permissive hold 켜짐, hold-on-other-key-press 꺼짐, retro tapping 꺼짐, quick tap term 0, chordal hold 꺼짐, flow tap term 0이다. 아래 11월 10일의 retro tapping 켜짐 기록은 당시 백업에 대한 것이다.
- 사용자의 동일 설정 요청에 따라 `config/hold_tap.dtsi`의 mod-tap과 layer-tap을 모두 `balanced`, 170ms, `quick-tap-ms = 0`으로 변경했다. retro-tap과 prior idle, positional hold-tap은 사용하지 않는다. `balanced`는 permissive hold와 유사하나 두 펌웨어의 모든 동작이나 무선 타이밍이 완전히 같다는 뜻은 아니다.
- 동글 후보: `C:\temp\zmk-qmk-vial-latest\zephyr\zmk.uf2`, SHA-256 `951797C8B3575D0573C49C3355C3480BC4B355508E54D5507294CC01B98C00F7`. 빌드에 성공했으나 이 후보의 실기기 설치 및 작동은 아직 확인하지 않았다. 현재 동글에 마지막으로 확인한 설치 상태는 아래 11월 10일의 170ms balanced / quick-tap 170ms 후보다.
- 동글과 일반 모드 왼쪽 펌웨어 빌드에 성공했다. ZMK native 테스트는 중첩 탭의 hold, 170ms가 지난 hold, 일반 롤의 tap, mod-tap 및 layer-tap의 탭 후 홀드를 확인했다. BLE 지연과 사용자의 실제 DYA 키맵 동작을 확인한 것은 아니다.

## 2026-11-10 당시 장치 상태

- 사용자는 마지막 비교에서 `C:\temp\zmk-oled-shift-candidate\zephyr\zmk.uf2`를 동글에 다시 설치했다고 확인했다. 이 펌웨어의 mod-tap과 layer-tap은 모두 `balanced`, 170ms이며 quick-tap은 170ms다.
- 이 후보의 SHA-256: `090DB51BB7432289768D0C69DF364251ADA684E679E83F3BA26FD791F026F5A6`.
- 동글에 저장된 DYA 키맵은 별도 상태다. 확인 당시 D는 `MT LCtrl D`, F는 `MT LShift F`, J는 `MT RShift J`, Space는 `LT LAYER2 Space`, Enter는 `LT LAYER1 Enter`였다. 키맵 초기화는 수행하지 않았다.
- 11월 10일 작업 트리의 `config/hold_tap.dtsi`는 마지막 설치 펌웨어와 달리 mod-tap 시험용 `tap-preferred`, 220ms, quick-tap 170ms였다. 11월 11일에는 위 갱신에 설명한 값으로 변경했다.

## 진행 결과

- 동글 OLED에 좌우 배터리 막대와 20% 이하 점멸 표시, 고양이 애니메이션을 추가했다. BLE 슬롯은 좌우 고정이 아니므로 각 반쪽의 키를 누른 뒤 위치로 좌우를 판별한다. 식별 전이나 배터리 정보가 없으면 `--`로 표시한다.
- OLED 최초 후보에서는 오른쪽 일반 Shift와 J의 mod-tap Shift가 작동하지 않았다. BLE 입력 콜백에서 화면용 입력 관찰을 제거한 비교 후보에서는 일반 오른쪽 Shift가 다시 작동했다. 입력 이벤트 전달 후 화면 상태를 갱신하도록 옮긴 위 170ms 후보에서는 사용자가 두 Shift와 배터리 표시·애니메이션이 정상이라고 확인했다. 정확한 내부 실패 기전은 미확인이다. 실기기 20% 경고는 아직 확인하지 않았다.
- 128×64 흑백 LVGL 시뮬레이터는 `tests/dongle-display/simulator/`에 있다. 20% 경고 켜짐·꺼짐, 21%에서 경고 없음, 타이핑 시 고양이 프레임 변경을 확인했다. 이는 실제 BLE·I²C나 실물 OLED를 검증하지 않는다. 시뮬레이터 이미지: `C:\temp\sofle-simulator-final.png`.
- 사용자가 빠른 `으` 입력 중 Ctrl 오작동을 보고해, 실제 ZMK native test로 빠른 D/다음 키 중첩 입력이 기존 `balanced`에서 Ctrl로 판정됨을 재현했다. mod-tap을 `tap-preferred` 220ms로 바꾼 뒤 같은 테스트에서 일반 문자로 판정됐고, 의도적으로 길게 누른 Ctrl 조합도 통과했다. 테스트: `tests/hold-tap/fast-roll/`.
- 220ms 시험 펌웨어는 `C:\temp\zmk-tap-comfort-candidate\zephyr\zmk.uf2`이며 SHA-256은 `A3D9FDC1CED1319D2D2F5C617FA93DB485B117390063085F932B51735C7454B3`다. 사용자는 Ctrl 오타가 줄고 Ctrl/Shift 조합이 괜찮다고 했지만 Space·레이어와 전반적인 반응성이 느리다고 했다. 이전 170ms 후보로 비교한 뒤 일반 키는 이전 후보가 조금 더 빠르게 느껴진다고 했다.

## 이전 QMK/Vial 설정

- 사용자가 제공한 백업: `C:\Users\kalst\내 드라이브\키보드\vial_dilemma_4x6\dilemma_key_setup.vil`. 백업의 QMK Settings 값은 tapping term 170ms, permissive hold 켜짐, hold-on-other-key-press 꺼짐, retro tapping 켜짐, quick tap term 0, chordal hold 꺼짐, flow tap term 0이다. 이 값이 백업 시점의 값이며 현재 물리 QMK 장치의 설정과 일치하는지는 별도 확인하지 않았다.
- 로컬 QMK 저장소의 `keyboards/bastardkb/dilemma/4x6_4/keymaps/kalstein/config.h`에는 저장되지 않은 작업 트리 변경이 있어 백업 값과 일부 다르다. 이 파일의 현재 내용이 실제 장치에 설치된 펌웨어 설정이라는 증거는 없다.

## 11월 10일 기준 남은 일

1. 당시 장치에는 170ms 후보가 설치됐고 작업 트리는 220ms 후보였다. 임시 `C:\temp` 산출물은 지속 보관을 보장하지 않는다.
2. Space는 DYA에서 layer-tap이라 탭을 즉시 내보내지 못할 수 있다. `tap-preferred`에서는 미결정 mod-tap 뒤의 다른 키 이벤트도 대기할 수 있다. 양쪽 키보드의 설정에는 누름과 해제 디바운스 각각 8ms가 있고, 동글까지 BLE를 사용한다. 실제 지연 시간과 주원인은 측정하지 않았다.
3. 빠른 타자 중 Space에 `require-prior-idle-ms`를 적용하는 방안을 제안했으나, 사용자가 내일 이어 하기로 했으므로 아직 선택하거나 적용하지 않았다. 이는 연속 타자 중 Space를 빠르게 출력할 수 있지만 직전 입력 뒤 곧바로 레이어를 사용하는 동작을 방해할 수 있다. 테스트 또는 적용 전 이 교환관계를 확인한다.
4. 필요하면 OLED·오른쪽 Shift 회귀와 빠른 `으`, 의도적 Ctrl/Shift, 레이어 사용을 함께 실기기로 확인한다. QMK 백업과 완전히 동일한 동작이라고 가정하지 않는다.
