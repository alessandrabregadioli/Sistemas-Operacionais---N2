#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    int pid;
    char user[64];
    char name[256];
    unsigned long uticks_before;
    unsigned long uticks_after;
    double cpu;   // %
    double mem;   // %
    int alive;
} Proc;

static int read_proc_stat(int pid, char *comm_out, size_t comm_sz,
                          unsigned long *utime, unsigned long *stime) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    int _pid; char comm[256], state;
    unsigned long dummy_ul;
    
    if (fscanf(f, "%d (%255[^)]) %c", &_pid, comm, &state) != 3) {
        fclose(f);
        return -1;
    }
    
    for (int i=0; i<11; i++) if (fscanf(f, " %lu", &dummy_ul) != 1) { fclose(f); return -1; }
    // Campos 14 (utime) e 15 (stime)
    if (fscanf(f, " %lu %lu", utime, stime) != 2) { fclose(f); return -1; }
    fclose(f);

    if (comm_out && comm_sz > 0) {
        strncpy(comm_out, comm, comm_sz);
        comm_out[comm_sz-1] = '\0';
    }
    return 0;
}

static int read_status_mem_uid(int pid, long *rss_kb, int *uid_out) {
    char path[64], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    long rss = 0; int uid = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            // "VmRSS:     12345 kB"
            long x=0; if (sscanf(line, "VmRSS:%ld kB", &x) == 1) rss = x;
        } else if (strncmp(line, "Uid:", 4) == 0) {
            // "Uid:    real  eff   saved  fs"
            int r=0,e=0,s=0,fs=0;
            if (sscanf(line, "Uid:\t%d\t%d\t%d\t%d", &r, &e, &s, &fs) >= 1) uid = r;
        }
    }
    fclose(f);
    if (rss_kb) *rss_kb = rss;
    if (uid_out) *uid_out = uid;
    return 0;
}

static double read_total_jiffies(void) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0.0;
    char cpu[8];
    unsigned long long a,b,c,d,e,fv,g,h,i,j;
    double tot = 0.0;
    if (fscanf(f, "%7s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
               cpu,&a,&b,&c,&d,&e,&fv,&g,&h,&i,&j) == 11) {
        tot = (double)(a+b+c+d+e+fv+g+h+i+j);
    }
    fclose(f);
    return tot;
}


static int cmp_cpu_desc(const void *pa, const void *pb) {
    const Proc *a = (const Proc*)pa, *b = (const Proc*)pb;
    if (b->cpu > a->cpu) return 1;
    if (b->cpu < a->cpu) return -1;
    return 0;
}
static int cmp_mem_desc(const void *pa, const void *pb) {
    const Proc *a = (const Proc*)pa, *b = (const Proc*)pb;
    if (b->mem > a->mem) return 1;
    if (b->mem < a->mem) return -1;
    return 0;
}

int main(void) {
    DIR *dir = opendir("/proc");
    if (!dir) { perror("opendir /proc"); return 1; }

    /* 1) Snapshot BEFORE: pegar PIDs e uticks */
    Proc *procs = NULL; int cap = 1024, n = 0;
    procs = calloc(cap, sizeof(Proc));
    if (!procs) { perror("calloc"); closedir(dir); return 1; }

    struct dirent *de;
    while ((de = readdir(dir))) {
        char *end; long pid_l = strtol(de->d_name, &end, 10);
        if (*end != '\0') continue; // não é numérico
        int pid = (int)pid_l;

        if (n == cap) {
            cap *= 2;
            Proc *tmp = realloc(procs, cap * sizeof(Proc));
            if (!tmp) { perror("realloc"); free(procs); closedir(dir); return 1; }
            procs = tmp;
        }

        unsigned long u=0, s=0;
        char name[256] = {0};
        if (read_proc_stat(pid, name, sizeof(name), &u, &s) == 0) {
            procs[n].pid = pid;
            procs[n].uticks_before = u + s;
            strncpy(procs[n].name, name, sizeof(procs[n].name));
            procs[n].alive = 1;
            n++;
        }
    }
    closedir(dir);

    double total_before = read_total_jiffies();
    usleep(200000); // 200 ms de amostragem
    double total_after  = read_total_jiffies();
    double total_delta  = total_after - total_before;
    if (total_delta <= 0) total_delta = 1.0;

    /* 2) Snapshot AFTER + memória/usuário */
    for (int i = 0; i < n; i++) {
        if (!procs[i].alive) continue;

        unsigned long u=0, s=0;
        if (read_proc_stat(procs[i].pid, NULL, 0, &u, &s) == 0) {
            procs[i].uticks_after = u + s;
        } else {
            procs[i].alive = 0; // morreu no meio
            continue;
        }

        long rss_kb = 0; int uid = -1;
        if (read_status_mem_uid(procs[i].pid, &rss_kb, &uid) == 0) {
            struct passwd *pw = (uid >= 0) ? getpwuid((uid_t)uid) : NULL;
            snprintf(procs[i].user, sizeof(procs[i].user), "%s", pw ? pw->pw_name : "unknown");

            struct sysinfo si; sysinfo(&si);
            double total_ram_bytes = (double)si.totalram * (double)si.mem_unit;
            procs[i].mem = (total_ram_bytes > 0.0) ? ((rss_kb * 1024.0) / total_ram_bytes * 100.0) : 0.0;
        } else {
            snprintf(procs[i].user, sizeof(procs[i].user), "%s", "unknown");
            procs[i].mem = 0.0;
        }

        double delta_ticks = (double)((long long)procs[i].uticks_after - (long long)procs[i].uticks_before);
        if (delta_ticks < 0) delta_ticks = 0;
        procs[i].cpu = (delta_ticks / total_delta) * 100.0;
    }

  
    int m = 0;
    for (int i=0;i<n;i++) if (procs[i].alive) procs[m++] = procs[i];


    qsort(procs, m, sizeof(Proc), cmp_cpu_desc);
    printf("===== TOP 5 CPU =====\n");
    for (int i=0; i<5 && i<m; i++) {
        printf("PID=%-6d USER=%-12s CMD=%-20s CPU=%6.2f%% MEM=%6.2f%%\n",
               procs[i].pid, procs[i].user, procs[i].name, procs[i].cpu, procs[i].mem);
    }


    qsort(procs, m, sizeof(Proc), cmp_mem_desc);
    printf("\n===== TOP 5 MEM =====\n");
    for (int i=0; i<5 && i<m; i++) {
        printf("PID=%-6d USER=%-12s CMD=%-20s MEM=%6.2f%% CPU=%6.2f%%\n",
               procs[i].pid, procs[i].user, procs[i].name, procs[i].mem, procs[i].cpu);
    }

 
    const double CPU_LIMIT = 50.0, MEM_LIMIT = 20.0;
    printf("\n===== ALERTAS (CPU>%.1f%% ou MEM>%.1f%%) =====\n", CPU_LIMIT, MEM_LIMIT);
    for (int i=0; i<m; i++) {
        if (procs[i].cpu > CPU_LIMIT || procs[i].mem > MEM_LIMIT) {
            printf("[ALERTA] PID %d (%s): CPU=%.2f%% MEM=%.2f%% USER=%s\n",
                   procs[i].pid, procs[i].name, procs[i].cpu, procs[i].mem, procs[i].user);
        }
    }

    free(procs);
    return 0;
}


