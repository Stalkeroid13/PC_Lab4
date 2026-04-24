#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602 // Вказуємо Windows 8 або вище
#endif

#include <iostream>
#include <thread>
#include "protocol.h"

using namespace std;

void handleClient(SOCKET client_socket)
{
    PacketHeader ph{};

    int bytes_received = recv(client_socket, reinterpret_cast<char *>(&ph), sizeof(ph), 0);

    if (bytes_received <= 0)
    {
        cerr << "Bytes receiving failed!\n";
        return;
    }

    uint32_t cmd = ntohl(ph.commandId);

    if (cmd == static_cast<uint32_t>(Command::SEND_DATA))
    {
        ConfigPayload config;
        recv(client_socket, reinterpret_cast<char *>(&config), sizeof(config), 0);

        uint32_t N = ntohl(config.matrixSize);
        auto matrix_elements = static_cast<size_t>(N * N);
        size_t matrix_bytes = matrix_elements * sizeof(long long);

        // Виділяємо пам'ять під матрицю
        vector<long long> matrix(matrix_elements);
        auto ptr = (char *) matrix.data();

        // Цикл викачування
        size_t received_total = 0;
        while (received_total < matrix_bytes)
        {
            int n = recv(client_socket, ptr + received_total, static_cast<int>(matrix_bytes - received_total), 0);
            if (n <= 0)
                break;
            received_total += n;
        }

        // Перетворюємо числа назад у формат ПК
        for (auto &element: matrix)
            element = ntohll(element);

        cout << "Successfully received matrix " << N << "x" << N << endl;

        closesocket(client_socket);
    }
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
