export interface ProcessInfo {
  pid: number;
  name: string;
  cpu: number;
  memoryMB: number;
}

export interface Snapshot {
  timestamp: number;
  cpu: number;
  memory: { usedGB: number; totalGB: number; percent: number };
  processes: ProcessInfo[];
}

export interface Alert {
  id: number;
  metric: 'cpu' | 'memory';
  threshold: number;
  peak: number;
  startedAt: number;
  endedAt: number | null;
}

export type HistoryRange = '1h' | '6h' | '24h' | '7d';

export interface HistoryPoint {
  ts: number;
  cpu: number;
  cpuMax: number;
  memPercent: number;
}

export interface HistoryResponse {
  range: HistoryRange;
  bucketMs: number;
  points: HistoryPoint[];
}

export interface LivePoint {
  ts: number;
  cpu: number;
  memPercent: number;
}

export type LiveMessage =
  | { type: 'snapshot'; data: Snapshot }
  | { type: 'status'; agentConnected: boolean }
  | { type: 'alerts-changed' };
