#!/bin/bash

TARGET_NAME="user"
TARGET_IP="192.168.0.9"
TARGET_FOLDER="/opt/"
BASE_DIR=$(dirname "$(realpath "$0")")
BASE_DIR=${BASE_DIR}/..

echo "Starting deployment..."

# 여러 파일을 한 번에 나열해서 전송
echo scp -r \
    "${BASE_DIR}/include" \
    "${BASE_DIR}/lib" \
    "${BASE_DIR}/resource" \
    "${BASE_DIR}/src" \
    "${BASE_DIR}/CMakeLists.txt" \
    "${BASE_DIR}/shell_scripts/compile.sh" \
    "${TARGET_NAME}@${TARGET_IP}:${TARGET_FOLDER}"
scp -r \
    "${BASE_DIR}/include" \
    "${BASE_DIR}/lib" \
    "${BASE_DIR}/resource" \
    "${BASE_DIR}/src" \
    "${BASE_DIR}/CMakeLists.txt" \
    "${BASE_DIR}/shell_scripts/compile.sh" \
    "${TARGET_NAME}@${TARGET_IP}:${TARGET_FOLDER}"

echo "Done."