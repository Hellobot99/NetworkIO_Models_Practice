#!/bin/bash

# 파일명
SERVER_SRC="server.cpp"
THREADPOOL_SRC="thread_pool.cpp"
OUTPUT="server"
PORT=9190

# 1. 컴파일
echo "[컴파일 중...]"
g++ $SERVER_SRC $THREADPOOL_SRC -o $OUTPUT -pthread

# 2. 컴파일 실패 시 종료
if [ $? -ne 0 ]; then
    echo "❌ 컴파일 실패!"
    exit 1
fi

echo "✅ 컴파일 성공! 실행 중..."
echo "🌐 서버 포트: $PORT"
echo "────────────── 로그 ──────────────"

# 3. 서버 실행
./$OUTPUT $PORT
