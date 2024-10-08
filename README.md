## Network Socket Project
### 회의실 프로그램(ConferenceProgram)
#### 프로젝트 소개
회의실 프로그램은 다자간 채팅으로 클라이언트가 서버로 데이터를 보내면, 서버는 받은 데이터를 파일에 저장하고 서버와 연결된 모든 클라이언트들에게 받은 데이터를 보내는 프로그램입니다. 회의실 서버를 구현하기 위해 Completion Port 모델을 사용하여 다중 처리와 메모리 최적화 및 소켓 함수 호출 시 블로킹을 최소화하였습니다. 회의실 클라이언트는 멀티 스레드를 사용해서 데이터 송신과 수신을 각각 수행하도록 하였습니다.

#### 실행 화면
<img width="90%" src="https://user-images.githubusercontent.com/76869178/279665862-b9b6970c-2bb3-410b-bc45-2a91790b783c.png"/>
<img width="60%" src="https://user-images.githubusercontent.com/76869178/279665888-b5de4fa5-73e7-44fa-aad1-5ddd5bf6d2f5.png"/>

#### 추가적으로
구체적인 계획 없이 코딩부터 치며 개발하였고 실행하면서 오류가 생길 때마다 보완하는 식으로 개발하였기에 소스 코드가 매우 더럽습니다.

<br>
<br>

### Unity 채팅 서버(Go/ChatProgram)
#### 소개
Jaeun-Choi98/GameWorld 프로젝트에서 채팅 기능을 구현하기 위해 설계한 채팅 서버입니다. 
실행 화면은 https://github.com/Jaeun-Choi98/GameWorld README.md 소개 글에 있는 게임 영상 링크에서 확인하실 수 있습니다.

<br>
<br>

### Unity 오브젝트 동기화 서버(Go/MultiplayerSyncProgram)
#### 소개 
Jaeun-Choi98/GameWorld 프로젝트에서 오브젝트 동기화 기능을 구현하기 위해 설계한 서버입니다.
실행 화면은 https://github.com/Jaeun-Choi98/GameWorld README.md 소개 글에 있는 게임 영상 링크에서 확인하실 수 있습니다.
