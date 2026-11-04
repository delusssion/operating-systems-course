#!/bin/bash
# Показывает, сколько потоков использует программа, с помощью ps.
# usage: ./show_threads.sh <max threads> [array size]

THREADS=${1:-4}
SIZE=${2:-200000000}

./timsort -t "$THREADS" -n "$SIZE" &
PID=$!

# пока программа работает, раз в 0.2 секунды считаем её потоки
while kill -0 "$PID" 2>/dev/null; do
    if [ "$(uname)" = "Darwin" ]; then
        # macOS: ps -M выводит по строке на каждый поток (+1 строка заголовка)
        COUNT=$(ps -M -p "$PID" 2>/dev/null | tail -n +2 | wc -l | tr -d " ")
    else
        # Linux: ps -T выводит по строке на каждый поток
        COUNT=$(ps -T -p "$PID" 2>/dev/null | tail -n +2 | wc -l | tr -d " ")
    fi
    if [ "$COUNT" -gt 0 ]; then
        echo "threads of process $PID: $COUNT"
    fi
    sleep 0.2
done
wait "$PID"
