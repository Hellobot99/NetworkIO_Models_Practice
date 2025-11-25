#!/bin/bash

CLIENT_SRC="client.cpp"
OUTPUT="client"
IP="127.0.0.1"
PORT=9190

# 1. 클라이언트 컴파일
echo "[컴파일 중...]"
g++ $CLIENT_SRC -o $OUTPUT
if [ $? -ne 0 ]; then
    echo "❌ 컴파일 실패!"
    exit 1
fi

# 2. 병렬 클라이언트 실행
for i in $(seq 1 10); do
  ROOM="Room$((i % 3 + 1))"  # Room1, Room2, Room3 중 하나
  {
    echo "User$i"            # 사용자 이름
    echo "$ROOM"             # 방 이름
    sleep 5                 # 5초 대기
    echo "Hello from User$i in $ROOM"  # 채팅 메시지
    echo "exit"              # 종료 명령
  } | ./$OUTPUT $IP $PORT > "client_$i.log" 2>&1 &
done

# 3. 모든 클라이언트 종료 대기
wait
echo "✅ 모든 클라이언트 테스트 완료!"

echo " 로그 확인 중..."
for i in $(seq 1 10); do
  LOG_FILE="client_$i.log"
  if [ -f "$LOG_FILE" ]; then
    echo "🔍 $LOG_FILE:"
    cat "$LOG_FILE"
    echo "───────────────────────────────"
  else
    echo "❌ $LOG_FILE 파일이 존재하지 않습니다!"
  fi
done