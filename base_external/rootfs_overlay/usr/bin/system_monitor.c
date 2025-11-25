#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>

#define TEMP_FILE "/sys/class/thermal/thermal_zone0/temp"
#define JSON_PATH "/var/www/system_stats.json"
#define LOG_PATH "/var/log/system_monitor.log"
#define TEMP_COOL_MAX 50.0f
#define TEMP_MEDIUM_MAX 70.0f // Fan speeds: 0% (<50°C), 50% (50-70°C), 100% (>70°C)
#define LOG_INTERVAL 2  // in seconds
#define RUNNING 1

volatile sig_atomic_t running = RUNNING;
FILE *log_stream = NULL;

void signal_handler(int sig) {
    running = 0;
}

float read_temperature() {
    FILE *file = fopen(TEMP_FILE, "r");
    if (file == NULL) {
        fprintf(log_stream, "ERROR: Failed to open temperature file\n");
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

float read_load_average(){
    FILE *load_file = fopen("/proc/loadavg", "r");

    if(load_file == NULL){
        fprintf(log_stream, "ERROR: Failed to open /proc/loadavg - %s\n", strerror(errno));
        return 0.0f;
    }

    float one_min, five_min, fifteen_min;
    int result = fscanf(load_file, "%f %f %f", &one_min, &five_min, &fifteen_min);
    fclose(load_file);

    if(result != 3) {
        fprintf(log_stream, "ERROR: Failed to parse /proc/loadavg - expected 3 values, got %d\n", result);
        return 0.0f;
    }

    fprintf(log_stream, "Load average: 1min=%.2f, 5min=%.2f, 15min=%.2f\n", one_min, five_min, fifteen_min);

    return one_min;
}

int calculate_fan_speed(float temperature, float load_avg){

    int base_speed;
    if(temperature < TEMP_COOL_MAX){
        base_speed =  0; // fan at 0% speed
    } else if (temperature <= TEMP_MEDIUM_MAX){
        base_speed = 50; // fan at 50% speed
    } else {
        base_speed = 100; // fan at 100% speed
    }

    // Calculate fan boost based on load_avg
    int load_boost = 0;
    if(load_avg > 3.0f){
        load_boost = 30; // Heavy load
    } else if (load_avg > 1.5f){
        load_boost = 15;
    }

    // Apply the boost, no more than 100% though
    int final_speed = base_speed + load_boost;
    return (final_speed > 100) ? 100 : final_speed;
}

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void write_json_data(float temperature, int fan_speed) {
    char timestamp[20];
    get_timestamp(timestamp, sizeof(timestamp));

    FILE *json_file = fopen(JSON_PATH, "w");
    if(json_file == NULL) {
        fprintf(log_stream, "[%s] Error: Failed to open %s - errno: %d\n", timestamp, JSON_PATH, errno);
        return;
    }

    fprintf(json_file, "{\"temperature\": %.1f, \"fan_speed\": %d, \"timestamp\": \"%s\", \"unit\": \"celsius\"}\n", temperature, fan_speed, timestamp);
    fclose(json_file);

    chmod(JSON_PATH, 0644);
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

    log_stream = fopen(LOG_PATH, "a");
    if(log_stream == NULL){
        exit(1);
    }

    // Ensure /var/www is writable by daemon
    int ret = system("chown -R root:root /var/www/ 2>/dev/null");
    if(ret != 0){
        fprintf(log_stream, "Warning: chown returned %d\n", ret);
    }

    // Close file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    fprintf(log_stream, "System Monitor Daemon started (PID: %d)\n", getpid());
    fflush(log_stream);
    
    while (running) {
        float temp = read_temperature();
        float load_avg = read_load_average();
        int fan_speed = calculate_fan_speed(temp, load_avg);
        char timestamp[20];
        get_timestamp(timestamp, sizeof(timestamp));
        
        if (temp >= 0) {
            write_json_data(temp, fan_speed);
            // Log temperature
            fprintf(log_stream, "[%s] CPU Temperature: %.1f°C, Fan Speed: %d%% - JSON updated\n", timestamp, temp, fan_speed);
        } else {
            fprintf(log_stream, "[%s] ERROR: Could not read temperature or fan speed from %s\n", 
                    timestamp, TEMP_FILE);
        }
        fflush(log_stream);
        sleep(LOG_INTERVAL);
    }
    
    fprintf(log_stream, "System Monitor Daemon shutting down...\n");
    fclose(log_stream);
    return 0;
}