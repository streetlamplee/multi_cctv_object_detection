import argparse
import os
from rknn.api import RKNN




def main():
    parser = argparse.ArgumentParser(description='Convert ONNX model to RKNN model')

    # 필수 인자: ONNX 파일 경로 및 타겟 플랫폼
    parser.add_argument('--input', '-i', type=str, required=True, help='Path to the input ONNX model')
    parser.add_argument('--target', '-t', type=str, required=True,
                        choices=['rk3562', 'rk3566', 'rk3568', 'rk3576', 'rk3588'],
                        help='Target Rockchip platform (e.g., rk3588)')

    # 선택 인자: 출력 경로 및 양자화 여부
    parser.add_argument('--output', '-o', type=str, help='Path to the output RKNN model (default: same as input)')
    parser.add_argument('--quant', '-q', action='store_true', help='Enable quantization (requires dataset)')
    parser.add_argument('--dataset', '-d', type=str, default='./dataset.txt',
                        help='Path to the dataset for quantization')

    args = parser.parse_args()

    # 출력 경로 설정 (지정하지 않으면 입력 파일 이름 기반으로 자동 생성)
    if not args.output:
        args.output = os.path.splitext(args.input)[0] + '.rknn'

    rknn = RKNN(verbose=True)

    # 1. Config (평균/표준편차는 일반적인 ImageNet 기준값 유지)
    print(f'--> Config model for {args.target}')
    rknn.config(mean_values=[0, 0, 0],
                std_values=[255, 255, 255],
                target_platform=args.target,
                )
    print('done')

    # 2. Load ONNX
    print(f'--> Loading ONNX model: {args.input}')
    ret = rknn.load_onnx(model=args.input, input_size_list=[[1, 3, 224, 384]])
    if ret != 0:
        print('Load model failed!')
        return
    print('done')

    # 3. Build
    print(f'--> Building model (Quantization: {args.quant})')
    ret = rknn.build(do_quantization=args.quant, dataset=args.dataset)
    if ret != 0:
        print('Build model failed!')
        return
    print('done')

    # 4. Export
    print(f'--> Exporting RKNN model to: {args.output}')
    ret = rknn.export_rknn(args.output)
    if ret != 0:
        print('Export RKNN model failed!')
        return
    print('done')

    rknn.release()
    print(f"Successfully converted to {args.output}")


if __name__ == '__main__':
    main()