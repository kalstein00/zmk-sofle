# 무선 동글 Phase 1 호스트 시험

Linux 또는 WSL에서 GCC와 Python 3을 준비한 뒤 저장소 루트에서 실행한다. 인수는 `config/west.yml`에 고정된 ZMK의 `app` 디렉터리다.

```sh
python3 tests/dongle-host/run.py /path/to/west/zmk/app
```

시험은 실제 상태 처리 코드와 고정 ZMK에서 생성한 HID·출력·페어링 코드를 실행한다. 기존 키 해제와 전환 중 누름 소비, 대기 HID 보고서의 PC 귀속, 오프라인 입력 폐기, 명시적 USB/BLE 전환을 검증한다. 컨트롤러 시험은 토글 키를 누른 뒤에도 전환이 끝나는지, 백그라운드 BLE 연결이 USB에서 누른 키를 해제하지 않는지, 이전 마우스 입력 콜백이 끝날 때까지 출력이 잠겨 있는지 확인한다. 페어링 시험은 이미 등록된 PC의 빈 슬롯 진입 차단, 120초 만료와 재시도, 다른 슬롯 선택 시 취소, PC 본딩 삭제 범위를 확인한다.

Bluetooth와 USB 전송은 시험용 경계로 교체한다. 실제 Windows 11 페어링·재연결, 좌우 키보드 동시 연결, DYA Studio 동작은 [기기 시험 절차](../../DONGLE_BUILD.md#무선-동글-phase-1)에서 검증한다.
