#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: child <file name>\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "w");
    if (file == NULL) {
        perror("child: fopen");
        return 1;
    }

    char *line = NULL;
    size_t capacity = 0;

    while (getline(&line, &capacity, stdin) != -1) {
        char *p = line;
        float sum = 0;
        int count = 0;
        int bad = 0;

        while (1) {
            while (isspace((unsigned char)*p)) {
                p++;
            }
            if (*p == '\0') {
                break;
            }

            char *end;
            float number = strtof(p, &end);
            if (end == p) {
                bad = 1;
                break;
            }
            sum += number;
            count++;
            p = end;
        }

        if (bad) {
            printf("error: line contains not a number\n");
            fflush(stdout);
            continue;
        }
        if (count == 0) {
            printf("error: empty line\n");
            fflush(stdout);
            continue;
        }

        if (fprintf(file, "%g\n", sum) < 0 || fflush(file) != 0) {
            perror("child: write to file");
            free(line);
            fclose(file);
            return 1;
        }

        printf("sum = %g\n", sum);
        fflush(stdout);
    }

    free(line);
    fclose(file);
    return 0;
}
