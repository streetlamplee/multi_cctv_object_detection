# 1. 베이스 이미지: AARCH64 아키텍처의 bullseye
FROM --platform=linux/amd64 debian:trixie

# 2. apt 패키지 설치 (OpenCV 빌드에 필요한 의존성 추가)
RUN apt update && apt install -y \
    build-essential \
    cmake \
    git \
    wget \
    unzip \
    libgtk2.0-dev \
    pkg-config \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    python3-dev \
    python3-numpy \
    libgfortran5 \
    libssl-dev \
    libmodbus-dev \
    nlohmann-json3-dev \
    mosquitto mosquitto-clients \
    libpaho-mqttpp-dev libpaho-mqtt-dev \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*

# 3. OpenCV 4.8.0 소스에서 빌드 및 설치
RUN wget -O opencv.zip https://github.com/opencv/opencv/archive/4.8.0.zip && \
    unzip opencv.zip && \
    mkdir -p opencv-4.8.0/build && \
    cd opencv-4.8.0/build && \
    cmake -D CMAKE_BUILD_TYPE=RELEASE \
          -D CMAKE_INSTALL_PREFIX=/usr/local \
          -D WITH_TBB=OFF \
          -D WITH_IPP=OFF \
          -D WITH_V4L=OFF \
          -D WITH_OPENGL=OFF \
          -D WITH_QT=OFF \
          -D BUILD_EXAMPLES=OFF \
          -D BUILD_TESTS=OFF \
          -D BUILD_PERF_TESTS=OFF .. && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -rf opencv.zip opencv-4.8.0

# 4. 작업 디렉터리 설정
WORKDIR /app
