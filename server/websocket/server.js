// Simple WebSocket Server - замена Soketi
const { Server } = require('socket.io');
const http = require('http');

const PORT = process.env.PORT || 6001;
const APP_ID = process.env.APP_ID || 'hydro-app';
const APP_KEY = process.env.APP_KEY || 'hydro-app-key';
const APP_SECRET = process.env.APP_SECRET || 'hydro-app-secret';

const httpServer = http.createServer();
const io = new Server(httpServer, {
  cors: {
    origin: '*',
    methods: ['GET', 'POST'],
    credentials: true
  },
  transports: ['websocket', 'polling'],
  allowUpgrades: true,
  pingTimeout: 30000,
  pingInterval: 25000
});

console.log('🚀 WebSocket Server starting...');
console.log(`📡 Port: ${PORT}`);
console.log(`🔑 App ID: ${APP_ID}`);

// Namespaces для совместимости с Laravel Broadcasting
const namespaces = new Map();

io.on('connection', (socket) => {
  console.log(`✅ Client connected: ${socket.id}`);

  // Subscribe to channel
  socket.on('subscribe', (data) => {
    const channel = data.channel;
    console.log(`📻 Subscribe to channel: ${channel}`);
    socket.join(channel);
    socket.emit('subscription_succeeded', { channel });
  });

  // Unsubscribe from channel
  socket.on('unsubscribe', (data) => {
    const channel = data.channel;
    console.log(`📴 Unsubscribe from channel: ${channel}`);
    socket.leave(channel);
  });

  // Client events
  socket.on('client-event', (data) => {
    console.log(`📤 Client event:`, data);
    socket.to(data.channel).emit(data.event, data.data);
  });

  socket.on('disconnect', (reason) => {
    console.log(`❌ Client disconnected: ${socket.id}, reason: ${reason}`);
  });

  socket.on('error', (error) => {
    console.error(`🔥 Socket error: ${error.message}`);
  });
});

// HTTP API для Laravel Broadcasting
httpServer.on('request', (req, res) => {
  // Health check
  if (req.url === '/' || req.url === '/health') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      status: 'ok',
      app_id: APP_ID,
      connections: io.engine.clientsCount,
      uptime: process.uptime()
    }));
    return;
  }

  // Broadcast event (from Laravel)
  if (req.url === `/apps/${APP_ID}/events` && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => {
      body += chunk.toString();
    });
    req.on('end', () => {
      try {
        const event = JSON.parse(body);
        console.log(`📡 Broadcasting event: ${event.name} to ${event.channels || event.channel}`);
        
        const channels = event.channels || [event.channel];
        channels.forEach(channel => {
          io.to(channel).emit(event.name, event.data);
        });

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ status: 'ok' }));
      } catch (error) {
        console.error(`❌ Broadcast error: ${error.message}`);
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: error.message }));
      }
    });
    return;
  }

  // 404
  res.writeHead(404);
  res.end('Not Found');
});

httpServer.listen(PORT, () => {
  console.log(`✅ WebSocket Server listening on port ${PORT}`);
  console.log(`🌐 Health check: http://localhost:${PORT}/health`);
});

// Graceful shutdown
process.on('SIGTERM', () => {
  console.log('📴 SIGTERM signal received: closing HTTP server');
  httpServer.close(() => {
    console.log('✅ HTTP server closed');
    process.exit(0);
  });
});

process.on('SIGINT', () => {
  console.log('📴 SIGINT signal received: closing HTTP server');
  httpServer.close(() => {
    console.log('✅ HTTP server closed');
    process.exit(0);
  });
});

