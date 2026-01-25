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
    int opt = 1;
    
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

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

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

    // 5. 수락 & 데이터 주고받기
    while(1) {
        // 연결 수락
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE] = {0};
        read(new_socket, buffer, BUFFER_SIZE);
        printf("Request: \n%s\n", buffer);

        // index.html 파일 열기
        FILE *file = fopen("index.html", "r");
        
        if (file == NULL) {
            printf("Error: File not found\n");
            char *err_msg = "HTTP/1.1 404 Not Found\r\n\r\nFile Not Found";
            send(new_socket, err_msg, strlen(err_msg), 0);
        } else {
            // 헤더 전송
            char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
            send(new_socket, header, strlen(header), 0);

            // 파일 내용 전송
            char file_buf[BUFFER_SIZE];

            while (fgets(file_buf, BUFFER_SIZE, file) != NULL) {
                send(new_socket, file_buf, strlen(file_buf), 0);
            }
            
            fclose(file);
            printf(">> File sent successfully!\n");
        }

        close(new_socket);
    }
    
    close(server_fd);
    return 0;
}