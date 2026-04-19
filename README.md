# CCTV-YOLO-RKNN: 실시간 객체 탐지 및 지능형 알람 시스템

## 1. 개요

이 프로젝트는 다중 CCTV 채널의 비디오 스트림을 실시간으로 처리하여, Rockchip NPU(RKNPU)에 최적화된 YOLOv8 모델을 통해 특정 객체를 탐지하는 고성능 C++ 애플리케이션입니다. Rockchip 하드웨어 가속(RKNN)을 활용하여 낮은 전력 소비로 실시간 추론을 수행하며, 탐지된 객체 정보를 바탕으로 지능형 알람을 생성하고 MQTT 및 웹 인터페이스를 통해 실시간 모니터링 기능을 제공합니다.

## 2. 핵심 기능

- **하드웨어 가속 추론 (RKNN)**: Rockchip NPU를 활용하여 YOLOv8 모델을 실행합니다. `best.rknn` 모델을 사용하여 고속 객체 탐지를 수행합니다.
- **다중 채널 효율적 처리**: POSIX 세마포어를 이용한 리소스 관리 시스템을 통해 다중 카메라 채널을 안정적으로 동기화 처리합니다.
- **ISAPI 이미지 획득**: Hikvision 등 ISAPI를 지원하는 장치로부터 고해상도 이미지 및 서브 스트림 이미지를 획득합니다.
- **지능형 알람 시스템**: `alarm.conf` 규칙에 따라 객체 탐지 시 알람을 생성하며, 동일 알람 중복 방지를 위한 쿨다운(Cool-time) 기능이 포함되어 있습니다.
- **MQTT 및 Modbus 연동**: 발생한 알람 정보를 MQTT 토픽(`CCTV/Alarm`)으로 발행하며, Modbus 프로토콜을 통한 외부 장비 제어 기능이 포함되어 있습니다.
- **웹 모니터링 인터페이스**: `cpp-httplib` 기반의 내장 웹 서버를 통해 실시간 탐지 결과(이미지 및 바운딩 박스)와 알람 로그를 웹 UI에서 확인할 수 있습니다.
- **데이터 자동 수집**: 추론 결과와 원본 이미지를 YOLO 포맷의 텍스트와 JPEG로 자동 저장하여 향후 모델 학습을 위한 데이터셋 구축 기능을 제공합니다.

## 3. 프로젝트 구조

```
/
├── CMakeLists.txt         # 프로젝트 빌드 스크립트
├── init.sh                # Docker 빌드 및 실행 스크립트
├── include/               # 헤더 파일
│   ├── alarm/             # 알람 매니저 및 규칙 처리
│   ├── cctv/              # 카메라 프레임 획득 및 제어
│   ├── config/            # INI 및 설정 핸들러
│   ├── inference/         # RKNN 기반 추론 엔진
│   ├── mqttManager/       # MQTT 통신 관리
│   ├── server/            # HTTP 웹 서버
│   └── thread_safe/       # 스레드 안전 큐 및 스택
├── src/                   # 소스 코드
│   ├── main.cpp           # 메인 제어 로직 및 스레드 루틴
│   └── (상위 include와 매칭되는 구현체 파일들)
├── assets/resource/       # 실행 리소스
│   ├── app.ini            # 애플리케이션 메인 설정
│   ├── alarm.conf         # 알람 규칙 설정
│   ├── best.rknn          # NPU 최적화 YOLOv8 모델
│   ├── index.html         # 웹 UI 메인 페이지
│   └── output/            # 실시간 탐지 결과 출력 폴더
├── scripts/               # 유틸리티 스크립트
│   ├── rknn/              # 모델 변환 (ONNX -> RKNN) 스크립트
│   └── shell_scripts/     # 빌드 및 배포 자동화 스크립트
└── lib/                   # 외부 라이브러리 (rknn, httplib 등)
```

## 4. 기술 스택 및 의존성

- **Language**: C++17
- **Hardware Acceleration**: RKNN SDK (Rockchip NPU)
- **Computer Vision**: OpenCV 4.x
- **Network**: 
  - `cpp-httplib`: HTTP Server/Client
  - `Paho MQTT Cpp`: MQTT Client
  - `libmodbus`: Modbus Communication
- **Serialization**: `nlohmann_json`
- **Synchronization**: POSIX Semaphores, std::atomic, std::mutex

## 5. 설치 및 빌드 방법

### 하드웨어 요구사항
- Rockchip NPU가 탑재된 보드 (예: RV1106, RV1103, RK3588 등)
- RKNPU 드라이버 및 SDK 설치 필요

### 빌드 프로세스

1.  **환경 설정**: `assets/resource/app.ini` 파일을 열어 카메라 IP, 계정 정보, NVR 설정을 환경에 맞게 수정합니다.
2.  **빌드 실행**:
    ```bash
    ./scripts/shell_scripts/compile.sh
    ```
    또는 디버그 모드 빌드:
    ```bash
    ./scripts/shell_scripts/debug_build.sh
    ```
3.  **애플리케이션 실행**:
    ```bash
    ./build/app
    ```

### Docker 환경 (추천)
```bash
./init.sh
```

## 6. 주요 설정 안내

- **app.ini**: 채널 개수, MQTT 브로커 정보, 카메라 연결 정보, 데이터 저장 경로 등을 설정합니다.
- **alarm.conf**: 특정 클래스(Class ID)가 탐지되었을 때의 알람 조건과 메시지를 정의합니다.
- **Semaphore 관리**: `main.cpp`에서 설정된 `/get_image`와 `/inference` 세마포어 값을 통해 동시 처리량을 조절할 수 있습니다.