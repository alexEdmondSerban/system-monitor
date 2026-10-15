import { DatabaseSync, type StatementSync } from 'node:sqlite';
import { mkdirSync } from 'node:fs';
import { dirname } from 'node:path';
import type { Alert, HistoryPoint, MetricName } from './types.js';

export class MetricsDb {
  private readonly db: DatabaseSync;
  private readonly insertMetricStmt: StatementSync;
  private readonly historyStmt: StatementSync;
  private readonly pruneStmt: StatementSync;
  private readonly openAlertStmt: StatementSync;
  private readonly updatePeakStmt: StatementSync;
  private readonly closeAlertStmt: StatementSync;
  private readonly listAlertsStmt: StatementSync;

  constructor(path: string) {
    mkdirSync(dirname(path), { recursive: true });
    this.db = new DatabaseSync(path);

    this.db.exec(`
      PRAGMA journal_mode = WAL;

      CREATE TABLE IF NOT EXISTS metrics (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        ts          INTEGER NOT NULL,
        cpu         REAL    NOT NULL,
        mem_percent REAL    NOT NULL,
        mem_used_gb REAL    NOT NULL
      );
      CREATE INDEX IF NOT EXISTS idx_metrics_ts ON metrics(ts);

      CREATE TABLE IF NOT EXISTS alerts (
        id         INTEGER PRIMARY KEY AUTOINCREMENT,
        metric     TEXT    NOT NULL,
        threshold  REAL    NOT NULL,
        peak       REAL    NOT NULL,
        started_at INTEGER NOT NULL,
        ended_at   INTEGER
      );
    `);


    this.db.prepare('UPDATE alerts SET ended_at = CAST(? AS INTEGER) WHERE ended_at IS NULL').run(Date.now());

    this.insertMetricStmt = this.db.prepare(
      'INSERT INTO metrics (ts, cpu, mem_percent, mem_used_gb) VALUES (CAST(? AS INTEGER), ?, ?, ?)');


    this.historyStmt = this.db.prepare(`
      SELECT (ts / CAST(? AS INTEGER)) * CAST(? AS INTEGER) AS bucket,
             ROUND(AVG(cpu), 1)         AS cpu,
             ROUND(MAX(cpu), 1)         AS cpuMax,
             ROUND(AVG(mem_percent), 1) AS memPercent
      FROM metrics
      WHERE ts >= CAST(? AS INTEGER)
      GROUP BY bucket
      ORDER BY bucket`);

    this.pruneStmt = this.db.prepare('DELETE FROM metrics WHERE ts < CAST(? AS INTEGER)');

    this.openAlertStmt = this.db.prepare(
      'INSERT INTO alerts (metric, threshold, peak, started_at) VALUES (?, ?, ?, CAST(? AS INTEGER))');
    this.updatePeakStmt = this.db.prepare('UPDATE alerts SET peak = ? WHERE id = CAST(? AS INTEGER)');
    this.closeAlertStmt = this.db.prepare(
      'UPDATE alerts SET ended_at = CAST(? AS INTEGER) WHERE id = CAST(? AS INTEGER)');
    this.listAlertsStmt = this.db.prepare(`
      SELECT id, metric, threshold, ROUND(peak, 1) AS peak,
             started_at AS startedAt, ended_at AS endedAt
      FROM alerts
      ORDER BY started_at DESC
      LIMIT CAST(? AS INTEGER)`);
  }

  insertMetric(ts: number, cpu: number, memPercent: number, memUsedGB: number): void {
    this.insertMetricStmt.run(ts, cpu, memPercent, memUsedGB);
  }

  history(fromTs: number, bucketMs: number): HistoryPoint[] {
    const rows = this.historyStmt.all(bucketMs, bucketMs, fromTs) as Array<{
      bucket: number; cpu: number; cpuMax: number; memPercent: number;
    }>;
    return rows.map((r) => ({ ts: r.bucket, cpu: r.cpu, cpuMax: r.cpuMax, memPercent: r.memPercent }));
  }

  prune(olderThanTs: number): void {
    this.pruneStmt.run(olderThanTs);
  }

  openAlert(metric: MetricName, threshold: number, peak: number, startedAt: number): number {
    const result = this.openAlertStmt.run(metric, threshold, peak, startedAt);
    return Number(result.lastInsertRowid);
  }

  updateAlertPeak(id: number, peak: number): void {
    this.updatePeakStmt.run(peak, id);
  }

  closeAlert(id: number, endedAt: number): void {
    this.closeAlertStmt.run(endedAt, id);
  }

  listAlerts(limit: number): Alert[] {
    return this.listAlertsStmt.all(limit) as unknown as Alert[];
  }

  close(): void {
    this.db.close();
  }
}