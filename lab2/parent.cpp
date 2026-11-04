#include <cerrno>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

bool writeAll(int fd, const std::string& text) {
    size_t written = 0;
    while (written < text.size()) {
        ssize_t result = write(fd, text.c_str() + written, text.size() - written);
        if (result == -1) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        written += result;
    }
    return true;
}

bool readLine(int fd, std::string& line) {
    line.clear();
    char c;
    while (true) {
        ssize_t result = read(fd, &c, 1);
        if (result == -1) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (result == 0) {
            return !line.empty();
        }
        if (c == '\n') {
            return true;
        }
        line += c;
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    std::string fileName;
    std::cout << "Enter file name: " << std::flush;
    if (!std::getline(std::cin, fileName) || fileName.empty()) {
        std::cerr << "error: file name is empty" << std::endl;
        return 1;
    }

    int pipe1[2];
    int pipe2[2];
    if (pipe(pipe1) == -1) {
        perror("pipe1");
        return 1;
    }
    if (pipe(pipe2) == -1) {
        perror("pipe2");
        close(pipe1[0]);
        close(pipe1[1]);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
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

        execl("./child", "child", fileName.c_str(), (char*)NULL);
        perror("execl ./child");
        _exit(1);
    }

    close(pipe1[0]);
    close(pipe2[1]);

    std::cout << "Enter numbers separated by spaces (Ctrl+D to finish):" << std::endl;

    std::string line;
    std::string answer;
    while (std::getline(std::cin, line)) {
        if (!writeAll(pipe1[1], line + "\n")) {
            perror("write to pipe1");
            break;
        }
        if (!readLine(pipe2[0], answer)) {
            std::cerr << "error: child process does not answer" << std::endl;
            break;
        }
        std::cout << answer << std::endl;
    }

    close(pipe1[1]);
    close(pipe2[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::cerr << "error: child process finished with error" << std::endl;
        return 1;
    }

    std::cout << "Results are saved to " << fileName << std::endl;
    return 0;
}
