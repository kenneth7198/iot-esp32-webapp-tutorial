// Simple Aedes broker with TCP(1883) and WS(9001)
const aedes = require('aedes')()
const net = require('net')
const http = require('http')
const ws = require('ws')

const TCP_PORT = 1883
const WS_PORT = 9001

const server = net.createServer(aedes.handle)
server.listen(TCP_PORT, () => console.log(`[MQTT] TCP broker listening on ${TCP_PORT}`))

const httpServer = http.createServer()
const wss = new ws.Server({ server: httpServer })
wss.on('connection', (socket) => {
  const stream = ws.createWebSocketStream(socket)
  aedes.handle(stream)
})
httpServer.listen(WS_PORT, () => console.log(`[MQTT] WS broker listening on ${WS_PORT}`))

aedes.on('client', c => console.log('[MQTT] client connected:', c ? c.id : 'unknown'))
aedes.on('publish', (p, c) => {
  if (p && p.topic && !p.topic.startsWith('$SYS')) {
    console.log(`[MQTT] ${c ? c.id : 'server'} -> ${p.topic} : ${p.payload}`)
  }
})
