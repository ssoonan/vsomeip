#!/bin/bash

# 환경 변수 설정
export VSOMEIP_CONFIGURATION="config/vsomeip-udp-client.json"
export VSOMEIP_APPLICATION_NAME="client-sample"

# 실행하려는 바이너리 경로
BINARY_PATH="build/examples/request-sd"  # 여기서 "your_binary"는 실행하려는 바이너리 이름입니다.

for i in {1..10}
do
    echo "실행 횟수: $i"
    # 바이너리 실행 (백그라운드로 실행)
    $BINARY_PATH &
    PID=$!  # 실행된 프로세스의 PID 저장

    # 1초 대기
    sleep 2

    # 실행 중인 프로세스 종료
    if kill -0 $PID 2>/dev/null; then
        kill $PID
        wait $PID 2>/dev/null  # 프로세스가 제대로 종료되었는지 확인
    fi
done
