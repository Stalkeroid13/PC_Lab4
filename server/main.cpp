#include <iostream>
#include <thread>
#include <vector>
#include "protocol.h"

using namespace std;

void handleClient(SOCKET client_socket)
{
    cout << "Thread " << this_thread::get_id() << " handling new client...\n";
}

int main()
{
    // Ініціалізація WinSock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed!\n";
        return 1;
    }

    // Створення сокета для прослуховуванн
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET)
    {
        WSACleanup();
        cerr << "Socket failed!\n";
        return 1;
    }

    // Налаштування адреси (всі інтерфейси, порт 7500)
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DEFAULT_PORT);

    // Прив'язка та перехід у режим очікування
    if (bind(listen_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(listen_socket);
        WSACleanup();
        cerr << "Bind failed!\n";
        return 1;
    }

    listen(listen_socket, SOMAXCONN);
    cout << "Server started on port " << DEFAULT_PORT << ". Waiting for clients...\n";

    while (true)
    {
        SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET)
            continue;

        cout << "New client connected!\n";

        // Створюємо новий потік для обслуговування клієнта
        // Використовуємо detach, щоб потік працював незалежно від main

        thread client_thread(handleClient, client_socket);
        client_thread.detach();
    }

    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
