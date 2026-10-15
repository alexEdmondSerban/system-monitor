import { HttpClient } from '@angular/common/http';
import { Injectable, inject, signal } from '@angular/core';
import { Observable } from 'rxjs';

import { Alert, HistoryRange, HistoryResponse, LiveMessage, LivePoint, Snapshot } from './models';

const LIVE_POINTS = 60;

@Injectable({ providedIn: 'root' })
export class MonitorService {
  private readonly http = inject(HttpClient);

  // Live status updated from WebSocket
  readonly backendConnected = signal(false);
  readonly agentConnected = signal(false);
  readonly latest = signal<Snapshot | null>(null);
  readonly liveHistory = signal<LivePoint[]>([]);
  readonly alertsVersion = signal(0);

  constructor() {
    this.connect();
  }

  getHistory(range: HistoryRange): Observable<HistoryResponse> {
    return this.http.get<HistoryResponse>('/api/metrics/history', { params: { range } });
  }

  getAlerts(limit = 20): Observable<Alert[]> {
    return this.http.get<Alert[]>('/api/alerts', { params: { limit } });
  }

  private connect(): void {
    const protocol = location.protocol === 'https:' ? 'wss' : 'ws';
    const socket = new WebSocket(`${protocol}://${location.host}/ws/live`);

    socket.onopen = () => this.backendConnected.set(true);
    socket.onmessage = (event) => this.handleMessage(JSON.parse(event.data) as LiveMessage);
    socket.onclose = () => {
      this.backendConnected.set(false);
      this.agentConnected.set(false);
      setTimeout(() => this.connect(), 2000);
    };
  }

  private handleMessage(message: LiveMessage): void {
    switch (message.type) {
      case 'snapshot': {
        const s = message.data;
        this.latest.set(s);
        this.liveHistory.update((points) =>
          [...points, { ts: s.timestamp, cpu: s.cpu, memPercent: s.memory.percent }].slice(-LIVE_POINTS));
        break;
      }
      case 'status':
        this.agentConnected.set(message.agentConnected);
        break;
      case 'alerts-changed':
        this.alertsVersion.update((v) => v + 1);
        break;
    }
  }
}
