#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include "core/runtime.h"

#pragma comment(lib, "ws2_32.lib")

// Simple helper to read entire request into a string
std::string recv_all(SOCKET client) {
    const int bufSize = 4096;
    char buf[bufSize];
    std::string data;
    int received = 0;
    // Read until we encounter "\r\n\r\n" (end of headers) then read Content-Length bytes if present
    while (true) {
        received = recv(client, buf, bufSize, 0);
        if (received <= 0) break;
        data.append(buf, received);
        // if we already have end of headers, break to parse body separately
        if (data.find("\r\n\r\n") != std::string::npos) break;
    }
    // Parse Content-Length (if any)
    size_t pos = data.find("Content-Length:");
    size_t bodyStart = data.find("\r\n\r\n");
    if (pos != std::string::npos && bodyStart != std::string::npos) {
        size_t lenStart = data.find_first_of("0123456789", pos);
        size_t lenEnd = data.find_first_not_of("0123456789", lenStart);
        int contentLength = std::stoi(data.substr(lenStart, lenEnd - lenStart));
        size_t alreadyRead = data.size() - (bodyStart + 4);
        while (alreadyRead < static_cast<size_t>(contentLength)) {
            received = recv(client, buf, bufSize, 0);
            if (received <= 0) break;
            data.append(buf, received);
            alreadyRead += received;
        }
    }
    return data;
}

// Extract the JSON body (very naive) and pull out the "prompt" value
std::string extract_prompt(const std::string& request) {
    size_t jsonPos = request.find("{\"");
    if (jsonPos == std::string::npos) return "";
    std::string json = request.substr(jsonPos);
    // Find "prompt":"..."
    size_t keyPos = json.find("\"prompt\"");
    if (keyPos == std::string::npos) return "";
    size_t colon = json.find(":", keyPos);
    size_t firstQuote = json.find("\"", colon);
    size_t secondQuote = json.find("\"", firstQuote + 1);
    return json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

std::string make_http_response(const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: application/json; charset=utf-8\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << body;
    return oss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model_path> [port=8080]\n";
        return 1;
    }
    std::string modelPath = argv[1];
    int port = 8080;
    if (argc >= 3) port = std::stoi(argv[2]);

    // Initialise runtime
    auto runtime = llm::core::LLMRuntime::create();
    if (!runtime->initialize(modelPath)) {
        std::cerr << "Failed to initialise LLM runtime" << std::endl;
        return 1;
    }

    // Initialise Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(static_cast<u_short>(port));
    if (bind(listenSock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    std::cout << "On‑Device LLM HTTP service listening on port " << port << "..." << std::endl;

    while (true) {
        SOCKET clientSock = accept(listenSock, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            break;
        }
        // Receive request
        std::string request = recv_all(clientSock);
        std::string prompt = extract_prompt(request);
        std::string responseBody;
        if (!prompt.empty()) {
            std::string answer = runtime->generate(prompt, 128);
            responseBody = "{\"response\": \"" + answer + "\"}";
        } else {
            responseBody = "{\"error\": \"Missing prompt in JSON body\"}";
        }
        std::string httpResp = make_http_response(responseBody);
        send(clientSock, httpResp.c_str(), static_cast<int>(httpResp.size()), 0);
        closesocket(clientSock);
    }

    closesocket(listenSock);
    WSACleanup();
    runtime->shutdown();
    return 0;
}
