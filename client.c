#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "c_utils.h"
#include "constructor.h"
#include "parser.h"
#include "dns_c.h"
#include <assert.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <getopt.h>

#define DEFAULT_NAME_SERVER "8.8.8.8"
#define DEFAULT_NUM_THREADS 2
#define DEFAULT_DOMAIN_NAME "www.google.com"

int _hex_print(unsigned char *array, int size);

int remove_www(char *url);

long long current_timestamp() {
    // https://www.codegrepper.com/code-examples/c/c+get+time+in+milliseconds
    struct timeval te;
    gettimeofday(&te, NULL);                                         // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

int sockfd = 0;

/** Returns true on success, or false if there was an error */
bool SetSocketBlockingEnabled(int fd, bool blocking) {
    // https://stackoverflow.com/a/1549344
    if (fd < 0)
        return false;

#ifdef _WIN32
    unsigned long mode = blocking ? 0 : 1;
    return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif
}

void PrepareDNS(unsigned char *buf, int *out_size, char *domain_name) {
    // Note: remember to call srand before calling PrepareDNS();

    struct DNS_REQUEST question;
    /*
     *  Ahead of all else, make the head.
     */
    // TODO write some freakin macro's

    question.id[0] = 'A' + rand() % 26;
    question.id[1] = 'A' + rand() % 26;

    question.flags1 = '\1';     // 0001
    question.flags2 = '\0';     // 0000
    question.qdcount[0] = '\0'; // 0000
    question.qdcount[1] = '\1'; // 0001
    // query get's filled in at run time
    question.qtype[0] = '\0';  // 0000
    question.qtype[1] = '\1';  // 0001
    question.qclass[0] = '\0'; // 0000
    question.qclass[1] = '\1'; // 0001
    // unsigned char query[1 + DNS_HEADER_SIZE + sizeof(DOMAIN_NAME) + 1 + 6];
    question.query = buf;
    // Fill in that cruft
    for (int i = 0; i < 6; i++) {
        question.cruft[i] = '\0'; // 0000
    }

    build_header(&question);
    // printf("HEADER:\n");
    // _hex_print(question.query, 12);

    /*
     * Get the query in the query.
     */

    // bytes for qtype and class
    build_query(&question, domain_name);
    // Print all the bytes in the DNS_HEADER, the request and the 4 bytes
    // of qtype qclass data.
    // printf("MESSAGE:\n");
    // _hex_print(question.query, DNS_HEADER_SIZE + strlen(DOMAIN_NAME) + 6);

    assert(buf == question.query);
    *out_size = question.size;
}

void SendDNS(unsigned char *buf, int size) {
    /*
     *  Sending request
     */
    assert(sockfd != 0);
    send(sockfd, buf, size, 0);
}

int RecvDNS(unsigned char *buf, int size) {
    assert(size == SIZE_OF_RESP);
    // unsigned char answer[SIZE_OF_RESP];
    memset(buf, 0, size);

    assert(sockfd != 0);
    int nb = recv(sockfd, buf, size, 0);
    return nb;

    // printf("====== %d ========\n", (long)threadid);
    // printf("RESPONSE:\n");
    // _hex_print(answer, 512);
    // parse_answer(answer, &question);
    // printf("==================\n");
}

int main(int argc, char **argv) {
    char *domain_name = DEFAULT_DOMAIN_NAME;
    char *name_server = DEFAULT_NAME_SERVER;
    int num_threads = DEFAULT_NUM_THREADS;

    int opt;
    while ((opt = getopt(argc, argv, "s:d:n:")) != -1) {
        switch (opt) {
            case 's':
                name_server = malloc(strlen(optarg));
                strcpy(name_server, optarg);
                break;
            case 'd':
                domain_name = malloc(strlen(optarg));
                strcpy(domain_name, optarg);
                break;
            case 'n':
                num_threads = atoi(optarg);
                break;
        }
    }

    srand(0); // Don't need real randomness here

    sockfd = client_get_socket(DNS_PORT, name_server);
    SetSocketBlockingEnabled(sockfd, false);

    unsigned char buf[num_threads][1 + DNS_HEADER_SIZE + strlen(domain_name) + 1 + 6];
    int buf_size[num_threads];
    for (int i = 0; i < num_threads; i++) {
        PrepareDNS(&buf[i][0], &buf_size[i], domain_name);
        assert(buf_size[i] != 0);
    }

    for (int i = 0; i < num_threads; i++) {
        SendDNS(buf[i], buf_size[i]);
    }

    long long send_time = current_timestamp();
    for (int i = 0; i < num_threads; i++) {
        printf("Thread %d sent a DNS query 0x%02x%02x to %s.\n", i, buf[i][0], buf[i][1], name_server);
    }
    SetSocketBlockingEnabled(sockfd, true);

    for (int i = 0; i < num_threads; i++) {
        unsigned char answer[SIZE_OF_RESP];
        int nb = RecvDNS(&answer, SIZE_OF_RESP);
        long long end_time = current_timestamp();
        printf("Thread %d received a DNS answer 0x%02x%02x with %d bytes. Time: %lld ms.\n", i, answer[0], answer[1],
               nb, end_time - send_time);
    }

    return 0;
}

int _hex_print(unsigned char *array, int size) {
    int i;
    printf("hex string: \n");
    for (i = 0; i < size; i++) {
        printf("%02hhX ", array[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
    return 1;
}
