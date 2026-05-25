#!/bin/bash

set -e

echo "=== Generating Performance Flame Graph ==="
echo

# 检查依赖
if ! command -v perf &> /dev/null; then
    echo "Error: perf not installed"
    echo "Install: sudo apt install linux-tools-generic"
    exit 1
fi

if [ ! -d "$HOME/FlameGraph" ]; then
    echo "Error: FlameGraph not found"
    echo "Clone: git clone https://github.com/brendangregg/FlameGraph.git ~/FlameGraph"
    exit 1
fi

# 参数
PID=${1:-$(pgrep gateway_server | head -1)}
DURATION=${2:-30}
OUTPUT_DIR="./flamegraph-results/$(date +%Y%m%d_%H%M%S)"

if [ -z "$PID" ]; then
    echo "Error: gateway_server not running"
    echo "Usage: $0 [PID] [DURATION]"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

echo "Target PID: $PID"
echo "Duration: ${DURATION}s"
echo "Output: $OUTPUT_DIR"
echo

# 采集性能数据
echo "[1/3] Recording performance data..."
sudo perf record -F 99 -p $PID -g -- sleep $DURATION

# 生成火焰图
echo "[2/3] Generating flame graph..."
sudo perf script | $HOME/FlameGraph/stackcollapse-perf.pl > "$OUTPUT_DIR/out.folded"
$HOME/FlameGraph/flamegraph.pl "$OUTPUT_DIR/out.folded" > "$OUTPUT_DIR/flamegraph.svg"

# 生成性能报告
echo "[3/3] Generating performance report..."
sudo perf report --stdio > "$OUTPUT_DIR/perf_report.txt"

# 清理
sudo rm -f perf.data

echo
echo "=== Flame Graph Generated ==="
echo "SVG: $OUTPUT_DIR/flamegraph.svg"
echo "Report: $OUTPUT_DIR/perf_report.txt"
echo
echo "View in browser:"
echo "  firefox $OUTPUT_DIR/flamegraph.svg"
