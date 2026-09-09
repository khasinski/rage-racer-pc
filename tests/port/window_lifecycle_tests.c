#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int Run(const char *const argv[], char *output, size_t outputSize) {
    int pipefd[2];
    pid_t pid;
    size_t used = 0;
    if (pipe(pipefd) != 0) return 0;
    pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    close(pipefd[1]);
    if (pid < 0) { close(pipefd[0]); return 0; }
    while (used + 1 < outputSize) {
        ssize_t count = read(pipefd[0], output + used, outputSize - used - 1);
        if (count <= 0) break;
        used += (size_t)count;
    }
    output[used] = '\0';
    close(pipefd[0]);
    int status;
    return waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static int FindWindow(char *window, size_t size) {
    const char *command[] = { "xdotool", "search", "--name", "Rage Racer", NULL };
    char output[1024];
    char *line;
    if (!Run(command, output, sizeof(output))) return 0;
    line = strtok(output, "\r\n");
    if (line == NULL) return 0;
    do {
        if (*line != '\0') {
            if (strlen(line) + 1 > size) return 0;
            strcpy(window, line);
        }
        line = strtok(NULL, "\r\n");
    } while (line != NULL);
    return window[0] != '\0';
}

static int Xdo(const char *action, const char *window, const char *first, const char *second) {
    const char *command[] = { "xdotool", action, window, first, second, NULL };
    char output[1024];
    return Run(command, output, sizeof(output));
}

int main(int argc, char **argv) {
    char directory[] = "/tmp/rage-window-lifecycle-XXXXXX";
    char scenario[4096], logPath[4096], window[128] = "", *log;
    FILE *file;
    pid_t game;
    int status = 0;
    if (argc != 3 || mkdtemp(directory) == NULL) return 2;
    if (snprintf(scenario, sizeof(scenario), "%s/scenario.ini", directory) >= (int)sizeof(scenario) ||
        snprintf(logPath, sizeof(logPath), "%s/game.log", directory) >= (int)sizeof(logPath)) return 2;
    file = fopen(scenario, "w");
    if (file == NULL) return 2;
    fputs("[video]\nrenderer = modern\n\n[diagnostics]\nrenderer_lifecycle = true\n\n[race]\nenabled = true\nmode = grand-prix\nseries = gp\nclass = 0\ncourse = 0\ncar = 3\n\n[run]\nframes = 2500\n\n[stop]\nscene = 12\ntimer = 120\n", file);
    if (fclose(file) != 0) return 2;
    game = fork();
    if (game == 0) {
        if (chdir(argv[2]) != 0 || setenv("SDL_AUDIODRIVER", "dummy", 1) != 0 ||
            freopen(logPath, "w", stdout) == NULL || freopen(logPath, "a", stderr) == NULL)
            _exit(127);
        execl(argv[1], argv[1], "--scenario", scenario, (char *)NULL);
        _exit(127);
    }
    if (game < 0) return 2;
    for (unsigned attempt = 0; attempt < 60 && !FindWindow(window, sizeof(window)); ++attempt) {
        if (waitpid(game, &status, WNOHANG) == game) break;
        usleep(100000);
    }
    if (window[0] == '\0') { kill(game, SIGKILL); waitpid(game, NULL, 0); return 1; }
    if (!Xdo("windowsize", window, "800", "600") ||
        !Xdo("windowsize", window, "1024", "576") ||
        !Xdo("windowsize", window, "640", "480") ||
        !Xdo("key", "--window", window, "F4") || !Xdo("key", "--window", window, "F4")) {
        kill(game, SIGKILL); waitpid(game, NULL, 0); return 1;
    }
    for (unsigned attempt = 0; attempt < 1650; ++attempt) {
        if (waitpid(game, &status, WNOHANG) == game) break;
        usleep(100000);
    }
    if (waitpid(game, &status, WNOHANG) == 0) { kill(game, SIGKILL); waitpid(game, NULL, 0); return 1; }
    file = fopen(logPath, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) return 1;
    long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0 || (log = malloc((size_t)size + 1)) == NULL) return 1;
    if (fread(log, 1, (size_t)size, file) != (size_t)size) return 1;
    log[size] = '\0';
    fclose(file);
    int ok = WIFEXITED(status) && WEXITSTATUS(status) == 0 && strstr(log, "scene 12") != NULL &&
        strstr(log, "resource setup failed") == NULL && strstr(log, "failed to enter fullscreen") == NULL;
    const char *first = strstr(log, "modern resources created generation=");
    if (first != NULL && strstr(first + 1, "modern resources created generation=") != NULL) ok = 0;
    free(log);
    return ok ? 0 : 1;
}
