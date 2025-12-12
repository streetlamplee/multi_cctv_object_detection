BASE_DIR=$(dirname "$(realpath "$0")")

cd ${BASE_DIR}

rm -rf app
mkdir app
cd app
cmake ..
make
cd ..
