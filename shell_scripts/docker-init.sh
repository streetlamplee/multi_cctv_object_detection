docker run --privileged --rm tonistiigi/binfmt --install arm64
docker run --rm -v "$(pwd)":/app aarch64-build /bin/bash -c "\
    rm -rf build; \
    mkdir build; \
    cd build; \
    cmake ..; \
    make; \
    cd ..; \
    /app/shell_scripts/copy_sysroot.sh;\
    "
