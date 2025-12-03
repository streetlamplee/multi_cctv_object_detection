# 홈 디렉토리로 이동 (원하는 위치가 있다면 변경 가능)
cd ~

# go2rtc 최신 버전 다운로드 (Linux arm64용)
wget https://github.com/AlexxIT/go2rtc/leases/latest/download/go2rtc_linux_arm64

# 파일 이름을 사용하기 편하게 변경
mv go2rtc_linux_arm64 go2rtc

# 실행 권한 부여
chmod +x go2rtc

touch go2rtc.yaml

echo '''api:
  listen: ":1984"  # API 및 웹 인터페이스 포트

rtsp:
  listen: ":8554"  # RTSP 서버 포트

log:
  level: info''' >> go2trc.yaml

