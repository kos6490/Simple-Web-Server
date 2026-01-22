#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char message[BUFFER_SIZE];

    // 1. 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 2. 서버 주소 변환 (127.0.0.1 -> 바이너리)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // 3. 연결 요청
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    printf("Connected to server! Type message (or 'exit' to quit):\n");

    // 4. 데이터 주고받기
    while(1) {
        printf("> ");
        fgets(message, BUFFER_SIZE, stdin); // 키보드 입력 받기
        message[strcspn(message, "\n")] = 0; // 개행문자 제거

        if (strcmp(message, "exit") == 0) break;

        send(sock, message, strlen(message), 0); // 서버로 전송
        
        memset(buffer, 0, BUFFER_SIZE);
        read(sock, buffer, BUFFER_SIZE); // 서버 응답 수신
        printf("Server echo: %s\n", buffer);
    }

    close(sock);
    return 0;
}