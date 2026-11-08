# Sofle 펌웨어

Sofle 좌우 키보드와 선택적으로 사용하는 OLED 동글의 역할 및 사용 모드를 정의한다.

## Language

**일반 모드**:
OLED 동글 없이 Sofle 좌우 키보드를 사용하는 구성. 왼쪽 키보드가 Central이고 오른쪽 키보드가 Peripheral이다.
_Avoid_: 단독 모드

**동글 모드**:
Sofle 좌우 키보드가 모두 Peripheral이고, OLED 동글이 Central로서 컴퓨터에 USB HID 입력만 전달하는 구성. 컴퓨터와의 Bluetooth 연결은 제공하지 않는다.
_Avoid_: 동글 화면 모드

**OLED 동글**:
동글 모드에서 키 입력을 모으고 컴퓨터에 전달하며, OLED 화면으로 키보드 상태를 표시하는 장치.
_Avoid_: 화면 모듈

**Central**:
이 키보드 구성에서 Peripheral의 입력을 모아 키맵과 레이어에 따라 해석하고 컴퓨터에 전달하는 장치.
_Avoid_: 항상 왼쪽 키보드

**Peripheral**:
이 키보드 구성에서 자신의 키와 엔코더 입력을 BLE로 Central에 전달하는 키보드 반쪽.
_Avoid_: 항상 오른쪽 키보드

**Idle**:
일정 시간 활동이 없을 때 화면과 조명 등을 끄면서 연결을 유지하는 상태.

**Deep Sleep**:
일정 시간 활동이 없을 때 키보드가 들어가는 소프트웨어 전원 차단 상태. 설정된 깨우기 수단으로 복귀할 수 있다.
_Avoid_: Soft Off, 자동 절전

**Soft Off**:
사용자가 명시적으로 진입하는 소프트웨어 전원 차단 상태. 이 키보드는 리셋 버튼으로 복귀한다.
_Avoid_: 깊은 절전, Deep Sleep

**OLED 소등**:
OLED 동글의 화면이 꺼져 있는 상태. 동글 자체의 종료나 키보드 연결 해제를 뜻하지 않는다.
_Avoid_: 동글 종료
