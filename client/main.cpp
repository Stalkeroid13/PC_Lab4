#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602 // Вказуємо Windows 8 або вище
#endif

#include <iostream>
#include "protocol.h"

using namespace std;

int main()
{
    // Ініціалізація
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed!\n";
        return 1;
    }

    // Створення сокета
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    // Налаштування адреси сервера
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(DEFAULT_PORT);

    // Підключення
    if (connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        WSACleanup();
        cerr << "Connection failed: " << WSAGetLastError() << endl;
        return 1;
    }

    // Параметри
    uint32_t N = 100;
    uint32_t threads = 12;
    auto matrix_elements = static_cast<uint32_t>(N * N);
    size_t matrix_bytes = matrix_elements * sizeof(long long);

    // Створюємо пласку матрицю
    vector<long long> matrix(matrix_elements, 0);
    // Кожен елемент матриці треба перетворити в мережевий формат
    for(auto& element : matrix)
        element = htonll(element);

    // Готуємо дані конфігурації
    ConfigPayload config;
    config.matrixSize = htonl(N);
    config.threadCount = htonl(threads);

    // Підготовка заголовка й даних
    PacketHeader ph;
    ph.commandId = htonl(static_cast<uint32_t>(Command::SEND_DATA));
    ph.payloadSize = htonl(sizeof(ConfigPayload) + static_cast<uint32_t>(matrix_bytes));

    // Відправка заголовка й усіх даних
    send(sock, reinterpret_cast<char *>(&ph), sizeof(ph), 0);
    send(sock, reinterpret_cast<char*>(&config), sizeof(config), 0);
    cout << "Configuration sent!\n";

    // Відправляємо матрицю в циклі
    auto data_ptr = reinterpret_cast<char *>(matrix.data());
    size_t sent_total = 0;
    while (sent_total < matrix_bytes)
    {
        int sent = send(sock, data_ptr + sent_total, static_cast<int>(matrix_bytes - sent_total), 0);
        if (sent == SOCKET_ERROR)
            break;
        sent_total += sent;
    }
    cout << "Data sent!\n";

    closesocket(sock);
    WSACleanup();
    return 0;
}
