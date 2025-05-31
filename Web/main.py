from flask import Flask, render_template, redirect, request, jsonify
import random
import subprocess
import signal
import datetime
import os

# ── LSTM 관련 임포트 추가 ──
import joblib
import numpy as np
from collections import deque

app = Flask(__name__)

# ── 기존 전역 변수 ──
socket_data = {}
charging = 2
test_socket_process = None
connector_stat = True
stat = ""
enable = False

Car_stat: dict = {
    "VIN" : "", #Car vin info
    "Model" : "", #Car model info
    "Connect" : "커넥터 연결 해제됨", #Default connector stat
    "Alert" : False #Modal alram stat
}

# 현재 파일(__file__) 위치: Env/Web/main.py
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # → Env 폴더
MODEL_DIR = os.path.join(BASE_DIR, "LSTM", "models")
LSTM_MODEL_PATH = os.path.join(MODEL_DIR, "lstm_pred.h5")
SCALER_PATH     = os.path.join(MODEL_DIR, "scaler.pkl")
THRESHOLD_PATH  = os.path.join(MODEL_DIR, "threshold.npy")
# 미리 로드
try:
    import tensorflow as tf
    lstm_model = tf.keras.models.load_model(LSTM_MODEL_PATH, compile=False)
    scaler = joblib.load(SCALER_PATH)
    threshold = float(np.load(THRESHOLD_PATH))
    print(f"[LSTM] 모델, 스케일러, threshold 로드 완료 (threshold={threshold:.6f})")
except Exception as e:
    print(f"[LSTM] 모델 로드 실패: {e}")
    lstm_model = None
    scaler = None
    threshold = None

# 시퀀스 길이와 피처 설정
SEQ_LEN = 120
FEATURE_KEYS = ["Voltage_terminal", "SOC", "Temperature", "Charge_Current"]
FEATURE_DIM = len(FEATURE_KEYS)

# 고정 길이 버퍼 생성
buffer_deque = deque(maxlen=SEQ_LEN)
@app.route('/') #main page router
def index():
    global socket_data
    global connector_stat
    if charging == 2:
        if(connector_stat):
            Car_stat["VIN"] = ""
            Car_stat["Model"] = ""
            Car_stat["Connect"] = "🔌 커넥터 연결 해제됨"
            connector_stat = False
            stat = "🔌 충전기 연결 필요"
        else:
            Car_stat["VIN"] = "0x12345678"
            Car_stat["Model"] = "🚘 Ionic 5"
            Car_stat["Connect"] = "🔌 커넥터 연결됨"
            connector_stat = True
            stat = "🟠 충전 대기"
    elif charging == 1:
        if(connector_stat):
            stat = "🟢 충전 중" #추후 아이콘 추가 예정
        else:
            stat = "🔌 충전기 연결 필요"
            Car_stat["Alert"] = True
    elif charging == 0:
        stat = "🔴 충전 중단"
    return render_template('index.html',datas=stat,info=Car_stat, data=socket_data) #Use template load index.html

@app.route("/connector", methods = ['POST'])
def connector(): #if connected connector -> used this
    global charging
    charging = 2
    return redirect('/')

@app.route('/start', methods=['POST'])  # 충전 시작 버튼
def start():
    global charging, test_socket_process, enable
    charging = 1  # 충전 중 상태로 전환
    if enable == False:
        try:
            print("[DEBUG] Launching testsocket.py...")
            path = os.path.join(os.getcwd(), "testsocket.py")
            # print(f"[DEBUG] Full path: {path}")
            test_socket_process = subprocess.Popen(["python3", path])
            enable = True
        except Exception as e:
            print(f"[ERROR] Failed to launch testsocket.py: {e}")
    else:
        print("이미 실행중")
    return redirect('/') #Click the 충전 시작 button will redirect to the main

@app.route('/stop', methods=['POST'])
def stop():
    global charging, test_socket_process,socket_data, enable
    charging = 0  # 충전 중단 상태로 전환
    socket_data={}
    enable = False
    if test_socket_process is not None:
        test_socket_process.send_signal(signal.SIGINT)
        test_socket_process.wait(timeout=5)
        test_socket_process = None

    return redirect('/') #Click the 충전 중단 button will redirect to the main

@app.route('/monitor', methods=['POST']) #monitor page
def monitoring():
    return render_template('monier.html')

@app.route('/update/data', methods=['GET'])
def Senddata():
    return jsonify(socket_data)

@app.route('/update', methods=['POST'])
def Update_data():
    global socket_data, buffer_deque, lstm_model, scaler, threshold

    # 1) 들어온 JSON 저장
    incoming = request.get_json()
    socket_data = incoming.copy()

    # 2) 피처 벡터 생성
    try:
        feature_vector = np.array([
            float(incoming.get("Voltage_terminal", 0)),
            float(incoming.get("SOC", 0)),
            float(incoming.get("Temperature", 0)),
            float(incoming.get("Charge_Current", 0))
        ], dtype=float)
    except Exception as e:
        print(f"[LSTM] Feature 벡터 생성 오류: {e}")
        return jsonify({"status": "invalid data"}), 400

    # 3) 버퍼에 추가
    buffer_deque.append(feature_vector)

    # 4) 버퍼가 가득 차면 예측 수행
    if (
        lstm_model is not None
        and scaler is not None
        and threshold is not None
        and len(buffer_deque) == SEQ_LEN
    ):
        arr = np.array(buffer_deque)  # shape: (SEQ_LEN, FEATURE_DIM)
        arr_scaled = scaler.transform(arr)
        input_seq = arr_scaled.reshape((1, SEQ_LEN, FEATURE_DIM))

        # LSTM 예측
        pred = lstm_model.predict(input_seq, verbose=0).flatten()
        actual_scaled = scaler.transform(arr[-1:].reshape(1, -1)).flatten()

        # 평균 절대 오차 계산
        err = np.mean(np.abs(pred - actual_scaled))

        if err > threshold:
            print(f"[LSTM][Anomaly] err={err:.6f} > threshold={threshold:.6f}")
            # 필요 시 추가 처리 (예: WebSocket으로 STOP_CHARGE 전송)

    return jsonify({"status": "received"})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=9359, debug=False) #debug mode -> Turn off debug EXPO
