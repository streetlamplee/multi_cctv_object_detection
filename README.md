# CCTV-YOLO: 실시간 객체 탐지 및 알람 시스템

## 1. 개요

이 프로젝트는 다중 CCTV 채널의 비디오 스트림을 실시간으로 처리하여, 커스텀 학습된 YOLOv8 모델을 사용해 특정 객체를 탐지하는 C++ 애플리케이션입니다. 객체가 탐지되면 정의된 조건에 따라 알람을 생성하고, MQTT 프로토콜을 통해 해당 알람을 발행합니다. 또한, 내장된 웹 서버를 통해 실시간 로그 및 알람 팝업을 확인할 수 있는 웹 UI를 제공합니다.

## 2. 핵심 기능

- **다중 채널 처리**: 여러 개의 CCTV 비디오 스트림을 동시에 처리합니다.
- **객체 탐지**: ONNX Runtime을 사용하여 양자화된 YOLOv8 모델(`yolov8n.quant.onnx`)로 추론을 수행합니다.
- **알람 시스템**: `alarm.conf`에 정의된 규칙에 따라 객체 탐지 시 알람을 생성합니다. 동일 알람에 대한 반복적인 알림을 방지하기 위해 60초의 쿨다운 기능이 포함되어 있습니다.
- **MQTT 연동**: 발생한 알람 정보를 MQTT 토픽(`CCTV/Alarm`)으로 발행하여 다른 시스템과 연동할 수 있습니다.
- **Modbus 통신**: Modbus 프로토콜을 이용한 통신 기능이 일부 구현되어 있습니다.
- **웹 인터페이스**: `index.html`과 `script.js`로 구현된 웹 UI를 통해 실시간 로그와 알람 발생 현황을 모니터링할 수 있습니다.

## 3. 프로젝트 구조

```
/
├── CMakeLists.txt         # 프로젝트 빌드 스크립트
├── init.sh                # Docker 빌드 및 실행 스크립트
├── README.md              # 프로젝트 문서
├── include/               # 헤더 파일
│   ├── alarm/
│   ├── cctv/
│   ├── config/
│   ├── inference/
│   └── mqttManager/
├── src/                   # 소스 코드
│   ├── main.cpp           # 애플리케이션 메인 로직
│   ├── alarm/
│   ├── cctv/
│   ├── config/
│   ├── inference/
│   └── mqttManager/
├── resource/              # 실행에 필요한 리소스
│   ├── app.ini            # 애플리케이션 설정
│   ├── alarm.conf         # 알람 규칙 설정
│   ├── yolov8n.quant.onnx # YOLOv8 ONNX 모델
│   ├── index.html         # 웹 UI
│   └── script.js          # 웹 UI 스크립트
├── shell_scripts/         # 셸 스크립트
│   ├── debug_build.sh     # 로컬 디버그 빌드 스크립트
│   └── docker-init.sh     # Docker 컨테이너 초기화 스크립트
└── lib/                   # 서드파티 라이브러리 (e.g., cpp-httplib)
```

## 4. 의존성

본 프로젝트는 다음 라이브러리들을 사용합니다.

- **OpenCV**: 비디오 스트림 처리 및 이미지 연산
- **Paho MQTT Cpp**: MQTT 프로토콜 통신
- **OpenSSL**: `cpp-httplib`의 HTTPS 지원을 위한 의존성
- **nlohmann_json**: JSON 데이터 처리
- **libmodbus**: Modbus 통신

## 5. 빌드 및 실행 방법

### 로컬 디버그 빌드

프로젝트를 로컬 환경에서 직접 빌드하고 실행할 수 있습니다.

1.  **빌드 스크립트 실행**:
    프로젝트 루트 디렉토리에서 아래 명령어를 실행하여 애플리케이션을 빌드합니다.

    ```bash
    ./shell_scripts/debug_build.sh
    ```
    이 스크립트는 `buildD` 디렉토리를 생성하고, 그 안에 실행 파일(`app`)과 필요한 리소스(`resource/`)를 준비합니다.

2.  **애플리케이션 실행**:
    빌드가 완료되면 생성된 `run.sh` 스크립트를 통해 애플리케이션을 실행합니다.

    ```bash
    ./buildD/run.sh
    ```

### Docker를 이용한 실행

`init.sh` 스크립트를 통해 Docker 이미지를 빌드하고 컨테이너를 실행할 수 있습니다.

```bash
./init.sh
```

## 6. 설정

- **애플리케이션 설정**: `resource/app.ini` 파일에서 CCTV 채널 정보 등 애플리케이션의 주요 설정을 변경할 수 있습니다.
- **알람 규칙**: `resource/alarm.conf` 파일에서 알람 발생 조건 및 내용을 정의할 수 있습니다.