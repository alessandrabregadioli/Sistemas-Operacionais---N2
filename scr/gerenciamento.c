#define _XOPEN_SOURCE 700
#include <errno.h>
#include <ftw.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static void warn_errno(const char *msg, const char *path) {
    fprintf(stderr, "[warn] %s: %s (%s)\n", msg, path, strerror(errno));
}

static const char* get_home_dir(void) {
    const char *home = getenv("HOME");
    if (home && *home) return home;
    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir && *pw->pw_dir) return pw->pw_dir;
    return ".";
}

static int mkdir_if_needed(const char *path, mode_t mode) {
    if (mkdir(path, mode) == 0) return 0;
    if (errno == EEXIST) return 0;
    warn_errno("mkdir falhou", path);
    return -1;
}

static int write_text_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) { warn_errno("fopen falhou", path); return -1; }
    if (fputs(content, f) == EOF) { warn_errno("fputs falhou", path); fclose(f); return -1; }
    if (fputc('\n', f) == EOF) { warn_errno("fputc falhou", path); fclose(f); return -1; }
    if (fclose(f) != 0) { warn_errno("fclose falhou", path); return -1; }
    return 0;
}

static void print_ls_long(const char *path) {
    char cmd[1024];
    int n = snprintf(cmd, sizeof(cmd), "ls -l -- '%s'", path);
    if (n < 0 || n >= (int)sizeof(cmd)) {
        fprintf(stderr, "[warn] caminho muito longo p/ ls: %s\n", path);
        return;
    }
    int rc = system(cmd);
    if (rc == -1) warn_errno("system(ls) falhou", path);
}

static int recent_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)ftwbuf; (void)typeflag;
    if (S_ISREG(sb->st_mode)) {
        time_t now = time(NULL);
        double hours = difftime(now, sb->st_mtime) / 3600.0;
        if (hours <= 24.0) {
            char when[64];
            struct tm lt;
            localtime_r(&sb->st_mtime, &lt);
            strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", &lt);
            printf("[<24h] %s  (%s)  size=%ld bytes\n", fpath, when, (long)sb->st_size);
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *home = get_home_dir();
    char base[1024];

    if (argc > 1 && argv[1] && argv[1][0] != '\0') {
        snprintf(base, sizeof(base), "%s", argv[1]);
    } else {
        snprintf(base, sizeof(base), "%s/gerenciamento_sistema", home);
    }

    char docs[1024], src[1024], bin[1024], logs[1024];
    snprintf(docs, sizeof(docs), "%s/docs", base);
    snprintf(src,  sizeof(src),  "%s/src",  base);
    snprintf(bin,  sizeof(bin),  "%s/bin",  base);
    snprintf(logs, sizeof(logs), "%s/logs", base);

    /* cria diretórios (dirs precisam do 'x' para entrar/listar) */
    if (mkdir_if_needed(base, 0755) < 0) return 1;
    if (mkdir_if_needed(docs, 0750) < 0) return 1;
    if (mkdir_if_needed(src,  0750) < 0) return 1;
    if (mkdir_if_needed(bin,  0755) < 0) return 1;
    if (mkdir_if_needed(logs, 0755) < 0) return 1;

    char f_in[1200], f_out[1200], readme[1200], logf[1200];
    snprintf(f_in,   sizeof(f_in),   "%s/mensagem_in.txt",  src);
    snprintf(f_out,  sizeof(f_out),  "%s/mensagem_out.txt", src);
    snprintf(readme, sizeof(readme), "%s/README.txt",       docs);
    snprintf(logf,   sizeof(logf),   "%s/run.log",          logs);

    if (write_text_file(f_in,  "PING 1\nPING 2\nPING 3") < 0) return 1;
    if (write_text_file(f_out, "PONG 1\nPONG 2\nPONG 3") < 0) return 1;
    if (write_text_file(readme,
        "Estrutura criada por gerenciamento.c para o Trabalho N2.\n"
        "Verifique permissões e arquivos modificados nas últimas 24h.") < 0) return 1;
    if (write_text_file(logf, "Log de execução criado por gerenciamento.c") < 0) return 1;

  
    if (chmod(bin,  0755) != 0) warn_errno("chmod falhou", bin);
    if (chmod(src,  0750) != 0) warn_errno("chmod falhou", src);
    if (chmod(docs, 0750) != 0) warn_errno("chmod falhou", docs);
    if (chmod(logs, 0755) != 0) warn_errno("chmod falhou", logs);

    if (chmod(f_in,   0640) != 0) warn_errno("chmod falhou", f_in);
    if (chmod(f_out,  0640) != 0) warn_errno("chmod falhou", f_out);
    if (chmod(readme, 0640) != 0) warn_errno("chmod falhou", readme);
    if (chmod(logf,   0640) != 0) warn_errno("chmod falhou", logf);

    printf("Diretórios criados em: %s\n\n", base);

    printf("== ls -l (base) ==\n");
    print_ls_long(base);  printf("\n");

    printf("== ls -l (docs) ==\n");
    print_ls_long(docs);  printf("\n");

    printf("== ls -l (src) ==\n");
    print_ls_long(src);   printf("\n");

    printf("== ls -l (bin) ==\n");
    print_ls_long(bin);   printf("\n");

    printf("== ls -l (logs) ==\n");
    print_ls_long(logs);  printf("\n");

    printf("Arquivos modificados nas últimas 24h em %s:\n", base);
    if (nftw(base, recent_cb, 16, FTW_PHYS) != 0) {
        warn_errno("nftw falhou", base);
        return 1;
    }

    printf("\nConcluído.\n");
    return 0;
}

