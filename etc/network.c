#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define RCV_BUF_SIZE 36
#define RCV_TIMEOUT 3
#define CMN_CNK_SIZE 1536

int makeSocket(){
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror("socket open");
        fprintf(stderr, "fail to open socket\n");
        return -1;
    }

    return sock;
}

int connectTo(int sock, const char *dest, unsigned short int port){
    struct sockaddr_in target;

    // 接続先指定用構造体の作成
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(dest);
    if (target.sin_addr.s_addr == 0xffffffff) {
        struct hostent *host;

        host = gethostbyname(dest);
        if (host == NULL) {
            return -1;
        }
        target.sin_addr.s_addr = *(unsigned int *)host->h_addr_list[0];
    }

    fprintf(stderr, "connecting...\n");
    if (connect(sock, (struct sockaddr *)&target, sizeof(target)) == -1){
        perror("connect");
        fprintf(stderr, "fail to connect\n");
        return -1;
    }

    return 1;
}


int receiveDataWithSize(int sock, char *buf, const int n){
    char *tmpBuf[RCV_BUF_SIZE];
    int rmn = n;
    int rcv = 0;
    while(rmn > 0){
        if ((rcv = read(sock, tmpBuf, RCV_BUF_SIZE < rmn ? RCV_BUF_SIZE : rmn)) == -1){
            perror("fail to receive data from server");
            return -1;
        }
        memcpy(buf + (n-rmn), tmpBuf, rcv);
        rmn -= rcv;
    }
    return n;
}


int receiveData(int sock, char *buf){
    return receiveDataWithSize(sock, buf, sizeof(buf));
}


int sendC0(int sock){
    char hello = 3;

    fprintf(stderr, "sending C0...\n");

    int ret = write(sock, &hello, 1);
    if (ret == -1){
        perror("fail to send C0 chunk");
        return -1;
    }

    return ret;
}


int sendC1(int sock){
    int time = 0;
    int zero = 0;
    char random[1528];
    memset(random, 9, sizeof(random));
    char payload[CMN_CNK_SIZE];
    memcpy(payload, &time, 4);
    memcpy(payload+4, &zero, 4);
    memcpy(payload+8, random, CMN_CNK_SIZE-8);

    fprintf(stderr, "sending C1...\n");

    int ret = write(sock, payload, CMN_CNK_SIZE);
    if (ret == -1){
        perror("fail to send C1 chunk");
        return -1;
    }
    return ret;
}


int handshakeWith(int sock, char *host, int port){
    if (connectTo(sock, host, port) == -1)
        return 1;
    else
        fprintf(stderr, "connect established\n");

    if (sendC0(sock) == -1)
        return 1;

    if (sendC1(sock) == -1)
        return 1;

    fprintf(stderr, "receiving data...\n");

    struct timeval tv;
    tv.tv_sec = RCV_TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

    char s0 = 0;
    if (receiveDataWithSize(sock, &s0, 1) != 1 || s0 != 3){
        fprintf(stderr, "fail to receive s0 chunk\n");
        return -1;
    }
    write(1, &s0, 1);

    char s1[CMN_CNK_SIZE];
    memset(s1, 0, CMN_CNK_SIZE);
    if (receiveDataWithSize(sock, s1, CMN_CNK_SIZE) != CMN_CNK_SIZE){
        fprintf(stderr, "fail to receive s1 chunk\n");
        return -1;
    }
    write(1, s1, CMN_CNK_SIZE);

    //send c2
    if(write(sock, s1, CMN_CNK_SIZE) == -1){
        perror("fail to send c2");
        return -1;
    }

    //receive s2
    char s2[CMN_CNK_SIZE];
    if (receiveDataWithSize(sock, s2, CMN_CNK_SIZE) != CMN_CNK_SIZE){
        fprintf(stderr, "fail to receive s2 chunk\n");
        return -1;
    }
    write(1, s2, CMN_CNK_SIZE);

    return 0;
}


int main(int argc, char **argv){
    if (argc != 3){
        fprintf(stderr, "invalid argument\nvalid paramater is - hostname port\n");
        return 1;
    }
    char *host = argv[1];
    int port = atoi(argv[2]);

    int sock = makeSocket();
    if (sock == -1)
        return 1;
    else
        fprintf(stderr, "socket open\n");

    handshakeWith(sock, host, port);

    return 0;
}
