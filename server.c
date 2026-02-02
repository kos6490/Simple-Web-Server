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
        int valread = read(new_socket, buffer, BUFFER_SIZE);
        
        if(valread <= 0) {
            close(new_socket);
            continue;
        }

        char method[10], path[100], protocol[10];
        // 첫 줄에서 "GET", "/index.html", "HTTP/1.1"을 분리해서 저장
        if (sscanf(buffer, "%s %s %s", method, path, protocol) < 3) {
            close(new_socket);
            continue;
        }
        printf("User requested: %s\n", path);

        // 파일 이름 결정 (기본값은 index.html)
        char file_name[110];
        if (strcmp(path, "/") == 0) {
            strcpy(file_name, "index.html");
        } else {
            // 경로의 앞 글자 '/'를 파일 이름에서 제외
            strcpy(file_name, path + 1);
        }

        // 결정된 file_name으로 파일 열기
        FILE *file = fopen(file_name, "rb");
        
        if (file == NULL) {
            printf(">> [Error] File not found: %s\n", file_name);
            char *err_msg = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nFile Not Found";
            send(new_socket, err_msg, strlen(err_msg), 0);
        } else {
            // 확장자 구분하여 헤더 설정
            char *content_type = "text/html";
            if(strstr(file_name, ".jpg") || strstr(file_name, ".jpeg")) content_type = "image/jpeg";
            else if(strstr(file_name, ".png")) content_type = "image/png";

            // 헤더 전송
            char header[256];
            sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s; charset=UTF-8\r\n\r\n", content_type);
            send(new_socket, header, strlen(header), 0);

            // 파일 내용 전송 (텍스트/이미지 모두 전송)
            unsigned char file_buf[BUFFER_SIZE];
            int bytes_read;
            while ((bytes_read = fread(file_buf, 1, BUFFER_SIZE, file)) > 0) {
                send(new_socket, file_buf, bytes_read, 0);
            }
            
            fclose(file);
            printf(">> [Success] Sent: %s as %s\n", file_name, content_type);
        }

        close(new_socket);
    }
    
    close(server_fd);
    return 0;
}