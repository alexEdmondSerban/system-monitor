import { Router } from 'express';
import type { AgentBridge } from './agentBridge.js';
import type { AlertEngine } from './alerts.js';
import type { MetricsDb } from './db.js';

const MINUTE = 60_000;
const HOUR = 60 * MINUTE;

const RANGES: Record<string, { durationMs: number; bucketMs: number }> = {
  '1h':  { durationMs: HOUR,        bucketMs: MINUTE },
  '6h':  { durationMs: 6 * HOUR,    bucketMs: 5 * MINUTE },
  '24h': { durationMs: 24 * HOUR,   bucketMs: 15 * MINUTE },
  '7d':  { durationMs: 168 * HOUR,  bucketMs: 2 * HOUR },
};

export function createRoutes(deps: { db: MetricsDb; agent: AgentBridge; alerts: AlertEngine }): Router {
  const { db, agent, alerts } = deps;
  const router = Router();

  router.get('/status', (_req, res) => {
    res.json({
      agentConnected: agent.isConnected,
      lastSnapshotAt: agent.latestSnapshot?.timestamp ?? null,
      activeAlerts: alerts.activeCount,
    });
  });

  router.get('/metrics/latest', (_req, res) => {
    const latest = agent.latestSnapshot;
    if (!latest) {
      res.status(503).json({ error: 'I didnt receive data from agent yet' });
      return;
    }
    res.json(latest);
  });

  router.get('/metrics/history', (req, res) => {
    const range = String(req.query.range ?? '1h');
    const settings = RANGES[range];
    if (!settings) {
      res.status(400).json({ error: `Invalid interval.Use: ${Object.keys(RANGES).join(', ')}` });
      return;
    }
    const points = db.history(Date.now() - settings.durationMs, settings.bucketMs);
    res.json({ range, bucketMs: settings.bucketMs, points });
  });

  router.get('/alerts', (req, res) => {
    const limit = Math.min(Math.max(Number(req.query.limit) || 20, 1), 200);
    res.json(db.listAlerts(limit));
  });

  return router;
}