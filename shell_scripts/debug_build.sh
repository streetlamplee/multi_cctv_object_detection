#!/bin/bash


set -e
echo '--- Cleaning and Building Application ---' 
rm -rf buildD
mkdir buildD
cd buildD
cmake .. 
make -j
cd ..
cp -rv resource ./buildD

cat << 'EOF' > "./buildD/run.sh"
#!/bin/bash

# 1. 이 스크립트가 있는 디렉토리를 기준으로 작업 디렉토리 설정
#    (어디에서 실행하든 스크립트 위치 기준으로 작동)
cd "$(dirname "$0")"

./app
EOF

chmod +x "./buildD/run.sh"
