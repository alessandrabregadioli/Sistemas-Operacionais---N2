#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    size_t step_mb = (argc > 1) ? strtoul(argv[1], NULL, 10) : 256;
    size_t target_mb = (argc > 2) ? strtoul(argv[2], NULL, 10) : 4096;

    if (step_mb == 0 || target_mb == 0) {
        fprintf(stderr, "Uso: %s <MB_por_passo> <alvo_total_MB>\n", argv[0]);
        return 1;
    }

    size_t step_bytes = step_mb * 1024UL * 1024UL;
    size_t target_bytes = target_mb * 1024UL * 1024UL;

    size_t allocated = 0;
    void **blocks = NULL;
    size_t n = 0, cap = 0;

    printf("Iniciando: passo=%zu MB, alvo=%zu MB\n", step_mb, target_mb);
    while (allocated < target_bytes) {
        void *p = malloc(step_bytes);
        if (!p) {
            perror("malloc falhou");
            break;
        }

        memset(p, 0xA5, step_bytes);

        if (n == cap) {
            cap = cap ? cap * 2 : 32;
            void **tmp = realloc(blocks, cap * sizeof(void*));
            if (!tmp) {
                perror("realloc falhou");
                free(p);
                break;
            }
            blocks = tmp;
        }
        blocks[n++] = p;
        allocated += step_bytes;

        printf("[OK] alocado: %6zu MB / %6zu MB\n", allocated / (1024*1024), target_bytes / (1024*1024));
        fflush(stdout);
        usleep(200000); /* 0.2s só pra ir com calma e facilitar prints */
    }

    printf("Pausa: pressione ENTER para liberar memória (ou Ctrl+C pra manter).\n");
    getchar();

    for (size_t i = 0; i < n; ++i) free(blocks[i]);
    free(blocks);

    printf("Memória liberada. Fim.\n");
    return 0;
}

