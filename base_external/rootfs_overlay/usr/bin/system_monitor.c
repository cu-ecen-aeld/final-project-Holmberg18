#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TEMP_FILE "/sys/class/thermal/thermal_zone0/temp"
#define LOG_INTERVAL 2  // in seconds
#define RUNNING 1

volatile sig_atomic_t running = RUNNING;

void signal_handler(int sig) {
    running = 0;
}

float read_temperature() {
    FILE *file = fopen(TEMP_FILE, "r");
    if (file == NULL) {
        return -1.0f;
    }
    
    int temp_raw;
    if (fscanf(file, "%d", &temp_raw) != 1) {
        fclose(file);
        return -1.0f;
    }
    
    fclose(file);
    return temp_raw / 1000.0f;
}

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

int main() {
    printf("System Monitor Daemon starting...\n");
    
    // Set up signal handling in case of shutdown
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    // Daemonize
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    if (pid > 0) {
        // Exit parent process
        exit(0);
    }
    
    // Continue daemon child
    umask(0);
    setsid();
    chdir("/");
    
    // Close file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Reopen stdout to /dev/tty1 for logging
    FILE *log_stream = fopen("/dev/tty1", "w");
    if (log_stream == NULL) {
        log_stream = fopen("/dev/null", "w");
    }
    
    fprintf(log_stream, "System Monitor Daemon started (PID: %d)\n", getpid());
    fflush(log_stream);
    
    while (running) {
        float temp = read_temperature();
        char timestamp[20];
        get_timestamp(timestamp, sizeof(timestamp));
        
        if (temp >= 0) {
            fprintf(log_stream, "[%s] CPU Temperature: %.1f°C\n", timestamp, temp);
            fflush(log_stream);
        } else {
            fprintf(log_stream, "[%s] ERROR: Could not read temperature from %s\n", 
                    timestamp, TEMP_FILE);
            fflush(log_stream);
        }
        
        sleep(LOG_INTERVAL);
    }
    
    fprintf(log_stream, "System Monitor Daemon shutting down...\n");
    fclose(log_stream);
    return 0;
}