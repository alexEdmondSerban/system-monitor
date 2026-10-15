import { Component, ElementRef, OnDestroy, effect, input, viewChild } from '@angular/core';
import { Chart, ChartDataset, registerables } from 'chart.js';

Chart.register(...registerables);
Chart.defaults.color = '#8b98a8';
Chart.defaults.borderColor = '#2a3542';

export interface ChartSeries {
  label: string;
  color: string;
  data: number[];
  dashed?: boolean;
}

@Component({
  selector: 'app-line-chart',
  template: `<div class="chart-box"><canvas #canvas></canvas></div>`,
  styles: `.chart-box { position: relative; height: 240px; }`,
})
export class LineChart implements OnDestroy {
  readonly labels = input.required<string[]>();
  readonly series = input.required<ChartSeries[]>();
  readonly yMax = input(100);

  private readonly canvas = viewChild<ElementRef<HTMLCanvasElement>>('canvas');
  private chart?: Chart<'line', number[], string>;

  constructor() {
    effect(() => {
      const canvas = this.canvas();
      const labels = this.labels();
      const datasets = this.series().map(toDataset);
      if (!canvas) return;

      if (!this.chart) {
        this.chart = new Chart<'line', number[], string>(canvas.nativeElement, {
          type: 'line',
          data: { labels, datasets },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            interaction: { mode: 'index', intersect: false },
            elements: { point: { radius: 0 }, line: { tension: 0.3, borderWidth: 2 } },
            scales: {
              y: { min: 0, max: this.yMax(), ticks: { callback: (v) => `${v}%` } },
              x: { ticks: { maxTicksLimit: 8, maxRotation: 0 } },
            },
            plugins: { legend: { labels: { boxWidth: 12 } } },
          },
        });
      } else {
        this.chart.data.labels = labels;
        this.chart.data.datasets = datasets;
        this.chart.update('none');
      }
    });
  }

  ngOnDestroy(): void {
    this.chart?.destroy();
  }
}

function toDataset(s: ChartSeries): ChartDataset<'line', number[]> {
  return {
    label: s.label,
    data: s.data,
    borderColor: s.color,
    backgroundColor: s.color,
    borderDash: s.dashed ? [5, 4] : [],
  };
}
