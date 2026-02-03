#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sqlite3.h>

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

        // 경로가 /users 일 때 DB 조회
        if(strcmp(path, "/users") == 0) {
            sqlite3 *db;
            sqlite3_stmt *res;

            // 1. DB 열기
            if(sqlite3_open("test.db", &db) != SQLITE_OK) {
                char *err_msg = "HTTP/1.1 500 Internal Server Error\r\n\r\nDB Error";
                send(new_socket, err_msg, strlen(err_msg), 0);
            } else {
                // 2. HTML 헤더 먼저 전송
                char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
                send(new_socket, header, strlen(header), 0);

                // 3. HTML 본문 조립 시작
                char body[2048] = "<html><body><h1>User List (from DB)</h1><ul>";

                // 4. DB 데이터 조회 (SELECT)
                char *sql = "SELECT Name FROM Users";
                sqlite3_prepare_v2(db, sql, -1, &res, 0);

                while(sqlite3_step(res) == SQLITE_ROW) {
                    const char* name = (const char *)sqlite3_column_text(res, 0);
                    strcat(body, "<li>");
                    strcat(body, name);

                    //삭제 버튼 추가
                    strcat(body, "  <a href='/delete_user?name=");
                    strcat(body, name);
                    strcat(body, "' style='color:red; font-size:0.8em;'>[삭제]</a>");
                    strcat(body, "<li>");
                }

                strcat(body, "</ul><a href='/'>Back to Home</a></body></html>");

                // 5. 최종 결과 전송
                send(new_socket, body, strlen(body), 0);

                sqlite3_finalize(res);
                sqlite3_close(db);
            }

            close(new_socket);
            continue;
        }

        if (strncmp(path, "/add_user", 9) == 0) {
            // 1. URL에서 이름(name) 추출하기 (예: /add_user?name=user1)
            char *name_ptr = strstr(path, "name=");
            if (name_ptr != NULL) {
                name_ptr += 5; // "name=" 이후의 입력된 실제 이름 시작 지점
        
                sqlite3 *db;
                if (sqlite3_open("test.db", &db) == SQLITE_OK) {
                    char sql[256];
                    // 2. DB에 삽입 (INSERT)
                    sprintf(sql, "INSERT INTO Users (Name) VALUES ('%s');", name_ptr);
                    sqlite3_exec(db, sql, 0, 0, 0);
                    sqlite3_close(db);
                    printf(">> [DB Success] Added user: %s\n", name_ptr);
                }
            }

            // 3. 등록 후 다시 목록 페이지로 리다이렉트
            char *msg = "HTTP/1.1 303 See Other\r\nLocation: /users\r\n\r\n";
            send(new_socket, msg, strlen(msg), 0);
            close(new_socket);
            continue;
        }

        if (strncmp(path, "/delete_user", 12) == 0) {
            char *name_ptr = strstr(path, "name=");
            if (name_ptr != NULL) {
                name_ptr += 5;
        
                sqlite3 *db;
                if (sqlite3_open("test.db", &db) == SQLITE_OK) {
                    char sql[256];
                    // SQL 삭제 명령 (DELETE)
                    sprintf(sql, "DELETE FROM Users WHERE Name = '%s';", name_ptr);
                    sqlite3_exec(db, sql, 0, 0, 0);
                    sqlite3_close(db);
                    printf(">> [DB Success] Deleted user: %s\n", name_ptr);
                }
            }

            // 삭제 후 다시 목록 페이지로 리다이렉트
            char *msg = "HTTP/1.1 303 See Other\r\nLocation: /users\r\n\r\n";
            send(new_socket, msg, strlen(msg), 0);
            close(new_socket);
            continue;
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