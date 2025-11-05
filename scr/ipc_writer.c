#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

int main() {
    const char *fifo_in = "fifo_in";
    const char *fifo_out = "fifo_out";

    mkfifo(fifo_in, 0666);
    mkfifo(fifo_out, 0666);

    int fd_in = open(fifo_in, O_WRONLY);
    int fd_out = open(fifo_out, O_RDONLY);

    if (fd_in < 0 || fd_out < 0) {
        perror("Erro abrindo FIFO");
        exit(1);
    }

    char msg[100];
    char buffer[100];
    int count = 1;

    while (1) {
        sprintf(msg, "PING %d", count++);
        write(fd_in, msg, strlen(msg));
        printf("[Writer] Enviado: %s\n", msg);

        int n = read(fd_out, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("[Writer] Recebido: %s\n", buffer);
        }

        sleep(2);
    }

    close(fd_in);
    close(fd_out);

    return 0;
}

