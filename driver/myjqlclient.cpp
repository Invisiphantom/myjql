#include "myjqlclient.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>

#define SOCKET_ERROR -1
#define SD_SEND SHUT_WR

void closesocket(int fd) {
    close(fd);
}

void WSACleanup() {}

int WSAGetLastError() {
    return 0;
}

namespace MyJQL {

client::client() {
    sockaddr_in serv_addr;
    hostent* server;

    /* Create a socket point */
    ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ConnectSocket < 0) {
        throw system_error{"socket", 0};
    }

    server = gethostbyname(DEFAULT_HOST);
    if (server == NULL) {
        throw system_error{"gethostbyname", 0};
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(DEFAULT_PORT);

    /* Now connect to the server */
    if (connect(ConnectSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        throw system_error{"connect", 0};
    }
}

client::~client() {
    // send: 3
    // recv: 1 (ok)
    // 3
    int code = 3;
    int iResult = send(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    // 1
    iResult = recv(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
}

std::optional<std::string> client::get(std::string_view key) {
    // get key
    // send: 0 key_len key
    // recv: 0 (not found)
    //       1 value_len value
    //       2 (key too long)
    // 0
    int code = 0;
    int iResult = send(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    // key
    code = (int)key.size();
    iResult = send(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    iResult = send(ConnectSocket, key.data(), code, 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    // code
    iResult = recv(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"recv", WSAGetLastError()};
    }
    if (code == 0) {
        return std::nullopt;
    } else if (code == 1) {
        // value
        iResult = recv(ConnectSocket, (char*)&code, sizeof(code), 0);
        if (iResult == SOCKET_ERROR) {
            throw system_error{"recv", WSAGetLastError()};
        }
        char* buf = (char*)malloc(code + 1);
        iResult = recv(ConnectSocket, buf, code, 0);
        if (iResult == SOCKET_ERROR) {
            free(buf);
            throw system_error{"recv", WSAGetLastError()};
        }
        if (iResult != code) {
            free(buf);
            throw system_error{"incomplete response", 0};
        } else {
            buf[code] = 0;
            std::string res = buf;
            free(buf);
            return res;
        }
    } else if (code == 2) {
        throw key_too_long{};
    } else {
        throw system_error{"unknown return code", code};
    }
}

void client::set(std::string_view key, std::string_view value) {
    // set key value
    // send: 1 key_len key value_len value
    // recv: 1 (ok)
    //       2 (key too long)
    //       3 (value too long)
    // 1
    int code = 1;
    int iResult = send(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    // key
    code = (int)key.size();
    iResult = send(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    iResult = send(ConnectSocket, key.data(), code, 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    // value
    code = (int)value.size();
    iResult = send(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    iResult = send(ConnectSocket, value.data(), code, 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    // code
    iResult = recv(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"recv", WSAGetLastError()};
    }
    if (code == 1) {
        // OK
    } else if (code == 2) {
        throw key_too_long{};
    } else if (code == 3) {
        throw value_too_long{};
    } else {
        throw system_error{"unknown return code", code};
    }
}

void client::del(std::string_view key) {
    // del key
    // send: 2 key_len key
    // recv: 1 (ok)
    //       2 (key too long)
    // 2
    int code = 2;
    int iResult = send(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    // key
    code = (int)key.size();
    iResult = send(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    iResult = send(ConnectSocket, key.data(), code, 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"send", WSAGetLastError()};
    }
    // code
    iResult = recv(ConnectSocket, (char*)&code, sizeof(code), 0);
    if (iResult == SOCKET_ERROR) {
        throw system_error{"recv", WSAGetLastError()};
    }
    if (code == 1) {
        // OK
    } else if (code == 2) {
        throw key_too_long{};
    } else {
        throw system_error{"unknown return code", code};
    }
}

}  // namespace MyJQL