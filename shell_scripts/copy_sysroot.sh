#!/bin/bash

# --- 스크립트 설정 ---

# 1. 컴파일된 실행 파일 경로
EXECUTABLE="./build/app"

# 2. SDK를 생성할 루트 폴더
SDK_ROOT_DIR="/app/app"

# 3. C++ 라이브러리를 복사할 하위 폴더
SDK_LIB_DIR="$SDK_ROOT_DIR/lib"


# --- 스크립트 시작 ---

# 1. 스크립트가 실패하면 즉시 중지
set -e

# 2. 🛡️ 아키텍처 안전장치
ARCH=$(uname -m)
if [ "$ARCH" != "x86_64" ]; then
    echo "🛑 ERROR: This script must be run inside the x86_64 container."
    echo "   Current architecture is '$ARCH', not 'x86_64'."
    exit 1
fi

echo "✅ Architecture check passed (running on $ARCH)."

# 3. 🧹 기존 SDK 폴더 정리 및 생성
echo ">>> Cleaning and creating SDK directory: $SDK_ROOT_DIR"
rm -rf "$SDK_ROOT_DIR"
mkdir -p "$SDK_LIB_DIR"

# 4. 🚚 실행 파일 복사
echo ">>> Copying executable..."
cp -v "$EXECUTABLE" "$SDK_ROOT_DIR/app"

# 5. 🚚 리소스 파일 복사 (모델 등)
echo ">>> Copying resource files..."
cp -rv resource "$SDK_ROOT_DIR/"

# 6. 🚚 C++ 의존성 라이브러리 복사
echo ">>> Copying C++ dependencies for $EXECUTABLE..."
ldd "$EXECUTABLE" | \
    awk '/=>/ {print $3}' | \
    grep -Ev 'libc.so|libm.so|libpthread.so|libgcc_s.so|libstdc++.so|librt.so|libdl.so|ld-linux' | \
    sort -u | \
    xargs -I {} cp -vL {} "$SDK_LIB_DIR"



# 8. 📜 run.sh 스크립트 생성
echo ">>> Creating run.sh..."
cat << 'EOF' > "$SDK_ROOT_DIR/run.sh"
#!/bin/bash

# 1. 이 스크립트가 있는 디렉토리를 기준으로 작업 디렉토리 설정
#    (어디에서 실행하든 스크립트 위치 기준으로 작동)
cd "$(dirname "$0")"

# 2. ./lib 폴더를 C++ 라이브러리 검색 경로로 설정
export LD_LIBRARY_PATH=$(pwd)/lib

# 3. 실행 파일 실행
echo "Starting app with LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
./app
EOF

# 9. 🔑 run.sh에 실행 권한 부여
chmod +x "$SDK_ROOT_DIR/run.sh"

echo ""
echo "-------------------------------------"
echo "✅ SDK Packaging Complete."
echo "-------------------------------------"
echo "Directory '$SDK_ROOT_DIR' is ready."
echo "이제 '/app' 디렉토리에서 'app' 폴더 전체를 타겟 보드로 복사하세요."
echo ""
echo "타겟 보드에서 실행 방법:"
echo "  cd app"
echo "  sudo ./run.sh"