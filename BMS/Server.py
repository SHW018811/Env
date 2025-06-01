import socket
import numpy as np
from tensorflow.keras.models import load_model
import joblib
import matplotlib.pyplot as plt
from collections import deque

# 모델 및 스케일러 로드
model = load_model("lstm_normal_model.h5", compile=False)
scaler = joblib.load("scaler.save")
SEQ_LEN = 20
threshold = 0.01
sequence = []

# 기록용
error_history = deque(maxlen=100)
x_data, y_data = [], []
temperature_data, voltage_data, current_data = [], [], []
anomaly_flags = []
time_step = 0

# 그래프 설정
plt.ion()
fig, ax1 = plt.subplots()

line1, = ax1.plot([], [], label="Prediction Error", color='blue')
line_anom, = ax1.plot([], [], 'ro', label="Anomaly", markersize=4)
thresh_line = ax1.axhline(y=threshold, color='r', linestyle='--', label="Threshold")

ax1.set_ylim(0, 0.05)
ax1.set_title("📈 실시간 LSTM 이상 탐지")
ax1.set_xlabel("Time Step")
ax1.set_ylabel("Prediction Error")
ax1.grid(True)

# 보조축: 센서값 표시 (온도, 전압, 전류)
ax2 = ax1.twinx()
line_temp, = ax2.plot([], [], label="Temperature (°C)", color='green', alpha=0.5)
line_volt, = ax2.plot([], [], label="Voltage (V)", color='orange', linestyle='--', alpha=0.6)
line_curr, = ax2.plot([], [], label="Current (A)", color='purple', linestyle=':', alpha=0.6)
ax2.set_ylabel("Sensor Data")

fig.legend(loc='upper left')

# 소켓 서버 초기화
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(('localhost', 12345))
server_socket.listen(5)
print("🚀 LSTM 이상 감지 서버 시작...")

# 데이터 수신 루프
while True:
    client_socket, addr = server_socket.accept()
    data = client_socket.recv(1024).decode().strip()
    client_socket.close()

    if data:
        try:
            soc, volt, curr, temp = map(float, data.split(","))
            sequence.append([soc, volt, curr, temp])
            if len(sequence) > SEQ_LEN:
                sequence = sequence[-SEQ_LEN:]
                X = np.array([scaler.transform(sequence)])
                y_pred = model.predict(X)[0]
                y_true = scaler.transform([[soc, volt, curr, temp]])[0]
                error = np.mean(np.abs(y_pred - y_true))

                # 출력
                if error > threshold:
                    print(f"⚠️ 이상 탐지! 오차: {error:.5f}")
                else:
                    print(f"✅ 정상: 오차 {error:.5f}")

                # 데이터 기록
                x_data.append(time_step)
                y_data.append(error)
                anomaly_flags.append(error > threshold)
                temperature_data.append(temp)
                voltage_data.append(volt)
                current_data.append(curr)
                time_step += 1

                # 그래프 업데이트
                line1.set_data(x_data, y_data)
                line_temp.set_data(x_data, temperature_data)
                line_volt.set_data(x_data, voltage_data)
                line_curr.set_data(x_data, current_data)
                anomaly_x = [x_data[i] for i in range(len(x_data)) if anomaly_flags[i]]
                anomaly_y = [y_data[i] for i in range(len(y_data)) if anomaly_flags[i]]
                line_anom.set_data(anomaly_x, anomaly_y)

                ax1.set_xlim(max(0, time_step - 100), time_step + 10)
                ax1.set_ylim(0, max(0.015, max(y_data) * 1.2))
                ax2.set_ylim(
                    min(min(temperature_data), min(voltage_data), min(current_data)) - 1,
                    max(max(temperature_data), max(voltage_data), max(current_data)) + 1
                )
                if time_step % 10 == 0:
                    plt.draw()
                    plt.pause(0.001)

        except Exception as e:
            print(f"⚠️ 데이터 파싱 오류: {e}")