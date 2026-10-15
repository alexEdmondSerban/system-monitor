# Protocol SystemMonitor

## Agent (C++), doar pe 127.0.0.1:8090

### REST
| Metodă | Rută           | Descriere                          |
|--------|----------------|------------------------------------|
| GET    | `/api/health`  | `{"status":"ok","service":"SystemMonitorAgent"}` |
| GET    | `/api/metrics` | Ultimul snapshot (format mai jos)  |

### WebSocket `ws://127.0.0.1:8090/ws/metrics`
La conectare primești imediat ultimul snapshot, apoi câte unul pe secundă.

### Formatul snapshot-ului
```json
{
  "timestamp": 1792059820117,
  "cpu": 12.4,
  "memory": { "usedGB": 9.8, "totalGB": 31.9, "percent": 30.7 },
  "processes": [
    { "pid": 1234, "name": "chrome.exe", "cpu": 3.2, "memoryMB": 412.5 }
  ]
}
```
- `timestamp`: milisecunde de la epoch (UTC)
- `cpu`: procent din tot procesorul (toate nucleele), 0-100
- `processes[].cpu`: procent din tot procesorul, ca în Task Manager
- `processes[].memoryMB`: working set