# multi_cctv_object_detection

요양병원 CCTV 객체 탐지 및 위험 감지 시스템

## 개요

본 프로젝트는 요양병원 내 다중 CCTV 영상을 실시간으로 분석하여 객체를 탐지하고, 설정된 조건에 따라 위험 상황을 감지하여 알림을 보내는 시스템입니다. YOLOv8 모델을 사용하여 객체 탐지를 수행하며, 웹 인터페이스를 통해 실시간 영상 및 탐지 결과를 확인할 수 있습니다.

## 주요 기능

- **다중 CCTV 영상 지원**: 최대 12대의 CCTV 카메라 영상을 동시에 처리합니다.
- **실시간 객체 탐지**: YOLOv8 ONNX 모델을 활용하여 사람 등의 객체를 실시간으로 탐지합니다.
- **위험 상황 감지 및 알림**: 설정 파일(`alarm.conf`)에 정의된 규칙에 따라 위험 상황(예: 넘어짐)을 감지하고 알림을 생성합니다.
- **웹 기반 모니터링**: 웹 브라우저를 통해 CCTV 영상과 객체 탐지 상황을 실시간으로 모니터링할 수 있습니다.
- **유연한 설정**: `app.ini` 파일을 통해 애플리케이션의 주요 설정을 변경할 수 있습니다.

## 빌드 방법

### 요구 사항

- C++ 컴파일러 (g++ 등)
- CMake 3.10 이상
- OpenCV

### 빌드 절차

1.  **저장소 복제**
    ```bash
    git clone https://github.com/your-repository/multi_cctv_object_detection.git
    cd multi_cctv_object_detection
    ```

2.  **빌드 디렉토리 생성 및 빌드**
    ```bash
    mkdir build
    cd build
    cp -rv ../resource ./resource
    cmake ..
    make
    ```

## 실행 방법

빌드가 완료되면 `build` 디렉토리 내에 실행 파일이 생성됩니다.

```bash
./app
```

애플리케이션 실행 후, 웹 브라우저에서 `http://localhost:8080` (포트는 `app.ini`에서 설정 가능) 또는 실행한 SBC의 IP로 접속하여 모니터링 페이지를 확인할 수 있습니다.

## 설정

-   `resource/app.ini`: 애플리케이션의 기본 설정을 관리합니다. (예: 웹 서버 포트, 로그 레벨 등)
-   `resource/alarm.conf`: 위험 상황 감지 규칙을 설정합니다.
-   `resource/yolov8n.onnx`: 객체 탐지에 사용되는 ONNX 모델 파일입니다.
