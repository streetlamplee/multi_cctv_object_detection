docker run --privileged --rm tonistiigi/binfmt --install arm64
docker run --rm -v "$(pwd)":/app j4125_cctv /bin/bash -c "\
    export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH; \
    rm -rf build; \
    mkdir build; \
    cd build; \
    cmake ..; \
    make; \
    cd ..; \
    /app/shell_scripts/copy_sysroot.sh;\
    "
