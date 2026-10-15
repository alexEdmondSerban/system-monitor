import { DecimalPipe } from '@angular/common';
import { Component, computed, inject, signal } from '@angular/core';
import { toObservable, toSignal } from '@angular/core/rxjs-interop';
import { catchError, combineLatest, of, switchMap, timer } from 'rxjs';

import { ChartSeries, LineChart } from './line-chart/line-chart';
import { Alert, HistoryRange } from './models';
import { MonitorService } from './monitor.service';

type ProcessSort = 'cpu' | 'memory';

@Component({
  selector: 'app-root',
  imports: [LineChart, DecimalPipe],
  templateUrl: './app.html',
  styleUrl: './app.scss',
})
export class App {
  protected readonly monitor = inject(MonitorService);

  protected readonly ranges: HistoryRange[] = ['1h', '6h', '24h', '7d'];
  protected readonly selectedRange = signal<HistoryRange>('1h');
  protected readonly processSort = signal<ProcessSort>('cpu');

  // ---------- Live graphic ----------
  protected readonly liveLabels = computed(() =>
    this.monitor.liveHistory().map((p) => formatTime(p.ts, true)));

  protected readonly liveSeries = computed<ChartSeries[]>(() => {
    const points = this.monitor.liveHistory();
    return [
      { label: 'CPU', color: '#4f9dff', data: points.map((p) => p.cpu) },
      { label: 'RAM', color: '#3ecf8e', data: points.map((p) => p.memPercent) },
    ];
  });

  // ---------- DB History ----------
  private readonly history = toSignal(
    combineLatest([toObservable(this.selectedRange), timer(0, 60_000)]).pipe(
      switchMap(([range]) => this.monitor.getHistory(range).pipe(catchError(() => of(null))))),
    { initialValue: null });

  protected readonly historyLabels = computed(() => {
    const h = this.history();
    if (!h) return [];
    return h.points.map((p) => (h.range === '7d' ? formatDayTime(p.ts) : formatTime(p.ts, false)));
  });

  protected readonly historySeries = computed<ChartSeries[]>(() => {
    const points = this.history()?.points ?? [];
    return [
      { label: 'CPU (medie)', color: '#4f9dff', data: points.map((p) => p.cpu) },
      { label: 'CPU (maxim)', color: '#4f9dff', data: points.map((p) => p.cpuMax), dashed: true },
      { label: 'RAM', color: '#3ecf8e', data: points.map((p) => p.memPercent) },
    ];
  });

  protected readonly historyEmpty = computed(() => (this.history()?.points.length ?? 0) === 0);

  // ---------- Alerts modified when backend signals a change ----------
  protected readonly alerts = toSignal(
    combineLatest([toObservable(this.monitor.alertsVersion), timer(0, 30_000)]).pipe(
      switchMap(() => this.monitor.getAlerts().pipe(catchError(() => of<Alert[]>([]))))),
    { initialValue: [] as Alert[] });

  protected readonly activeAlerts = computed(() => this.alerts().filter((a) => a.endedAt === null).length);

  // ---------- Top proccesses ----------
  protected readonly topProcesses = computed(() => {
    const processes = this.monitor.latest()?.processes ?? [];
    const key = this.processSort() === 'cpu' ? 'cpu' : 'memoryMB';
    return [...processes].sort((a, b) => b[key] - a[key]).slice(0, 10);
  });

  protected alertLabel(alert: Alert): string {
    return alert.metric === 'cpu' ? 'CPU' : 'RAM';
  }

  protected alertWhen(alert: Alert): string {
    return formatDayTime(alert.startedAt);
  }

  protected alertDuration(alert: Alert): string {
    const end = alert.endedAt ?? Date.now();
    const seconds = Math.max(0, Math.round((end - alert.startedAt) / 1000));
    if (seconds < 60) return `${seconds}s`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `${minutes}m ${seconds % 60}s`;
    return `${Math.floor(minutes / 60)}h ${minutes % 60}m`;
  }
}

function pad(n: number): string {
  return n.toString().padStart(2, '0');
}

function formatTime(ts: number, withSeconds: boolean): string {
  const d = new Date(ts);
  const hm = `${pad(d.getHours())}:${pad(d.getMinutes())}`;
  return withSeconds ? `${hm}:${pad(d.getSeconds())}` : hm;
}

function formatDayTime(ts: number): string {
  const d = new Date(ts);
  return `${pad(d.getDate())}.${pad(d.getMonth() + 1)} ${formatTime(ts, false)}`;
}
