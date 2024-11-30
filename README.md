# RUDPServer

1. 개요
2. 구성

---

1. 개요

RUDP를 만들어 보고, 이를 테스트 하기 위한 프로젝트입니다.

RIO를 이용하려고 했으나, 구조가 UDP와는 어울리지 않는 것 같아 수정 예정입니다.

---

2. 구성

2.1 스레드 구성

* IORecvWorkerThread
  * 하나의 스레드만 존재하며, 하나의 소켓이 지속적으로 recvfrom()을 수행합니다.
  * 송신 받은 데이터 스트림을 버퍼에 셋하고, 적절한 LogicWorkerThread들 중 하나에 버퍼를 전달합니다.
  * LogicWorkerThread로 버퍼를 전달하면 이벤트 플래그를 셋하여 해당하는 스레드에 로직 실행을 전담시킵니다.
  * 단 IOSendWorkerThread에 있는 소켓과는 별도의 소켓으로, 수신만을 목적으로 사용하는 소켓입니다.

* LogicWorkerThread
  * n개의 스레드가 존재하며, IORecvWorkerThread 혹은, 외부에서 정지할 때, 이벤트가 셋 되어, 현재 해당 스레드의 큐에 저장 되어 있는 모든 버퍼를 처리합니다.
  * 생성된 패킷 처리와 세션 생성을 담당합니다.

* RetransmissionThread
  * n개의 스레드가 존재하며, 주기적으로 깨어나 아래를 수행합니다.
    * 송신한 패킷들이 정상적으로 송신 대상에 보내졌는지 확인하며, 보내지지 않았다면, 재전송을 합니다.
      * three hand shake는 아직 미구현 상태입니다.
    * 일정 횟수 이상 송신에 실패한다면, 끊어진 세션이라고 판단하고, 해당 세션을 정리하는 과정을 거칩니다.
    * 로직 스레드에서 생성된 송신 패킷들을 send 합니다(IORecvWorkerThread로 이관 예정)

* SessionDeleteThread
  * 끊어진 세션이거나 명시적으로 서버에서 세션을 끊을 대상들을 다른 스레드에서 deleteSessionIdList에 기록하면, 이 스레드가 루프를 돌면서 해당 세션 객체들을 정리하는 스레드입니다.
  * 1개의 스레드만 존재하며, n개의 리스트를 순회하면서 다른 스레드에서 정리를 요청한 세션을 정리합니다.

* IOSendWorkerThread에
  * n개의 스레드가 존재하며, 하나의 소켓이 지속적으로 sendto()를 수행합니다.(예정)
  * 단 IORecvWorkerThread에 있는 소켓과는 별도의 소켓으로, 송신만을 목적으로 사용하는 소켓입니다.

2.2 내부 주요 클래스 

* RUDPServerCore
* RUDPSession

---

3. 발견된 문제

* 현재 구조에서는 클라이언트가 바라보는 recv 소켓이 하나 밖에 없는데, 해당 소켓에 부담이 너무 커진다.
  * 클라이언트 수가 적으면 문제 없겠지만, 많아질 경우 소켓 버퍼커 터질 가능성도 존재함
  * 현재 구조상 해결이 불가능한 문제이기에 MultiSocketRUDP[https://github.com/m5623skhj/MultiSocketRUDP]에 새로 작성하기로 함
