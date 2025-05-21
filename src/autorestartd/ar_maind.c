//
// Created by sheverdin on 6/24/24.
//

#include "ar_mainApp.h"
#include <signal.h>
#include <sys/ucontext.h>
#include <pthread.h>
#include "init_module.h"
#include "app_control.h"

#define MAX_STACK_FRAMES 50
#define LOG_FILE "/var/log/tf_autorestart_debug.log"

#ifdef HAVE_EXECINFO_H
    #include <execinfo.h>
#endif

volatile sig_atomic_t keepRunning = 1;
pthread_mutex_t shutdownMutex = PTHREAD_MUTEX_INITIALIZER;

static void graceful_shutdown_handler(int sig);

static void print_stacktrace(void)
{
    #ifdef HAVE_EXECINFO_H
    void *buffer[MAX_STACK_FRAMES];
    int frames = backtrace(buffer, MAX_STACK_FRAMES);
    char **symbols = backtrace_symbols(buffer, frames);

    char trace_msg[512];
    snprintf(trace_msg, sizeof(trace_msg), "\nStack trace (%d frames):\n", frames);
    openlog("tf_ar_poe", 0, LOG_USER);
    syslog(LOG_CRIT, "trace_msg 1 %s", trace_msg);

    for (int i = 0; i < frames; i++) {
        snprintf(trace_msg, sizeof(trace_msg), "#%d %s\n", i, symbols[i]);
        syslog(LOG_CRIT, "trace_msg 2 %s", trace_msg);
    }
    closelog();
    free(symbols);
    #else
          syslog(LOG_DEBUG, "Stacktrace not available (execinfo.h missing)");
    #endif
}

// Расшифровка кода сигнала
static const char* get_signal_code(int sig, int code) {
    switch(sig) {
        case SIGSEGV:
            return (code == SEGV_MAPERR) ? "Address not mapped" :
                   (code == SEGV_ACCERR) ? "Invalid permissions" : "Unknown";
        case SIGILL:
            return (code == ILL_ILLOPC) ? "Illegal opcode" :
                   (code == ILL_ILLOPN) ? "Illegal operand" : "Unknown";
        default:
            return "N/A";
    }
}

void critical_signal_handler(int sig, siginfo_t *info, void *ucontext) {
    char error_msg[512];
    ucontext_t *uc = (ucontext_t *)ucontext;
    void *fault_addr = info->si_addr;

    // Базовая информация
    snprintf(error_msg, sizeof(error_msg),
             "\n=== CRITICAL ERROR [%s]===\n"
             "Signal: %d (%s)\n"
             //"Fault address: %p\n"
             "Code: %s (%d)\n"
             "PID: %d, UID: %d\n",
             strsignal(sig),
             sig, strsignal(sig),
             get_signal_code(sig, info->si_code), info->si_code,
             getpid(), getuid());
    syslog(LOG_CRIT,  "error_msg 2: %s ", error_msg);

    // Регистры процессора
    //snprintf(error_msg, sizeof(error_msg),
    //         "Registers:\n"
    //         "RIP: 0x%lx\n"
    //         "RAX: 0x%lx\n"
    //         "RBX: 0x%lx\n"
    //         "RCX: 0x%lx\n",
    //         (long)uc->uc_mcontext.gregs[REG_RIP],
    //         (long)uc->uc_mcontext.gregs[REG_RAX],
    //         (long)uc->uc_mcontext.gregs[REG_RBX],
    //         (long)uc->uc_mcontext.gregs[REG_RCX]);
    //syslog(LOG_CRIT,  "error_msg 2: %s ", error_msg);

    print_stacktrace();
    syslog(LOG_CRIT, "CRASH: Signal %d (%s) at %p. Stacktrace saved to %s",
           sig, strsignal(sig), fault_addr, LOG_FILE);

    signal(sig, SIG_DFL);
    raise(sig);
    keepRunning = 0;
    syslog(LOG_CRIT, "Initiating graceful shutdown...");
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = critical_signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

    const int signals[] = {SIGSEGV, SIGILL, SIGFPE, SIGABRT, SIGBUS};
    for (size_t i = 0; i < sizeof(signals)/sizeof(*signals); i++) {
        if (sigaction(signals[i], &sa, NULL) == -1) {
            syslog(LOG_ERR, "Failed to setup handler for signal %d", signals[i]);
        }
    }
    sa.sa_handler = graceful_shutdown_handler;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

static void graceful_shutdown_handler(int sig)
{
    keepRunning = 0;
    syslog(LOG_INFO, "Received shutdown signal: %d", sig);
}


int main(int argc, char **argv)
{
    openlog("tf_ar_poe", 0, LOG_USER);
    setup_signal_handlers();
    syslog(LOG_INFO, "Autorestart v%s started", VERSION);
    system_init(); // Инициализация системы
    ar_mainApp();
    pthread_mutex_lock(&shutdownMutex);
    while (keepRunning)
    {
        sleep(1);
        // Проверка статуса потоков может быть добавлена здесь
    }
    pthread_mutex_unlock(&shutdownMutex);
    closelog();
    return 0;
}

