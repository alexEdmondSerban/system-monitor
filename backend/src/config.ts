import type { AlertRule } from './types.js';

export const config = {
  port: Number(process.env.PORT ?? 3000),
  agentUrl: process.env.AGENT_URL ?? 'ws://127.0.0.1:8090/ws/metrics',
  dbPath: process.env.DB_PATH ?? 'data/metrics.db',

  // Save in db the median of 5 seconds
  sampleIntervalSec: 5,
  retentionDays: 7,

  alertRules: [
    { metric: 'cpu', threshold: 90, durationSec: 30 },
    { metric: 'memory', threshold: 90, durationSec: 30 },
  ] satisfies AlertRule[],
};