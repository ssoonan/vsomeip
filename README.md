# vSomeIP 프로토콜 최적화 프로젝트

본 README는 **vSomeIP** 프로토콜의 **Service Discovery (SD) 시간** 및 **TCP 패킷 회복 시간** 최적화를 목표로 진행한 프로젝트의 세부 내용을 담고 있음.

---

## 1. 개요

### 프로젝트 배경

[vSomeIP](https://github.com/covesa/vsomeip)는 자동차 통신 시스템에서 널리 사용되는 미들웨어 솔루션으로, ECU 간의 통신을 지원  
본 프로젝트는 **SD 시간**과 **TCP 패킷 회복 시간**을 최적화하는 과제

| vSomeIP에 대한 개괄적인 내용: https://github.com/COVESA/vsomeip/wiki/vsomeip-in-10-minutes

### 프로젝트 최종 목표

1. **SD 지연 시간** 단축

- [x] 2024-2: 4s (테스트 통과)
- [ ] 2026-2: 2s

2. **TCP 패킷 회복 시간** 단축

- [x] 2024-2: 4s (테스트 통과)
- [ ] 2026-2: 2s


---

## 2. 실험

### 개발 환경

**하드웨어**

- **개발 보드**: NVIDIA Orin 2대
- **CPU**: ARMv8
- **메모리**: 32GB

**소프트웨어**

- **운영체제**: Ubuntu 20.04
- **커널 버전**: Linux Kernel 5.10.104-tegra (일부 설정은 제외되어있음)
-

**네트워크 구성**

- **물리적 네트워크 환경**: NVIDIA Orin 2대를 이더넷 케이블로 다이렉트로 연결하여 통신 테스트.
- **통신 프로토콜**: SD는 UDP의 멀티캐스트, TCP Packet Recovery는 TCP 통신 가정
- **패킷 손실 환경**: 2대가 유선으로 바로 연결되어 있기에 일반적인 상황에서 패킷이 유실될 가능성은 없음. 그렇기에 tc, iptables 커맨드를 활용하여 인위적으로 손실 유발. 타겟 보드에선 tc 커맨드 사용 불가로 iptables 커맨드 사용

```bash
sudo iptables -D INPUT -i wlo1 -m statistic --mode random --probability 0.1 -j DROP # 10% 확률로 해당 컴퓨터로 들어오는 패킷의 10%를 유실
```

### 빌드 커맨드

vsomeip 설치는 다 되었다는 가정 하에 (https://github.com/COVESA/vsomeip)

```
make examples
```

실행으로 examples 내 코드만 빌드 가능

### 테스트 방법

**SD 지연 시간**

1. 한 대의 보드에서 `./response.sh` 스크립트 실행 -> SomeIP 프로토콜의 FindService 호출 시 응답할 수 있게 대기 (이하 server)
2. 다른 보드에서 `./request_sd.sh` 스크립트 실행으로 FindService 송신 (이하 client)
3. server에서 client로부터 온 FindService를 체크 후 application name, service id, instance id가 일치할 시 응답 후 해당 시간 측정
4. 1-3까지의 과정 시간을 10번 측정 한 후 이 평균 값을 1회의 시간으로 측정, 10회의 측정값이 기준값을 통과하는지 최종 확인 (총 100회 요청 테스트)

**TCP 패킷 회복 시간**

1. 앞선 `iptables` 커맨드로 인위적 패킷 손실 환경 만들기
2. 한 대의 보드에서 `./response_tcp.sh` 스크립트 실행으로 응답 대기
3. 다른 보드에서 `./request_tcp.sh` 스크립트 실행으로 SD 과정 없이 바로 TCP 메시지 통신 시작
4. 10% 확률로 랜덤하게 패킷 손실이 발생 시 TCP 프로토콜 내에 내장된 로직으로 패킷 손실 회복 과정 진행
5. wireshark 툴을 활용하여 해당 패킷 복구 소요 시간 측정
6. 패킷 손실이 10번 발생할 때까지 실험 진행 후 10회 내에서 모두 통과하는지 최종 확인

---

## 3. 현재까지 발견 과정

- config 내 `request_response_delay` 조절을 통한 SD 시간 단축
- `iptables`를 통한 패킷 로스는 소프트웨어 레벨로 이를 막지만 wireshark는 하드웨어 데이터를 관찰하기에 둘 간의 패킷 불일치 발생



---
## 4. 향후 과제

config를 통한 시간 단축, TCP 프로토콜 내에서 할 수 있는 테스트는 다 해봤기에 SOME/IP 내용 이해 후 vSOMEIP C++ 코드 최적화 과정 진행 필요


---

## 5. 참고 자료

- [SOME/IP 프로토콜 명세서](https://some-ip.com/standards.shtml)
  - 이 중 SD 관련된 내용 위주로 살펴보기
- [차량 내에서 TCP](https://some-ip.com/papers/2022-11_IEEE-Techday_TCP_and_Automotive_Ethernet.pdf)
  - 어떻게 차량 환경에서 TCP 프로토콜을 최적화 할 지에 대한 가이드
- 기타 웹 자료
  - https://watchout31337.tistory.com/444
  

---

### 비고

2024.12 기준 담당자
- 옥순환 (shock@redwood.snu.ac.kr)
- 이남철 (nclee@redwood.snu.ac.kr)
