import express from 'express';
import { createServer } from 'node:http';
import { WebSocket, WebSocketServer } from 'ws';

import { AgentBridge } from './agentBridge.js';
import { AlertEngine } from './alerts.js';
import { config } from './config.js';
import { MetricsDb } from './db.js';
import { createRoutes } from './routes.js';
import type { Snapshot } from './types.js';

const db = new MetricsDb(config.dbPath);
const agent = new AgentBridge(config.agentUrl);
const alerts = new AlertEngine(config.alertRules, db);

// ---------- HTTP (REST) ----------
const app = express();
app.use(express.json());
app.use('/api', createRoutes({ db, agent, alerts }));

const server = createServer(app);

// ---------- WebSocket to Angular ----------
const wss = new WebSocketServer({ server, path: '/ws/live' });

function broadcast(message: object): void {
  const data = JSON.stringify(message);
  for (const client of wss.clients) {
    if (client.readyState === WebSocket.OPEN) client.send(data);
  }
}

wss.on('connection', (socket) => {
  socket.send(JSON.stringify({ type: 'status', agentConnected: agent.isConnected }));
  const latest = agent.latestSnapshot;
  if (latest) socket.send(JSON.stringify({ type: 'snapshot', data: latest }));
});

// ---------- Data from agent ----------
let acc = { cpu: 0, memPercent: 0, memUsedGB: 0, count: 0, since: 0 };

agent.on('snapshot', (snapshot: Snapshot) => {
  broadcast({ type: 'snapshot', data: snapshot });
  alerts.evaluate(snapshot);

  if (acc.count === 0) acc.since = snapshot.timestamp;
  acc.cpu += snapshot.cpu;
  acc.memPercent += snapshot.memory.percent;
  acc.memUsedGB += snapshot.memory.usedGB;
  acc.count++;

  if (snapshot.timestamp - acc.since >= config.sampleIntervalSec * 1000) {
    db.insertMetric(snapshot.timestamp, acc.cpu / acc.count, acc.memPercent / acc.count, acc.memUsedGB / acc.count);
    acc = { cpu: 0, memPercent: 0, memUsedGB: 0, count: 0, since: 0 };
  }
});

agent.on('status', (connected: boolean) => broadcast({ type: 'status', agentConnected: connected }));
alerts.on('changed', () => broadcast({ type: 'alerts-changed' }));

setInterval(() => db.prune(Date.now() - config.retentionDays * 24 * 3600_000), 3600_000);

// ---------- Start / Stop ----------
server.listen(config.port, '127.0.0.1', () => {
  console.log(`[backend] pornit pe http://127.0.0.1:${config.port}`);
  agent.start();
});

process.on('SIGINT', () => {
  console.log('\n[backend] stops...');
  wss.close();
  server.close();
  db.close();
  process.exit(0);
});