#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define RED     "\033[1;31m"
#define YELLOW  "\033[1;33m"
#define GREEN   "\033[1;32m"
#define RESET   "\033[0m"
#define WHITE   "\033[1m"
#define CYAN    "\033[1;36m"

//longest aka arrow


void ukaz(char *label, char *text, int longest){
    char log[8];
    for (size_t i = 0; i < 7; i++)
    {
        log[i] = '=';

    }
    log[longest] = '\0';
    if (label == NULL) label = "";
    
    printf("\033[1;32m%s>\033[0m \033[1m %s%s\033[0m\n", log, label, text);
}

void print_info(char *label, char *path, char *search_word, int row){
    FILE *file = fopen(path, "r");
    if (file == NULL){
        printf("error");
        return;
    }

    char line[256];
    while(fgets(line, sizeof(line), file)){
        if(strstr(line, search_word)){
            char *ptr = strchr(line, ':');
            if(ptr != NULL){

                char *value = ptr + 2;

                strtok(value, "\n");
                
                ukaz(label,value, row);
            }
            break;
        }
    }
    fclose(file);
}

void print_gpu(int arrow){
    FILE *fp = popen("lspci | grep -E 'VGA|3D' | cut -d '[' -f2 | cut -d ']' -f1", "r");
    
    if(fp == NULL)return;
    char buffer[128];

    if(fgets(buffer, sizeof(buffer), fp) != NULL){
        buffer[strcspn(buffer, "\r\n")] = '\0';

        ukaz("GPU: ", buffer, arrow);
    }

    pclose(fp);
}


void print_temp(int arrow){
    const char *targets[] = {"k10temp", "coretemp", "zenpower", "cpu_thermal"};
    int found = 0;

    for (int i = 0; i < 6; i++) {
        char path[100], name[32];
        sprintf(path, "/sys/class/hwmon/hwmon%d/name", i);
        FILE *f = fopen(path, "r");
        if (!f) continue;

        fgets(name, sizeof(name), f);
        fclose(f);

        for (int t = 0; t < 4; t++) {
            if (strstr(name, targets[t])) {
                sprintf(path, "/sys/class/hwmon/hwmon%d/temp1_input", i);
                FILE *f_temp = fopen(path, "r");
                if (f_temp) {
                    int raw;
                    fscanf(f_temp, "%d", &raw);
                    fclose(f_temp);
                    double celsius = raw / 1000.0;
                    char buffer[64];
                    
                    
                    char *color;
                    if (celsius <= 50.0) color = GREEN;
                    else if (celsius <= 75.0) color = YELLOW;
                    else color = RED;

                    sprintf(buffer, "%s%.1f°C" RESET, color, raw / 1000.0);
                    ukaz("Temp: ", buffer, arrow);
                    found = 1; break;
                }
            }
        }
        if (found) break;
    }
}

void print_ram(int arrow){
    FILE *file = fopen("/proc/meminfo", "r");

    if(file == NULL)return;

    long total = 0, available = 0;
    char line[256];

    while (fgets(line, sizeof(line), file)){
        if(sscanf(line, "MemTotal: %ld", &total) == 1) continue;
        if(sscanf(line, "MemAvailable: %ld", &available) == 1) break;
    }
    fclose(file);

    if(total > 0){
        long used = total - available;
        char buffer[128];
       
        double used_gb = used / 1048576.0;
        double total_gb = total / 1048576.0;
        double avail_gb = available / 1048576;

        sprintf(buffer, RED "used %.1f GiB" RESET "|" GREEN "available %.1f GiB" RESET "|" YELLOW "total %.1f GiB" RESET, used_gb, avail_gb, total_gb);

        ukaz("RAM: ", buffer, arrow);
        
    }
}

void print_disk(char *label, char *mount_point, int arrow){
    struct statvfs ds;

    if(statvfs(mount_point, &ds) != 0){
        ukaz("Disk", "denied access", arrow);
        return;
    }

    double total = (double)ds.f_blocks * ds.f_frsize;
    double avail = (double)ds.f_bavail * ds.f_frsize;
    double used = total - avail;

    double used_gb = used / 1073741824.0;
    double total_gb = total / 1073741824.0;
    double avail_gb = total_gb - used_gb;

    char buffer[128];

    sprintf(buffer, RED "used %.1f GiB" RESET "|" GREEN "available %.1f GiB" RESET"|"YELLOW "total %.1f GiB" RESET, used_gb, avail_gb, total_gb);

    ukaz(label, buffer, arrow);
   
}

void print_uptime(int arrow){
    FILE *file = fopen("/proc/uptime", "r");
    double seconds;

    if(file == NULL)return;

    if(fscanf(file, "%lf", &seconds) != 1){
        fclose(file);
        return;
    }
    fclose(file);

    int hours = (int)seconds / 3600;
    int minutes = ((int)seconds % 3600) / 60;
    int sec = (int)seconds % 60;

    char buffer[64];

     if (hours > 0) {
        sprintf(buffer, "%dh %dm", hours, minutes);
    } else {
        sprintf(buffer, "%dm %ds", minutes, sec); 
    }

    ukaz("Uptime: ", buffer, arrow);
}

void print_os(int arrow){
    FILE *file = fopen("/etc/os-release", "r");

    if (file == NULL){
        printf("error\n");
        return;
    }

    char line[128]; 
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "PRETTY_NAME")) {
            char *ptr = strchr(line, '=');
            if (ptr != NULL) {
                char *value = ptr + 2; 

                char *end_quote = strchr(value, '"');
                if (end_quote) {
                    *end_quote = '\0';
                }

                value[strcspn(value, "\r\n")] = '\0';
                
                ukaz("OS: ", value, arrow);
            }
            break;
        }
    }
    fclose(file);
}

void print_logo(int row) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    if (w.ws_col < 85) {
        return; 
    }

    static  const char *logo[] = {
    CYAN "       ________________      " RESET,
    CYAN "      /                \\     " RESET,
    CYAN "     /      ________    \\    " RESET, 
    CYAN "    /      /\\       \\    \\   " RESET,
    CYAN "   /      /  \\       \\    \\  " RESET,
    CYAN "  /      /    \\       \\    \\ " RESET,
    CYAN " /      /  __  \\       \\    \\" RESET,
    CYAN "/      /  |  |  \\       \\    \\" RESET,
    CYAN "|     |   |__|   |       |    |" RESET, 
    "\033[1;34m\\      \\  |__|  /       /    /" RESET,
    "\033[1;34m \\      \\      /       /    / " RESET,
    "\033[1;34m  \\      \\____/       /    /  " RESET,
    "\033[1;34m   \\                 /    /   " RESET, 
    "\033[1;34m    \\               /    /    " RESET, 
    "\033[1;34m     \\_____________/    /     " RESET, 
    "\033[1;34m      \\________________/      " RESET, 
    "\033[1;34m       \\______________/       " RESET, 
    "\033[1;34m        \\____________/        " RESET  

    };  
    

    

    printf("%s ", logo[row]);
}

void print_cmd(char *label, char *command, int arrow) {
    
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    FILE *fp = popen(command, "r");
    char buffer[128] = {0};

    
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\r\n")] = '\0';
        
        if (strlen(buffer) > 0) {
            ukaz(label, buffer, arrow);
        } else {
            if (w.ws_col >= 85) printf("\n"); 
        }
    } else {
        if (w.ws_col >= 85) printf("\n");
    }

    if (fp) pclose(fp);
}


void print_color() {
    for (int i = 0; i < 8; i++) {
        printf("\033[4%dm   ", i);
    }
    printf("\033[0m\n");

}

void print_battery(int arrow) {
    FILE *f_cap = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    FILE *f_stat = fopen("/sys/class/power_supply/BAT0/status", "r");
    
    if (!f_cap) { 
        f_cap = fopen("/sys/class/power_supply/BAT1/capacity", "r");
        f_stat = fopen("/sys/class/power_supply/BAT1/status", "r");
    }

    if (f_cap) {
        int cap;
        char stat[20];
        fscanf(f_cap, "%d", &cap);
        if (f_stat) {
            fscanf(f_stat, "%s", stat);
            fclose(f_stat);
        }
        fclose(f_cap);

        char *color = (cap > 60) ? GREEN : (cap > 20) ? YELLOW : RED;
        char buffer[64];
        sprintf(buffer, "%s%d%%" RESET " [%s]", color, cap, stat);
        ukaz("Battery:  ", buffer, arrow);
    } else {
        printf("\n"); 
    }
}


int main(int argc, char *argv[]){
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf(YELLOW "infofetch " RESET "is a simple neofetch-like system information tool written in C.\n");
        printf(WHITE "infofetch v1.0" RESET " by " CYAN "NTMPO" RESET "\n");
        return 0;
    }

    printf("\t\t---SYSTEM INFO---\n");
    print_logo(0);   print_info("CPU: ", "/proc/cpuinfo", "model name", 4);
    print_logo(1);   print_info("Core: ", "/proc/cpuinfo", "cpu cores", 2);
    print_logo(2);   print_temp(2);
    print_logo(3);   print_gpu(4);
    print_logo(4);   print_ram(4);
    print_logo(5);   print_disk("Disk /: ", "/", 1);
    print_logo(6);   print_disk("Disk /home: ", "/home", 0);
    print_logo(7);   print_uptime(4);
    print_logo(8);   print_battery(3);
    print_logo(9);   print_color();
    print_logo(10);  print_os(4);
    print_logo(11);  print_cmd("Host: ", "uname -n",4);
    print_logo(12);  print_cmd("User: ", "echo $USER", 4);
    print_logo(13);  print_cmd("Shell: ", "basename $SHELL", 4);
    print_logo(14);  print_cmd("IP:", "ip a 2>/dev/null | grep 'global' | awk '{print $2}' | cut -d/ -f1", 4);
    print_logo(15);  print_cmd("DE: ","echo $XDG_CURRENT_DESKTOP", 4);
    print_logo(16);  print_cmd("Pkgs: ", "pacman -Q | wc -l | tr -d '\n' && echo ' (pacman)'", 5);
    print_logo(17);  print_cmd("Res:", "xrandr 2>/dev/null | grep '*' | awk '{print $1}'", 5);
    
   

    return 0;
}