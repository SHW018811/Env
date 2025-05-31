from flask import Flask, render_template, redirect, request, jsonify
import random
import subprocess
import signal
import datetime
import os

app = Flask(__name__)

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
    global socket_data
    socket_data = request.get_json()
    return jsonify({"status": "received"})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=9359, debug=False) #debug mode -> Turn off debug EXPO
