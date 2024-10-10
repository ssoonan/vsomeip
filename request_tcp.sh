#!/bin/bash

# 환경 변수 설정
export VSOMEIP_CONFIGURATION="config/vsomeip-tcp-client.json"
export VSOMEIP_APPLICATION_NAME="client-sample"

# 실행하려는 바이너리 경로
BINARY_PATH="build/examples/request-tcp-recovery"
$BINARY_PATH