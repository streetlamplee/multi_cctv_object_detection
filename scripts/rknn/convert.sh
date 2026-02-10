BASE_DIR=$(dirname "$(realpath "$0")")

cd ${BASE_DIR}

echo "가상환경 생성중..."

python3.10 -m venv rknn-venv
python3 -m venv onnx-venv

echo "onnx 변환 가상환경 설정"
source onnx-venv/bin/activate
pip install -r onnx-venv-requirements.txt
echo "onnx 변환 중"
python3 export_to_onnx.py

deactivate

echo "rknn 변환 가상환경 설정"
source rknn-venv/bin/activate
pip install -r rknn-venv-requirements.txt
echo "rknn 변환 중"
python3.10 convert_to_rknn.py -i './best.onnx' -t 'rk3576'

echo "rknn 변환 완료"