#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stop_flag = 0;
static volatile sig_atomic_t child_flag = 0;
static pid_t child_pid = -1;
static int restart_on_failure = 1;

static void on_signal(int sig) {
    if (sig == SIGCHLD) child_flag = 1;
    else stop_flag = 1;
}

static void install_handlers(void) {
    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
}

static pid_t spawn(char **cmd) {
    int pipefd[2];
    if (pipe(pipefd) != 0) { perror("pipe"); return -1; }
    pid_t pid = fork();
    if (pid == 0) {
        setpgid(0, 0);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execvp(cmd[0], cmd);
        perror("execvp");
        _exit(127);
    }
    if (pid < 0) { perror("fork"); close(pipefd[0]); close(pipefd[1]); return -1; }
    close(pipefd[1]);
    fprintf(stderr, "[supervisor] started pid=%ld\n", (long)pid);
    char buf[512];
    ssize_t n;
    while (!stop_flag && (n = read(pipefd[0], buf, sizeof buf)) > 0) {
        fwrite(buf, 1, (size_t)n, stdout);
        fflush(stdout);
        if (child_flag) break;
    }
    close(pipefd[0]);
    return pid;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s [--never-restart] command [args...]\n", argv[0]);
        return 2;
    }
    int idx = 1;
    if (strcmp(argv[idx], "--never-restart") == 0) { restart_on_failure = 0; idx++; }
    if (idx >= argc) return 2;
    install_handlers();
    unsigned backoff = 1;
    while (!stop_flag) {
        child_flag = 0;
        child_pid = spawn(&argv[idx]);
        if (child_pid < 0) return 1;
        int status = 0;
        while (!stop_flag) {
            pid_t r = waitpid(child_pid, &status, 0);
            if (r == child_pid) break;
            if (r < 0 && errno == EINTR) continue;
            if (r < 0) { perror("waitpid"); break; }
        }
        if (stop_flag) {
            if (child_pid > 0) kill(-child_pid, SIGTERM);
            waitpid(child_pid, NULL, 0);
            break;
        }
        bool failed = !WIFEXITED(status) || WEXITSTATUS(status) != 0;
        fprintf(stderr, "[supervisor] child exited status=%d\n", status);
        if (!restart_on_failure || !failed) break;
        fprintf(stderr, "[supervisor] restarting in %u second(s)\n", backoff);
        struct timespec ts = {.tv_sec=(time_t)backoff,.tv_nsec=0};
        nanosleep(&ts, NULL);
        if (backoff < 8) backoff *= 2;
    }
    fprintf(stderr, "[supervisor] stopped\n");
    return 0;
}
