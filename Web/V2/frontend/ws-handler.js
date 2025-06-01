export function startWebSocket(onSOCUpdate, onTempUpdate, onVoltageUpdate) {
    const ws = new WebSocket("ws://hoonservice.iptime.org:12261");

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);

            if (data.id == 1574) {
                const soc = data.data[0];
                const soh = data.data[5];
                onSOCUpdate(soc, soh);
            }

            if (data.id == 1575) {
                const batteryTemp = data.data[0];
                const externalTemp = data.data[1];
                onTempUpdate(batteryTemp, externalTemp);
            }

            if (data.id == 1576) {
                const voltage = data.data[0];
                onVoltageUpdate(voltage);
            }
        } catch (err) {
            console.error("WebSocket message error:", err);
        }
    };
}