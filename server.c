#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    // 1. 소켓 생성
    // AF_INET: IPv4, SOCK_STREAM: TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. 주소 설정
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 내 컴퓨터의 모든 IP로 접속 허용
    address.sin_port = htons(PORT);       // 포트 번호 8080

    // 3. 바인딩
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. 리슨
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    // 5. 수락
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Client connected!\n");

    // 6. 데이터 주고받기
    while(1) {
        memset(buffer, 0, BUFFER_SIZE); // 버퍼 비우기
        int valread = read(new_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) break; // 연결 끊기면 종료

        printf("Client said: %s\n", buffer);
        
        // 그대로 돌려주기 (Echo)
        send(new_socket, buffer, strlen(buffer), 0);
    }

    printf("Client disconnected.\n");
    close(new_socket);
    close(server_fd);
    return 0;
}