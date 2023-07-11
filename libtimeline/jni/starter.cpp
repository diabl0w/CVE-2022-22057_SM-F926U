#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <dirent.h>
#include <ctime>
#include <cstring>
#include <libgen.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <cerrno>
#include <string_view>
#include <termios.h>
#include <experimental/string>
#include <sys/wait.h>
#include <asm-generic/fcntl.h>
#include <fstream>
//#include "android.h"
#include "misc.h"
//#include "selinux.h"
//#include "cgroup.h"
#include "logging.h"
#include <fcntl.h>

#define perrorf(...) fprintf(stderr, __VA_ARGS__)

#define EXIT_FATAL_FORK 4
#define EXIT_FATAL_APP_PROCESS 5
#define EXIT_FATAL_UID 6
#define EXIT_FATAL_KILL 9

#define SERVER_NAME "timeline_server"

#if defined(__arm__)
#define ABI "armeabi-v7a"
#elif defined(__i386__)
#define ABI "x86"
#elif defined(__x86_64__)
#define ABI "x86_64"
#elif defined(__aarch64__)
#define ABI "arm64"
#endif


static void start_server(const char *process_name) {
    if (daemon(false, false) == 0) {
        LOGD("Running Timeline as Init Child");
        if (execv("/system/bin/nice -n -10 /data/local/tmp/timeline", NULL)) {
            exit(EXIT_FATAL_APP_PROCESS);
        }
    } else {
        perrorf("fatal: can't fork\n");
        exit(EXIT_FATAL_FORK);
    }
}

/*
static int check_selinux(const char *s, const char *t, const char *c, const char *p) {
    int res = se::selinux_check_access(s, t, c, p, nullptr);
#ifndef DEBUG
    if (res != 0) {
#endif
    printf("info: selinux_check_access %s %s %s %s: %d\n", s, t, c, p, res);
    fflush(stdout);
#ifndef DEBUG
    }
#endif
    return res;
}

static int switch_cgroup() {
    int s_cuid, s_cpid;
    int spid = getpid();

    if (cgroup::get_cgroup(spid, &s_cuid, &s_cpid) != 0) {
        printf("warn: can't read cgroup\n");
        fflush(stdout);
        return -1;
    }

    printf("info: cgroup is /uid_%d/pid_%d\n", s_cuid, s_cpid);
    fflush(stdout);

    if (cgroup::switch_cgroup(spid, -1, -1) != 0) {
        printf("warn: can't switch cgroup\n");
        fflush(stdout);
        return -1;
    }

    if (cgroup::get_cgroup(spid, &s_cuid, &s_cpid) != 0) {
        printf("info: switch cgroup succeeded\n");
        fflush(stdout);
        return 0;
    }

    printf("warn: can't switch self, current cgroup is /uid_%d/pid_%d\n", s_cuid, s_cpid);
    fflush(stdout);
    return -1;
}
char *context = nullptr;
*/

int starter_main(int argc, char *argv[]) {
    
    int uid = getuid();
    if (uid != 0 && uid != 2000 && uid != 1000) {
        perrorf("fatal: run Shizuku from non root nor adb user (uid=%d).\n", uid);
        exit(EXIT_FATAL_UID);
    }

    //se::init();
    
    LOGE("starter begin");
    printf("info: starter begin\n");
    fflush(stdout);

    // kill old server
    LOGE("killing old process");
    printf("info: killing old process...\n");
    fflush(stdout);

    foreach_proc([](pid_t pid) {
        if (pid == getpid()) return;

        char name[1024];
        if (get_proc_name(pid, name, 1024) != 0) return;

        if (strcmp(SERVER_NAME, name) != 0
            && strcmp("shizuku_server_legacy", name) != 0)
            return;

        if (kill(pid, SIGKILL) == 0)
            printf("info: killed %d (%s)\n", pid, name);
        else if (errno == EPERM) {
            perrorf("fatal: can't kill %d, please try to stop existing Shizuku from app first.\n", pid);
            exit(EXIT_FATAL_KILL);
        } else {
            printf("warn: failed to kill %d (%s)\n", pid, name);
        }
    });

    printf("info: starting server...\n");
    fflush(stdout);
    LOGD("start_server");
    start_server(SERVER_NAME);
    exit(EXIT_SUCCESS);
}

using main_func = int (*)(int, char *[]);

static main_func applet_main[] = {starter_main, nullptr};

static int fork_daemon(int returnParent) {
    pid_t child = fork();
    if (child == 0) { // 1st child
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        int devNull = open("/dev/null", O_RDWR);
        dup2(devNull, STDIN_FILENO);
        dup2(devNull, STDOUT_FILENO);
        dup2(devNull, STDERR_FILENO);
        close(devNull);

        setsid();
        pid_t child2 = fork();
        if (child2 == 0) { // 2nd child
            return 0; // return execution to caller
        } else if (child2 > 0) { // 1st child, fork ok
            exit(EXIT_SUCCESS);
        } else if (child2 < 0) { // 1st child, fork fail
            LOGE("2nd fork failed (%d)", errno);
            exit(EXIT_FAILURE);
        }
    }

    // parent
    if (child < 0) {
        LOGE("1st fork failed (%d)", errno);
        return -1; // error on 1st fork
    }
    while (true) {
        int status;
        pid_t waited = waitpid(child, &status, 0);
        if ((waited == child) && WIFEXITED(status)) {
            break;
        }
    }
    if (!returnParent) exit(EXIT_SUCCESS);
    return 1; // success parent
}

int sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    if ((nanosleep(&ts,&ts) == -1) && (errno == EINTR)) {
        int ret = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
        if (ret < 1) ret = 1;
        return ret;
    }
    return 0;
}

int main(int argc, char **argv) {
    std::string_view base = basename(argv[0]);
    printf("running timeline \n");
    fflush(stdout);
    LOGD("applet %s", base.data());

    constexpr const char *applet_names[] = {"timeline", nullptr, "libtimeline.so"};

    if (fork_daemon(0) == 0) {
        LOGD("Daemonized");
        //for (int i = 0; i < 16; i++) {
            starter_main(argc, argv);
            sleep_ms(16);
        //}
    }

    return 1;

    return 1;
}
