// WebSocket 伺服器 + HTTP 靜態檔案伺服器
const WebSocket = require('ws');
const mqtt = require('mqtt');
const http = require('http');
const fs = require('fs');
const path = require('path');

// 設定
const HTTP_PORT = 8081;  // HTTP 伺服器端口（給靜態檔案用）
const WS_PORT = 8080;    // WebSocket 伺服器端口（給前端用）
const MQTT_BROKER = 'mqtt://localhost:1883';  // Mosquitto MQTT Broker

// MQTT 主題
const TOPIC_LED_CONTROL = 'sensor/LED';
const TOPIC_LED_STATUS = 'status/led';

// MIME types
const mimeTypes = {
  '.html': 'text/html',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.gif': 'image/gif',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon'
};

// 建立 HTTP 伺服器（提供靜態檔案）
const httpServer = http.createServer((req, res) => {
  // 處理根路徑
  let filePath = req.url === '/' ? '/index.html' : req.url;
  
  // 建立完整路徑
  filePath = path.join(__dirname, '..', 'web', filePath);
  
  // 取得副檔名
  const extname = path.extname(filePath);
  const contentType = mimeTypes[extname] || 'text/plain';
  
  // 讀取並回傳檔案
  fs.readFile(filePath, (err, content) => {
    if (err) {
      if (err.code === 'ENOENT') {
        res.writeHead(404, { 'Content-Type': 'text/html' });
        res.end('<h1>404 - 檔案不存在</h1>', 'utf-8');
      } else {
        res.writeHead(500);
        res.end(`伺服器錯誤: ${err.code}`, 'utf-8');
      }
    } else {
      res.writeHead(200, { 'Content-Type': contentType });
      res.end(content, 'utf-8');
    }
  });
});

httpServer.listen(HTTP_PORT, '0.0.0.0', () => {
  // 取得本機 IP
  const networkInterfaces = require('os').networkInterfaces();
  const addresses = [];
  
  for (const name of Object.keys(networkInterfaces)) {
    for (const net of networkInterfaces[name]) {
      if (net.family === 'IPv4' && !net.internal) {
        addresses.push(net.address);
      }
    }
  }
  
  console.log('\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
  console.log('🌐 HTTP 靜態檔案伺服器已啟動');
  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
  console.log(`📱 手機訪問: http://192.168.100.200:${HTTP_PORT}/esp32-led-control.html`);
  console.log(`💻 本機訪問: http://localhost:${HTTP_PORT}/esp32-led-control.html`);
  if (addresses.length > 0) {
    addresses.forEach(addr => {
      console.log(`🔗 網路訪問: http://${addr}:${HTTP_PORT}/esp32-led-control.html`);
    });
  }
  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');
});

// 建立 WebSocket 伺服器
const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`[WebSocket] 伺服器運行於 ws://0.0.0.0:${WS_PORT}`);

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
  
  // 訂閱所有 ESP32 設備的光線數據
  mqttClient.subscribe('esp32/+/light', (err) => {
    if (!err) {
      console.log('[MQTT] 已訂閱主題: esp32/+/light');
    }
  });
  
  // 訂閱所有鬼魂移動指令
  mqttClient.subscribe('ghost/move/#', (err) => {
    if (!err) {
      console.log('[MQTT] 已訂閱主題: ghost/move/#');
    }
  });
  
  // 訂閱觸控數據
  mqttClient.subscribe('esp32/+/touch', (err) => {
    if (!err) {
      console.log('[MQTT] 已訂閱主題: esp32/+/touch');
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

        case 'subscribe':
          // 動態訂閱 MQTT 主題
          if (message.topic) {
            mqttClient.subscribe(message.topic, (err) => {
              if (!err) {
                console.log(`[MQTT] 客戶端訂閱: ${message.topic}`);
              }
            });
          }
          break;

        case 'publish':
          // 發布 MQTT 訊息（用於鬼魂移動等）
          if (message.topic && message.payload) {
            mqttClient.publish(message.topic, message.payload, { qos: 1 }, (err) => {
              if (!err) {
                console.log(`[MQTT] 發布到 ${message.topic}`);
              }
            });
          }
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
  const payloadStr = payload.toString();
  
  // 簡化日誌輸出（光線數據頻繁，不完整顯示）
  if (topic.includes('/light')) {
    console.log(`[MQTT] 💡 ${topic}`);
  } else if (topic.includes('/move')) {
    console.log(`[MQTT] 🎮 ${topic}: ${payloadStr}`);
  } else {
    console.log(`[MQTT] 收到訊息 ${topic}:`, payloadStr);
  }
  
  // 轉發給所有 WebSocket 客戶端
  broadcast({
    type: 'mqtt_message',
    topic: topic,
    payload: payloadStr,
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
  
  // 關閉 HTTP 伺服器
  httpServer.close(() => {
    console.log('[HTTP] HTTP 伺服器已關閉');
  });
  
  // 關閉 WebSocket 伺服器
  wss.close(() => {
    console.log('[WebSocket] WebSocket 伺服器已關閉');
    console.log('[系統] 所有服務已關閉');
    process.exit(0);
  });
});
