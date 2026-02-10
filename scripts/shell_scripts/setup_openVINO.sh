# 1. GPG 키 다운로드 및 저장
wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB

# 2. OpenVINO 저장소 추가 (Ubuntu 22.04 기준, 버전에 따라 jammy/focal 선택)
echo "deb https://apt.repos.intel.com/openvino/2024 ubuntu22 main" | sudo tee /etc/apt/sources.list.d/intel-openvino-2024.list

# 3. 패키지 목록 업데이트
sudo apt update

sudo apt install intel-opencl-icd

# C++ 개발 환경을 위한 전체 패키지 설치
sudo apt install openvino-2024.0.0

sudo apt update
sudo apt install -y intel-opencl-icd libze1 libze-dev
# sudo apt install -y intel-media-va-driver-non-free libmfx1 libmfxgen1
sudo usermod -aG video,render $USER
newgrp video
newgrp render
