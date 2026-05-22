#!/bin/bash

set -e

PORT=${1:-8080}
CONNECTIONS=${2:-100}
DURATION=${3:-30}

echo "=== Multi-threaded Performance Comparison ==="
echo "Port: $PORT"
echo "Connections: $CONNECTIONS"
echo "Duration: ${DURATION}s"
echo

# Test different thread counts
for THREADS in 1 2 4 8; do
    echo "----------------------------------------"
    echo "Testing with $THREADS thread(s)..."
    echo "----------------------------------------"

    # Start server
    ./build/gateway_server $PORT $THREADS > /tmp/server_${THREADS}t.log 2>&1 &
    SERVER_PID=$!
    sleep 2

    # Check if server started
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo "Error: Server failed to start"
        cat /tmp/server_${THREADS}t.log
        continue
    fi

    # Run benchmark
    echo "Spawning $CONNECTIONS connections..."
    PIDS=()
    ITERATIONS=$((DURATION / 5))
    START_TIME=$(date +%s.%N)

    for i in $(seq 1 $CONNECTIONS); do
        bash -c "
            exec 3<>/dev/tcp/localhost/$1
            cat <&3 > /dev/null &
            for j in \$(seq 1 $2); do
                echo 'PING' >&3
                sleep 5
            done
            echo 'QUIT' >&3
            exec 3>&-
        " -- "$PORT" "$ITERATIONS" 2>/dev/null &
        PIDS+=($!)
    done

    # Wait for connections to establish
    sleep 2

    # Monitor performance
    echo "Monitoring for ${DURATION}s..."
    SAMPLES=()
    for i in $(seq 1 $((DURATION / 5))); do
        sleep 5
        ACTIVE=$(netstat -an 2>/dev/null | grep ":$PORT" | grep ESTABLISHED | wc -l)
        SAMPLES+=($ACTIVE)
        echo "  [${i}*5s] Active connections: $ACTIVE"
    done

    END_TIME=$(date +%s.%N)
    ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)

    # Calculate average
    TOTAL=0
    for val in "${SAMPLES[@]}"; do
        TOTAL=$((TOTAL + val))
    done
    AVG=$((TOTAL / ${#SAMPLES[@]}))

    echo "Results:"
    echo "  - Elapsed time: ${ELAPSED}s"
    echo "  - Average active connections: $AVG"
    echo "  - Peak connections: $(printf '%s\n' "${SAMPLES[@]}" | sort -rn | head -1)"

    # Cleanup
    for pid in "${PIDS[@]}"; do
        kill $pid 2>/dev/null || true
    done
    kill $SERVER_PID 2>/dev/null || true
    wait 2>/dev/null || true

    sleep 2
    echo
done

echo "=== Comparison Complete ==="
echo "Check /tmp/server_*t.log for detailed server logs"
