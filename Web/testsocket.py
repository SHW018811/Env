import requests
import time
import os
import re
import websocket
import threading
import json

psdata = []


def on_message(ws, message):
    try:
        msg = json.loads(message)
        can_id = f"{msg.get('id'):03X}"
        data_list = msg.get("data", [])
        data_hex = ''.join(f'{byte:02X}' for byte in data_list)
        lst = [data_hex[i:i+2] for i in range(0, len(data_hex), 2)]
        chg = ""
        if can_id in idmap:
            chg = idmap[can_id](lst)
            # print(f"ID: {can_id} -> {data_hex} ({chg})")
            res = requests.post("http://127.0.0.1:9359/update", json=test_data)
            # print(f"POST {res.status_code}")
    except Exception as e:
        print(f"Error: {e}")

def on_error(ws, error):
    print(f"WebSocket error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("WebSocket connection closed")

def on_open(ws):
    print("WebSocket connection established")

def run_websocket():
    websocket.enableTrace(False)
    ws = websocket.WebSocketApp("ws://hoonservice.iptime.org:12261",
                                on_open=on_open,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)
    ws.run_forever()

# Run websocket in background thread
ws_thread = threading.Thread(target=run_websocket)
ws_thread.start()


def id620(lst):
    return bytes.fromhex(''.join(lst)).decode('ascii')
def id621(lst):
    return bytes.fromhex(''.join(lst)).decode('ascii')
def id622(lst):
    return f"Status: {int(lst[0],16)} // Time: {int(lst[2]+lst[1], 16)}sec // Flags: {int(lst[3], 16)} // DTC: {int(lst[5] + lst[4], 16)}"
def id623(lst):
    test_data["Voltage"] = ((int(lst[1]+lst[0],16)))
    return f"Voltage: {int(lst[1]+lst[0],16)}V // MinVolt: {int(lst[2], 16)*0.1}V // MinVoltID: {int(lst[3], 16)} // MaxVolt: {int(lst[4],16)*0.1}V // MaxVoltID: {int(lst[5], 16)}"
def id624(lst):
    return f"Current: {int(lst[1]+lst[0],16)}A // ChgLmt: {int(lst[3]+lst[2],16)}A // DischgLmt: {int(lst[5]+lst[4],16)}A"
def id626(lst):
    test_data["SOC"] = int(lst[0],16)
    test_data["DOD"] = int(lst[2] + lst[1], 16)
    test_data["SOH"] = int(lst[5],16)
    return f"SOC: {int(lst[0],16)}% // DoD: {int(lst[2]+lst[1],16)}Ah // Capacity: {int(lst[4]+lst[3],16)}Ah // SOH: {int(lst[5],16)}"
def id627(lst):
    test_data["Temp"] = int(lst[0],16)
    return f"Temperature: {int(lst[0],16)}C // AirTemp: {int(lst[1],16)}C // MinTemp: {int(lst[2],16)}C // MinTempID: {int(lst[3],16)} // MaxTemp: {int(lst[4],16)}C // MaxTempID: {int(lst[5],16)}"
def id628(lst):
    return f"Resistance: {int(lst[1]+lst[0],16)} Om // MinResistance: {int(lst[2],16)} Om // MinResistanceID: {int(lst[3],16)} // MaxResistance: {int(lst[4],16)} Om // MaxResistanceID: {int(lst[5],16)}"
def id629(lst):
    return f"DCLineVoltage: {int(lst[1]+lst[0],16)} // DCLinCurrent: {int(lst[3]+lst[2],16)} // MaxChargeCurrent: {int(lst[4],16)} // MaxDischargeCurrent: {int(lst[5],16)} // DCLinePower: {int(lst[7]+lst[6],16)}"

idmap = {
    "620": id620,
    "621": id621,
    "622": id622,
    "623": id623,
    "624": id624,
    "626": id626,
    "627": id627,
    "628": id628,
    "629": id629
}


# 테스트용 임의 데이터
test_data = {
    "SOC": 0,
    "DOD": 100,
    "SOH": 0,
    "Temp": 0,
    "Voltage" : 0
}
