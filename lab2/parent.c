#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int writeAll(int fd, const char *text, size_t length) {
    size_t written = 0;
    while (written < length) {
        ssize_t result = write(fd, text + written, length - written);
        if (result == -1) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }
        written += result;
    }
    return 1;
}

int readLine(int fd, char *buffer, int size) {
    int length = 0;
    char c;
    while (1) {
        ssize_t result = read(fd, &c, 1);
        if (result == -1) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }
        if (result == 0) {
            buffer[length] = '\0';
            return length > 0;
        }
        if (c == '\n') {
            buffer[length] = '\0';
            return 1;
        }
        if (length < size - 1) {
            buffer[length] = c;
            length++;
        }
    }
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    char *fileName = NULL;
    size_t capacity = 0;

    printf("Enter file name: ");
    fflush(stdout);
    ssize_t nameLength = getline(&fileName, &capacity, stdin);
    if (nameLength == -1) {
        fprintf(stderr, "error: file name is empty\n");
        free(fileName);
        return 1;
    }
    if (nameLength > 0 && fileName[nameLength - 1] == '\n') {
        fileName[nameLength - 1] = '\0';
    }
    if (fileName[0] == '\0') {
        fprintf(stderr, "error: file name is empty\n");
        free(fileName);
        return 1;
    }

    int pipe1[2];
    int pipe2[2];
    if (pipe(pipe1) == -1) {
        perror("pipe1");
        free(fileName);
        return 1;
    }
    if (pipe(pipe2) == -1) {
        perror("pipe2");
        close(pipe1[0]);
        close(pipe1[1]);
        free(fileName);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        free(fileName);
        return 1;
    }

    if (pid == 0) {
        if (dup2(pipe1[0], STDIN_FILENO) == -1 || dup2(pipe2[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(1);
        }
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        execl("./child", "child", fileName, (char *)NULL);
        perror("execl ./child");
        _exit(1);
    }

    close(pipe1[0]);
    close(pipe2[1]);

    printf("Enter numbers separated by spaces (Ctrl+D to finish):\n");

    char *line = NULL;
    size_t lineCapacity = 0;
    ssize_t lineLength;
    char answer[256];

    while ((lineLength = getline(&line, &lineCapacity, stdin)) != -1) {
        if (!writeAll(pipe1[1], line, lineLength)) {
            perror("write to pipe1");
            break;
        }
        if (line[lineLength - 1] != '\n' && !writeAll(pipe1[1], "\n", 1)) {
            perror("write to pipe1");
            break;
        }
        if (!readLine(pipe2[0], answer, sizeof(answer))) {
            fprintf(stderr, "error: child process does not answer\n");
            break;
        }
        printf("%s\n", answer);
    }

    free(line);
    close(pipe1[1]);
    close(pipe2[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        free(fileName);
        return 1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "error: child process finished with error\n");
        free(fileName);
        return 1;
    }

    printf("Results are saved to %s\n", fileName);
    free(fileName);
    return 0;
}
