import requests
import threading
import json
import time

import can  # pip install python-can
from websocket import WebSocketApp  # pip install websocket-client

# ── 1) 설정 부분 ──
# WebSocket 브로커 주소 (ESP32 펌웨어의 WS_SERVER, WS_PORT와 동일)
WS_URI = "ws://hoonservice.iptime.org:12261"

# 가상 CAN 인터페이스 이름 (vcan0)
CAN_CHANNEL = "vcan0"
CAN_BUSTYPE = "socketcan"

# WebSocketApp 인스턴스 전역 변수
ws_app = None



# ── 충전 상태 감시 및 CAN 제어 루프 ──
def charge_control_loop():
    """
    주기적으로 http://localhost:9359/ 에 GET 요청을 보내
    '🟢 충전 중' 또는 '🔴 충전 중단' 상태를 감지하여,
    충전 상태가 변하면 CAN ID=0x010 제어 프레임을 전송한다.
    """
    prev_charging = None
    bus = None
    try:
        bus = can.Bus(channel="vcan0", bustype="socketcan")
    except Exception as e:
        print(f"[CHG_CTRL] CAN bus open failed: {e}")
        return
    print("[CHG_CTRL] Charge control loop started.")
    while True:
        try:
            resp = requests.get("http://localhost:9359/")
            if resp.status_code == 200:
                txt = resp.text
                if "🟢 충전 중" in txt:
                    curr_charging = True
                elif "🔴 충전 중단" in txt:
                    curr_charging = False
                else:
                    curr_charging = prev_charging
            else:
                curr_charging = prev_charging
        except Exception as e:
            print(f"[CHG_CTRL] HTTP req error: {e}")
            curr_charging = prev_charging
        if prev_charging is not None and curr_charging is not None and prev_charging != curr_charging:
            try:
                if curr_charging:
                    frame = can.Message(arbitration_id=0x010, data=bytes([0x01]), is_extended_id=False, dlc=1)
                    bus.send(frame)
                    print("[CHG_CTRL] Sent CAN: START_CHARGE (0x01)")
                else:
                    frame = can.Message(arbitration_id=0x010, data=bytes([0x00]), is_extended_id=False, dlc=1)
                    bus.send(frame)
                    print("[CHG_CTRL] Sent CAN: STOP_CHARGE (0x00)")
            except Exception as e:
                print(f"[CHG_CTRL] CAN send error: {e}")
        prev_charging = curr_charging
        time.sleep(0.5)

# ── 2) CAN → WebSocket 역할 함수 ──
def can_rx_to_ws_loop():
    """
    vcan0으로부터 CAN 프레임을 수신하면 JSON 문자열로 포맷팅하여
    WebSocket 서버로 전송한다.
    """
    try:
        bus = can.Bus(channel=CAN_CHANNEL, bustype=CAN_BUSTYPE)
    except Exception as e:
        print(f"[CAN_RX] vcan0 오픈 실패: {e}")
        return

    print(f"[CAN_RX] Listening on {CAN_CHANNEL}...")

    while True:
        try:
            msg = bus.recv(timeout=1.0)  # 타임아웃 1초
        except Exception as e:
            print(f"[CAN_RX] recv 에러: {e}")
            break

        if msg is None:
            continue

        # CAN ID와 DLC, Data 바이트 추출
        can_id = msg.arbitration_id
        dlc = msg.dlc
        data_bytes = list(msg.data)  # 예: [0x01, 0xFF, ...]

        # JSON 포맷 (main.cpp에서 사용하던 형식과 최대한 유사하게)
        # 예시: {"type":"CAN","id":1572,"dlc":6,"data":[1,255, ...]}
        can_json = {
            "type": "CAN",
            "id": can_id,
            "dlc": dlc,
            "data": data_bytes
        }

        txt = json.dumps(can_json)

        # WebSocket 연결이 열려 있으면 전송
        if ws_app and ws_app.sock and ws_app.sock.connected:
            try:
                ws_app.send(txt)
                # 출력 예시: [WS_SEND] {"type":"CAN","id":1572,...}
                print(f"[WS_SEND] {txt}")
            except Exception as e:
                print(f"[WS_SEND] 전송 실패: {e}")
                # 재연결할 수 있도록 잠시 멈춤
                time.sleep(1)
        else:
            # 아직 WebSocket이 연결되지 않았거나, 끊겼으면 잠시 대기
            time.sleep(0.2)


# ── 3) WebSocket 메시지 수신 콜백 ──
def on_ws_message(ws, message):
    """
    WebSocket 서버로부터 수신된 메시지가 이 함수에 전달된다.
    JSON으로 파싱하여 "type":"CMD"일 때, CAN 프레임 형식(CAN ID=0x010, 데이터=[0/1])로
    vcan0에 전송한다.
    """
    try:
        msg = json.loads(message)
    except json.JSONDecodeError:
        print(f"[WS_RX] JSON 파싱 실패: {message}")
        return

    mtype = msg.get("type")
    if mtype != "CMD":
        # CAN 데이터(예: type=="CAN")나 다른 메시지가 들어오면 무시
        return

    act = msg.get("act")
    if act is None:
        return

    # bms 시뮬레이터에서 처리하던 CAN ID=0x010 (충전 제어)
    can_id = 0x010
    if act == "STOP_CHARGE":
        data_byte = 0x00
        print("[WS_RX] STOP_CHARGE 명령 수신")
    elif act == "START_CHARGE":
        data_byte = 0x01
        print("[WS_RX] START_CHARGE 명령 수신")
    else:
        # 그 외 CMD(ex. RESET_BATTERY 등)를 직접 추가하려면 아래와 같이 분기
        # 예시:
        # elif act == "RESET_BATTERY":
        #     can_id = 0x11
        #     data_byte = 0x01
        #     print("[WS_RX] RESET_BATTERY 명령 수신")
        return

    # CAN 프레임 생성 후 vcan0으로 전송
    frame = can.Message(
        arbitration_id=can_id,
        data=bytes([data_byte]),
        is_extended_id=False,
        dlc=1
    )
    try:
        tx_bus = can.Bus(channel=CAN_CHANNEL, bustype=CAN_BUSTYPE)
        tx_bus.send(frame)
        print(f"[CAN_TX] Sent: ID=0x{can_id:03X}, DATA=[{data_byte:02X}]")
        tx_bus.shutdown()
    except Exception as e:
        print(f"[CAN_TX] 전송 실패: {e}")


def on_ws_error(ws, error):
    print(f"[WS_ERR] {error}")


def on_ws_close(ws, close_status_code, close_msg):
    print("[WS_CLOSE] 연결 종료")


def on_ws_open(ws):
    print("[WS_OPEN] WebSocket 연결 성공")


# ── 4) WebSocket 연결 스레드 함수 ──
def run_websocket_client():
    """
    웹소켓 클라이언트를 생성하여 서버(WS_URI)에 연결한다.
    연결이 수립되면 on_ws_open(), 수신 메시지는 on_ws_message()로 콜백된다.
    """
    global ws_app

    # WebSocketApp 생성
    ws_app = WebSocketApp(
        WS_URI,
        on_open=on_ws_open,
        on_message=on_ws_message,
        on_error=on_ws_error,
        on_close=on_ws_close
    )

    # run_forever()는 blocking 호출이므로, 이 함수는 별도 스레드에서 실행해야 한다.
    ws_app.run_forever(ping_interval=5, ping_timeout=3)


# ── 5) 메인 진입점 ──
if __name__ == "__main__":
    # 충전 상태 감시/제어 루프 스레드 시작
    t_charge = threading.Thread(target=charge_control_loop, daemon=True)
    t_charge.start()
    # 1) WebSocket 연결 스레드 시작
    t_ws = threading.Thread(target=run_websocket_client, daemon=True)
    t_ws.start()

    # 2) CAN → WebSocket 역할 스레드 시작
    t_canrx = threading.Thread(target=can_rx_to_ws_loop, daemon=True)
    t_canrx.start()

    print("=== ESP32 시뮬레이터 시작 ===")
    print(f"- WebSocket 서버: {WS_URI}")
    print(f"- CAN 인터페이스: {CAN_CHANNEL}")
    print(" * 종료하려면 Ctrl+C를 누르세요.\n")

    try:
        # 두 스레드는 데몬 모드로 돌아가므로, 메인 스레드를 무한 대기 상태로 둔다.
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[Main] 종료 신호(Ctrl+C) 감지, 프로그램을 종료합니다.")
        # WebSocketApp을 안전하게 종료하려면 아래와 같이 할 수 있다.
        if ws_app:
            ws_app.close()
        time.sleep(0.5)