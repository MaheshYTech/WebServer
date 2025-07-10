#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 8192

// Extract filename from Content-Disposition line
std::string extract_filename(const std::string& header_line) {
    size_t start = header_line.find("filename=\"");
    if (start == std::string::npos) return "unknown";
    start += 10;
    size_t end = header_line.find("\"", start);
    return header_line.substr(start, end - start);
}

int mainsource() {
    WSADATA wsa;
    SOCKET server, client;
    struct sockaddr_in serverAddr, clientAddr;
    int clientLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];

    WSAStartup(MAKEWORD(2, 2), &wsa);
    server = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    bind(server, (struct sockaddr*) & serverAddr, sizeof(serverAddr));
    listen(server, 1);

    std::cout << "Listening on http://localhost:" << PORT << std::endl;
    client = accept(server, (struct sockaddr*) & clientAddr, &clientLen);


    int totalReceived = 0;
    std::string request;
    int received;

    while ((received = recv(client, buffer, BUFFER_SIZE, 0)) > 0) {
        request.append(buffer, received);
        totalReceived += received;
        std::cout << buffer << std::endl;
        if (request.find("\r\n\r\n") == std::string::npos) break; // headers complete
    }

    // 1. Get boundary from Content-Type
    std::string boundary;
    size_t bpos = request.find("boundary=");
    if (bpos != std::string::npos) {
        boundary = "--" + request.substr(bpos + 9, request.find("\r\n", bpos) - bpos - 9);
    }

    // 2. Get the body
    size_t body_start = request.find("\r\n\r\n") + 4;
    std::string body = request.substr(body_start);

    // 3. Split by boundary
    std::istringstream ss(body);
    std::string part;
    int fileIndex = 0;
    while (std::getline(ss, part, '\n')) {
        if (part.find(boundary) != std::string::npos) {
            std::string headers, filedata;
            std::getline(ss, headers);
            std::getline(ss, part); // Content-Type
            std::getline(ss, part); // empty line

            std::string filename = extract_filename(headers);
            std::ofstream out(filename.empty() ? ("file" + std::to_string(fileIndex++) + ".bin") : filename, std::ios::binary);

            // Read file content until next boundary
            std::string line;
            while (std::getline(ss, line)) {
                if (line.find(boundary) != std::string::npos)
                    break;
                out << line << "\n";
            }
            out.close();
        }
    }

    // Send response
    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body><h2>Multiple files received</h2></body></html>";
    send(client, response, strlen(response), 0);

    closesocket(client);
    closesocket(server);
    WSACleanup();
    return 0;
}