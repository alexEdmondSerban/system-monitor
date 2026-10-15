// Fromat of messages from agent
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

export type MetricName = 'cpu' | 'memory';

export interface AlertRule {
  metric: MetricName;
  threshold: number;   // percent
  durationSec: number;
}

export interface Alert {
  id: number;
  metric: MetricName;
  threshold: number;
  peak: number;
  startedAt: number;
  endedAt: number | null;
}

export interface HistoryPoint {
  ts: number;
  cpu: number;
  cpuMax: number;
  memPercent: number;
}