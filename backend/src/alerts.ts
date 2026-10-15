import { EventEmitter } from 'node:events';
import type { MetricsDb } from './db.js';
import type { AlertRule, Snapshot } from './types.js';

interface RuleState {
  aboveSince: number | null;    
  activeAlertId: number | null; 
  peak: number;
}

export class AlertEngine extends EventEmitter {
  private readonly states = new Map<AlertRule, RuleState>();

  constructor(private readonly rules: readonly AlertRule[], private readonly db: MetricsDb) {
    super();
    for (const rule of rules) {
      this.states.set(rule, { aboveSince: null, activeAlertId: null, peak: 0 });
    }
  }

  get activeCount(): number {
    let count = 0;
    for (const state of this.states.values()) if (state.activeAlertId !== null) count++;
    return count;
  }

  evaluate(snapshot: Snapshot): void {
    const now = snapshot.timestamp;

    for (const rule of this.rules) {
      const state = this.states.get(rule)!;
      const value = rule.metric === 'cpu' ? snapshot.cpu : snapshot.memory.percent;

      if (value >= rule.threshold) {
        state.aboveSince ??= now;
        const newPeak = value > state.peak;
        if (newPeak) state.peak = value;

        if (state.activeAlertId === null) {
          if (now - state.aboveSince >= rule.durationSec * 1000) {
            state.activeAlertId = this.db.openAlert(rule.metric, rule.threshold, state.peak, state.aboveSince);
            console.log(`[alerte] ${rule.metric} peste ${rule.threshold}%`);
            this.emit('changed');
          }
        } else if (newPeak) {
          this.db.updateAlertPeak(state.activeAlertId, state.peak);
        }
      } else {
        if (state.activeAlertId !== null) {
          this.db.closeAlert(state.activeAlertId, now);
          console.log(`[alerte] ${rule.metric} a revenit sub ${rule.threshold}%`);
          this.emit('changed');
        }
        state.aboveSince = null;
        state.activeAlertId = null;
        state.peak = 0;
      }
    }
  }
}