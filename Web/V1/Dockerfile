FROM python:3.10-slim

# 2. 작업 디렉토리 설정
WORKDIR /app

# 3. 코드 복사
COPY . /app

RUN echo '\
import websocket\n\
\n\
def on_message(ws, message):\n\
    print("[수신됨]", message)\n\
    print(f"📤 POST 전송 시도: {test_data}")\n\
    res = requests.post("http://127.0.0.1:9359/update", json=test_data)\n\
    print(f"✅ 서버 응답: {res.status_code}, 내용: {res.text}")\n\
\n\
def on_error(ws, error):\n\
    print("[에러]", error)\n\
\n\
def on_close(ws, a, b):\n\
    print("[연결 종료]")\n\
\n\
def on_open(ws):\n\
    print("[연결 성공]")\n\
\n\
ws = websocket.WebSocketApp("ws://hoonservice.iptime.org:12261",\n\
                            on_open=on_open,\n\
                            on_message=on_message,\n\
                            on_error=on_error,\n\
                            on_close=on_close)\n\
ws.run_forever()\n\
' > /app/ws_test.py

# 4. 필요한 패키지 설치
RUN pip install flask requests websocket-client gunicorn

# 5. 포트 개방
EXPOSE 9359

# 6. 실행 명령
CMD ["gunicorn", "-w", "2", "-b", "0.0.0.0:9359", "main:app"]