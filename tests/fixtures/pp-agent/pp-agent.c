/*
 * Minimal pp-agent reference (iOS arm64 ramdisk TCP server).
 * Build with iphoneos SDK; not compiled by purplepois0n by default.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

static ssize_t write_all(int fd, const void* buf, size_t len) {
    const char* p = (const char*)buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, p + sent, len - sent);
        if (n <= 0) {
            return -1;
        }
        sent += (size_t)n;
    }
    return (ssize_t)sent;
}

static int read_line(int fd, char* out, size_t cap) {
    size_t n = 0;
    while (n + 1 < cap) {
        char c;
        ssize_t r = read(fd, &c, 1);
        if (r <= 0) {
            return -1;
        }
        if (c == '\n') {
            out[n] = '\0';
            return 0;
        }
        if (c != '\r') {
            out[n++] = c;
        }
    }
    return -1;
}

static void handle_client(int fd) {
    char line[4096];
    if (read_line(fd, line, sizeof(line)) != 0) {
        return;
    }
    if (strcmp(line, "PING") == 0) {
        write_all(fd, "PONG\n", 5);
        return;
    }
    if (strncmp(line, "EXEC ", 5) == 0) {
        FILE* fp = popen(line + 5, "r");
        char buf[512];
        int code = 127;
        if (fp != NULL) {
            while (fgets(buf, sizeof(buf), fp) != NULL) {
                write_all(fd, buf, strlen(buf));
            }
            code = pclose(fp);
            if (code < 0) {
                code = 127;
            }
        }
        char status[32];
        snprintf(status, sizeof(status), "OK %d\n", code);
        write_all(fd, status, strlen(status));
        write_all(fd, ".\n", 2);
        return;
    }
    if (strncmp(line, "PUT ", 4) == 0) {
        char path[1024];
        unsigned long size = 0;
        if (sscanf(line + 4, "%1023s %lu", path, &size) != 2) {
            write_all(fd, "ERR bad put\n", 12);
            return;
        }
        FILE* out = fopen(path, "wb");
        if (out == NULL) {
            write_all(fd, "ERR open\n", 9);
            return;
        }
        unsigned char* data = (unsigned char*)malloc(size ? size : 1);
        if (data == NULL) {
            fclose(out);
            write_all(fd, "ERR nomem\n", 10);
            return;
        }
        size_t got = 0;
        while (got < size) {
            ssize_t chunk = read(fd, data + got, size - got);
            if (chunk <= 0) {
                break;
            }
            got += (size_t)chunk;
        }
        if (got == size) {
            fwrite(data, 1, size, out);
            write_all(fd, "OK\n", 3);
        } else {
            write_all(fd, "ERR read\n", 9);
        }
        free(data);
        fclose(out);
        return;
    }
    if (strncmp(line, "GET ", 4) == 0) {
        const char* path = line + 4;
        FILE* in = fopen(path, "rb");
        if (in == NULL) {
            write_all(fd, "ERR missing\n", 12);
            return;
        }
        fseek(in, 0, SEEK_END);
        long sz = ftell(in);
        fseek(in, 0, SEEK_SET);
        if (sz < 0) {
            fclose(in);
            write_all(fd, "ERR size\n", 9);
            return;
        }
        char header[64];
        snprintf(header, sizeof(header), "DATA %ld\n", sz);
        write_all(fd, header, strlen(header));
        char buf[512];
        long left = sz;
        while (left > 0) {
            size_t chunk = (size_t)(left > (long)sizeof(buf) ? sizeof(buf) : left);
            size_t n = fread(buf, 1, chunk, in);
            if (n == 0) {
                break;
            }
            write_all(fd, buf, n);
            left -= (long)n;
        }
        fclose(in);
        return;
    }
    write_all(fd, "ERR unknown\n", 12);
}

int main(void) {
    const int port = 4444;
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        return 1;
    }
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        return 1;
    }
    listen(srv, 4);
    for (;;) {
        int client = accept(srv, NULL, NULL);
        if (client < 0) {
            continue;
        }
        handle_client(client);
        close(client);
    }
    return 0;
}
