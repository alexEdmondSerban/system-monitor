import { EventEmitter } from 'node:events';
import WebSocket from 'ws';
import type { Snapshot } from './types.js';

//  WebSocket client to C++ agent. Emits:
//   'snapshot' (Snapshot) - at every message received
//   'status'   (boolean)  - when connection status changes
export class AgentBridge extends EventEmitter {
  private connected = false;
  private latest: Snapshot | null = null;
  private reconnectTimer: NodeJS.Timeout | null = null;

  constructor(private readonly url: string) {
    super();
  }

  get isConnected(): boolean {
    return this.connected;
  }

  get latestSnapshot(): Snapshot | null {
    return this.latest;
  }

  start(): void {
    this.connect();
  }

  private connect(): void {
    const ws = new WebSocket(this.url);

    ws.on('open', () => {
      console.log(`[agent] connected at ${this.url}`);
      this.setConnected(true);
    });

    ws.on('message', (data) => {
      try {
        const snapshot = JSON.parse(data.toString()) as Snapshot;
        if (typeof snapshot.cpu !== 'number') return;
        this.latest = snapshot;
        this.emit('snapshot', snapshot);
      } catch {
        
      }
    });

    ws.on('close', () => {
      if (this.connected) console.log('[agent] deconnected, retry...');
      this.setConnected(false);
      this.scheduleReconnect();
    });

    ws.on('error', () => {});
  }

  private scheduleReconnect(): void {
    if (this.reconnectTimer) return;
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.connect();
    }, 2000);
  }

  private setConnected(value: boolean): void {
    if (this.connected === value) return;
    this.connected = value;
    this.emit('status', value);
  }
}