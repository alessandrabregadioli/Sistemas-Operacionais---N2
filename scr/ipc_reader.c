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

    int fd_in = open(fifo_in, O_RDONLY);
    int fd_out = open(fifo_out, O_WRONLY);

    if (fd_in < 0 || fd_out < 0) {
        perror("Erro abrindo FIFO");
        exit(1);
    }

    char buffer[100];
    int count = 1;

    while (1) {
        int n = read(fd_in, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("[Reader] Recebido: %s\n", buffer);

            char reply[100];
            sprintf(reply, "PONG %d <- %s", count++, buffer);
            write(fd_out, reply, strlen(reply));
            printf("[Reader] Respondido: %s\n", reply);
        }
    }

    close(fd_in);
    close(fd_out);

    return 0;
}

