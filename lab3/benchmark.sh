#!/bin/bash
# Замер времени сортировки для разного числа потоков и размеров массива.
# Для каждой пары берётся лучшее время из 3 запусков.
# Выводит таблицу: время, ускорение S = T1 / Tp, эффективность E = S / p.

export LC_ALL=C

SIZES="10000 100000 1000000 10000000 50000000"
THREADS="1 2 4 6 8 10 16 32"

make -s

for n in $SIZES; do
    echo
    echo "### n = $n"
    echo
    echo "| threads | time, ms | speedup | efficiency |"
    echo "|---|---|---|---|"
    base=""
    for t in $THREADS; do
        best=""
        for run in 1 2 3; do
            time=$(./timsort -t "$t" -n "$n" 2>/dev/null | awk '{print $6}')
            if [ -z "$best" ] || awk "BEGIN {exit !($time < $best)}"; then
                best=$time
            fi
        done
        if [ -z "$base" ]; then
            base=$best
        fi
        awk -v t="$t" -v time="$best" -v base="$base" \
            'BEGIN { s = base / time; printf "| %d | %.2f | %.2f | %.2f |\n", t, time, s, s / t }'
    done
done
