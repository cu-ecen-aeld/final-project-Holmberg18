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
#define PROC_STAT "/proc/stat"
#define PROC_MEMINFO "/proc/meminfo"
#define PROC_LOADAVG "/proc/loadavg"
#define TEMP_COOL_MAX 50.0f
#define TEMP_MEDIUM_MAX 70.0f // Fan speeds: 0% (<50°C), 50% (50-70°C), 100% (>70°C)
#define LOG_INTERVAL 2  // in seconds
#define RUNNING 1

volatile sig_atomic_t running = RUNNING;
FILE *log_stream = NULL;
typedef struct {
    float cpu_usage;
    unsigned long mem_total; // Total mem in KB
    unsigned long mem_available; // Available mem in KB
    unsigned long mem_used; // Total used mem in KB
    float mem_usage; // Mem usage as percentage
    float load_1min; // 1-min load average
    float load_5min; // 5-min load average
    float load_15min; //15 min load average
} system_stats_t;

static unsigned long prev_total = 0, prev_idle = 0;

void signal_handler(int sig) {
    running = 0;
}



system_stats_t read_load_average(){
    FILE *load_file = fopen(PROC_LOADAVG, "r");
    system_stats_t load_avg = {0};

    if(load_file == NULL){
        fprintf(log_stream, "ERROR: Failed to open /proc/loadavg - %s\n", strerror(errno));
        load_avg.load_1min = -1.0f; // Error
        return load_avg;
    }

    int result = fscanf(load_file, "%f %f %f", &load_avg.load_1min, &load_avg.load_5min, &load_avg.load_15min);
    fclose(load_file);

    if(result != 3) {
        fprintf(log_stream, "ERROR: Failed to parse /proc/loadavg - expected 3 values, got %d\n", result);
        load_avg.load_1min = -1.0f; // Error
    }

    fprintf(log_stream, "Load average: 1min=%.2f, 5min=%.2f, 15min=%.2f\n", load_avg.load_1min, load_avg.load_5min, load_avg.load_15min);

    return load_avg;
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
        fprintf(log_stream, "ERROR: Unable to read data from temperature file\n");
        return -1.0f;
    }
    
    fclose(file);
    return temp_raw / 1000.0f;
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

float calculate_cpu_usage(){
    FILE *stat_file = fopen(PROC_STAT, "r");
    if(!stat_file) return -1.0f;

    char cpu[10];
    unsigned long user, nice, system, idle, iowait, irq, softirq;

    int result = fscanf(stat_file, "%s %lu %lu %lu %lu %lu %lu %lu",
                        cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(stat_file);

    if(result != 8) return -1.0f;

    unsigned long total = user + nice + system + idle + iowait + irq + softirq;
    unsigned long idle_time = idle + iowait;

    if(prev_total == 0){
        prev_total = total;
        prev_idle = idle_time;
        return 0.0f;
    }

    unsigned long total_diff = total - prev_total;
    unsigned long idle_diff = idle_time - prev_idle;

    prev_total = total; 
    prev_idle = idle_time;

    if(total_diff == 0) return 0.0f;

    return 100.0f * ((float)(total_diff - idle_diff) / (float)total_diff);
}

int read_memory_info(unsigned long *total, unsigned long *available){
    FILE *mem_file = fopen(PROC_MEMINFO, "r");
    if(!mem_file) return -1;

    char line[256];
    *total = *available = 0;

    while (fgets(line, sizeof(line), mem_file)){
        if(strstr(line, "MemTotal:") == line){
            sscanf(line, "MemTotal: %lu kB", total);
        } else if(strstr(line, "MemAvailable:") == line){
            sscanf(line, "MemAvailable: %lu kB", available);
        }
    }

    fclose(mem_file);
    return (*total > 0 && *available > 0) ? 0: -1;
}

system_stats_t read_system_stats(){
    system_stats_t stats = {0};

    // Get the cpu usage
    stats.cpu_usage = calculate_cpu_usage();

    // Generate memory info
    unsigned long mem_total, mem_available;
    if(read_memory_info(&mem_total, &mem_available) == 0){
        stats.mem_total = mem_total;
        stats.mem_available = mem_available;
        stats.mem_used = mem_total - mem_available;
        stats.mem_usage = 100.0f * (mem_total - mem_available) / mem_total;
    } else {
        stats.mem_usage = -1.0f; // ran into an error
    }

    // Fetch load avg
    system_stats_t load = read_load_average();
    stats.load_1min = load.load_1min;
    stats.load_5min = load.load_5min;
    stats.load_15min = load.load_15min;

    return stats;

}

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void write_json_data(float temperature, int fan_speed, system_stats_t stats) {
    char timestamp[20];
    get_timestamp(timestamp, sizeof(timestamp));

    FILE *json_file = fopen(JSON_PATH, "w");
    if(json_file == NULL) {
        fprintf(log_stream, "[%s] Error: Failed to open %s - errno: %d\n", timestamp, JSON_PATH, errno);
        return;
    }

    fprintf(json_file, 
    "{\n"
    "  \"temperature\": %.1f,\n"
    "  \"fan_speed\": %d,\n"
    "  \"cpu_usage\": %.1f,\n"
    "  \"memory_used\": %lu,\n"
    "  \"memory_total\": %lu,\n"
    "  \"memory_usage\": %.1f,\n"
    "  \"load_1min\": %.2f,\n"
    "  \"load_5min\": %.2f,\n"
    "  \"load_15min\": %.2f,\n"
    "  \"timestamp\": \"%s\",\n"
    "  \"unit\": \"celsius\"\n"
    "}\n",
    temperature, fan_speed, stats.cpu_usage,
    stats.mem_used, stats.mem_total, stats.mem_usage,
    stats.load_1min, stats.load_5min, stats.load_15min,
    timestamp);
    
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
        system_stats_t system_stats = read_system_stats();
        int fan_speed = calculate_fan_speed(temp, system_stats.load_1min);

        char timestamp[20];
        get_timestamp(timestamp, sizeof(timestamp));
        
        if (temp >= 0) {
            write_json_data(temp, fan_speed, system_stats);
            // Log temperature
            fprintf(log_stream, "[%s] CPU Temperature: %.1f°C, Fan Speed: %d%%, CPU: %.1f%%, Mem: %.1f%% - JSON updated\n", 
                    timestamp, temp, fan_speed, system_stats.cpu_usage, system_stats.mem_usage);
        } else {
            fprintf(log_stream, "[%s] ERROR: Could not read temperature or system data from %s\n", 
                    timestamp, TEMP_FILE);
        }
        fflush(log_stream);
        sleep(LOG_INTERVAL);
    }
    
    fprintf(log_stream, "System Monitor Daemon shutting down...\n");
    fclose(log_stream);
    return 0;
}