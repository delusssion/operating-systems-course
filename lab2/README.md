# Lab 2 — processes and pipes (variant 2)

Parent reads a file name, creates `pipe1` and `pipe2` and starts `child`
(`fork` + `execl`). Child's stdin is `pipe1`, child's stdout is `pipe2`.
The parent sends lines like `1.5 2 3` to the child, the child sums the float
numbers, writes the sum to the file and sends the answer back through `pipe2`.

```
make
./parent              # interactive, finish with Ctrl+D
./parent < input.txt  # example input
cat result.txt
```

Run from this directory: the parent starts `./child`.
