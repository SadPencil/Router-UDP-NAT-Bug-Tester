#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "socket_utils.h"
#include "c_utils.h"
#include "constructor.h"
#include "parser.h"
#include "dns_c.h"
#include <assert.h>
#include <sys/time.h>
#include <stdbool.h>

#define DEFAULT_NUM_THREADS 2
#define DOMAIN_NAME "www.google.com"

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

void SendDNS() {
    struct DNS_REQUEST question;
    /*
     *  Ahead of all else, make the head.
     */
    // TODO write some freakin macro's
    question.id[0] = 'A';       // 1010
    question.id[1] = 'B';       // 1010
    question.flags1 = '\1';     // 0001
    question.flags2 = '\0';     // 0000
    question.qdcount[0] = '\0'; // 0000
    question.qdcount[1] = '\1'; // 0001
    // query get's filled in at run time
    question.qtype[0] = '\0';  // 0000
    question.qtype[1] = '\1';  // 0001
    question.qclass[0] = '\0'; // 0000
    question.qclass[1] = '\1'; // 0001
    unsigned char query[1 + DNS_HEADER_SIZE + sizeof(DOMAIN_NAME) + 1 + 6];
    question.query = &query[0];
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
    build_query(&question, DOMAIN_NAME);
    // Print all the bytes in the DNS_HEADER, the request and the 4 bytes
    // of qtype qclass data.
    // printf("MESSAGE:\n");
    // _hex_print(question.query, DNS_HEADER_SIZE + strlen(DOMAIN_NAME) + 6);

    /*
     *  Sending request
     */
    assert(sockfd != 0);
    send(sockfd, question.query, question.size, 0);
}

int RecvDNS() {
    unsigned char answer[SIZE_OF_RESP];
    memset(answer, 0, SIZE_OF_RESP);

    assert(sockfd != 0);
    int nb = recv(sockfd, answer, 512, 0);
    return nb;

    // printf("====== %d ========\n", (long)threadid);
    // printf("RESPONSE:\n");
    // _hex_print(answer, 512);
    // parse_answer(answer, &question);
    // printf("==================\n");
}

int main(int argc, char **argv) {
    sockfd = client_get_socket(DNS_PORT, NAME_SERVER);

    int num_threads = DEFAULT_NUM_THREADS;
    if (argc >= 2)
        num_threads = atoi(argv[1]);

    long long send_time[num_threads];
    for (int i = 0; i < num_threads; i++) {
        SendDNS();
        send_time[i] = current_timestamp();
        printf("Thread %d sent a DNS query to %s.\n", i, NAME_SERVER);
    }
    for (int i = 0; i < num_threads; i++) {
        int nb = RecvDNS();
        long long end_time = current_timestamp();
        printf("Thread %d received a DNS answer with %d bytes. Time: %lld ms.\n", i, nb, end_time - send_time[i]);
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
