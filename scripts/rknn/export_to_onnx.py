from ultralytics import YOLO

# 1. 학습된 .pt 모델 로드
model = YOLO("./best.pt")

# 2. ONNX로 내보내기
# opset=17: 이전의 Opset 19 에러를 피하면서 최신 기능을 지원하는 최적의 설정입니다.
# imgsz: 640 또는 모델 학습 시 설정한 크기로 고정하는 것이 NPU 성능에 유리합니다.
success = model.export(format="onnx", opset=12, imgsz=[224,384], simplify=True)

if success:
    print(f"변환 완료: {success}")