#!/bin/bash

THREADS=${1:-4}
SIZE=${2:-200000000}

./timsort -t "$THREADS" -n "$SIZE" &
PID=$!

while kill -0 "$PID" 2>/dev/null; do
    if [ "$(uname)" = "Darwin" ]; then
        COUNT=$(ps -M -p "$PID" 2>/dev/null | tail -n +2 | wc -l | tr -d " ")
    else
        COUNT=$(ps -T -p "$PID" 2>/dev/null | tail -n +2 | wc -l | tr -d " ")
    fi
    if [ "$COUNT" -gt 0 ]; then
        echo "threads of process $PID: $COUNT"
    fi
    sleep 0.2
done
wait "$PID"
