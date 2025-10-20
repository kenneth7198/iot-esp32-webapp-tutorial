// WebSocket 伺服器 - 處理前端事件並與 MQTT Broker 通訊
const WebSocket = require('ws');
const mqtt = require('mqtt');

// 設定
const WS_PORT = 8080;  // WebSocket 伺服器端口（給前端用）
const MQTT_BROKER = 'mqtt://localhost:1883';  // Mosquitto MQTT Broker

// MQTT 主題
const TOPIC_LED_CONTROL = 'sensor/LED';
const TOPIC_LED_STATUS = 'status/led';

// 建立 WebSocket 伺服器
const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`[WebSocket] 伺服器運行於 ws://localhost:${WS_PORT}`);

// 連接到 MQTT Broker
const mqttClient = mqtt.connect(MQTT_BROKER);

mqttClient.on('connect', () => {
  console.log('[MQTT] 已連接到 Mosquitto Broker');
  
  // 訂閱 ESP32 的狀態主題
  mqttClient.subscribe(TOPIC_LED_STATUS, (err) => {
    if (!err) {
      console.log(`[MQTT] 已訂閱主題: ${TOPIC_LED_STATUS}`);
    }
  });
});

mqttClient.on('error', (err) => {
  console.error('[MQTT] 連接錯誤:', err.message);
});

// 儲存所有連接的 WebSocket 客戶端
const clients = new Set();

// WebSocket 連接處理
wss.on('connection', (ws, req) => {
  const clientIp = req.socket.remoteAddress;
  console.log(`[WebSocket] 新客戶端連接: ${clientIp}`);
  
  clients.add(ws);

  // 發送歡迎訊息
  ws.send(JSON.stringify({
    type: 'welcome',
    message: '已連接到 WebSocket 伺服器',
    timestamp: new Date().toISOString()
  }));

  // 接收來自前端的訊息
  ws.on('message', (data) => {
    try {
      const message = JSON.parse(data);
      console.log(`[WebSocket] 收到訊息:`, message);

      // 根據訊息類型處理
      switch (message.type) {
        case 'led_control':
          // 轉發 LED 控制指令到 MQTT
          handleLedControl(message.payload);
          break;

        case 'get_status':
          // 請求當前狀態
          handleGetStatus(ws);
          break;

        case 'shake_detected':
          // 處理搖晃事件
          handleShakeEvent(message);
          break;

        default:
          ws.send(JSON.stringify({
            type: 'error',
            message: `未知的訊息類型: ${message.type}`
          }));
      }
    } catch (err) {
      console.error('[WebSocket] 解析訊息錯誤:', err.message);
      ws.send(JSON.stringify({
        type: 'error',
        message: '無效的 JSON 格式'
      }));
    }
  });

  // 客戶端斷線
  ws.on('close', () => {
    console.log(`[WebSocket] 客戶端斷線: ${clientIp}`);
    clients.delete(ws);
  });

  ws.on('error', (err) => {
    console.error('[WebSocket] 錯誤:', err.message);
  });
});

// 處理 LED 控制
function handleLedControl(payload) {
  console.log('[控制] LED 控制指令:', payload);
  
  // 發送到 MQTT (給 ESP32)
  const mqttPayload = JSON.stringify(payload);
  mqttClient.publish(TOPIC_LED_CONTROL, mqttPayload, { qos: 1 }, (err) => {
    if (err) {
      console.error('[MQTT] 發送失敗:', err.message);
    } else {
      console.log(`[MQTT] 已發送到 ${TOPIC_LED_CONTROL}:`, mqttPayload);
      
      // 廣播給所有 WebSocket 客戶端
      broadcast({
        type: 'led_command_sent',
        payload: payload,
        timestamp: new Date().toISOString()
      });
    }
  });
}

// 處理狀態查詢
function handleGetStatus(ws) {
  console.log('[查詢] 客戶端請求狀態');
  
  // 這裡可以從資料庫或快取獲取狀態
  // 目前簡單回傳一個示例
  ws.send(JSON.stringify({
    type: 'status_response',
    status: {
      connected: mqttClient.connected,
      timestamp: new Date().toISOString()
    }
  }));
}

// 處理搖晃事件
function handleShakeEvent(message) {
  console.log('[事件] 偵測到搖晃事件:', message);
  
  // 自動開啟 LED
  handleLedControl({ GPIO23: 'on' });
  
  // 記錄事件（可以存入資料庫）
  const event = {
    type: 'shake_event',
    timestamp: new Date().toISOString(),
    action: 'LED turned on by shake'
  };
  
  // 廣播給所有客戶端
  broadcast(event);
}

// 廣播訊息給所有連接的客戶端
function broadcast(data) {
  const message = JSON.stringify(data);
  clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(message);
    }
  });
}

// 接收來自 MQTT 的訊息（ESP32 狀態更新）
mqttClient.on('message', (topic, payload) => {
  console.log(`[MQTT] 收到訊息 ${topic}:`, payload.toString());
  
  // 轉發給所有 WebSocket 客戶端
  broadcast({
    type: 'mqtt_message',
    topic: topic,
    payload: payload.toString(),
    timestamp: new Date().toISOString()
  });
});

// 優雅關閉
process.on('SIGINT', () => {
  console.log('\n[系統] 正在關閉伺服器...');
  
  // 關閉所有 WebSocket 連接
  clients.forEach((client) => {
    client.close();
  });
  
  // 斷開 MQTT
  mqttClient.end();
  
  // 關閉 WebSocket 伺服器
  wss.close(() => {
    console.log('[系統] 伺服器已關閉');
    process.exit(0);
  });
});
