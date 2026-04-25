#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602 // Вказуємо Windows 8 або вище
#endif

#include <iostream>
#include "protocol.h"
#include <random>

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
    uint32_t N = 10000;
    uint32_t threads = 12;
    auto matrix_elements = N * N;
    size_t matrix_bytes = matrix_elements * sizeof(long long);

    // Створюємо пласку матрицю
    vector<long long> matrix(matrix_elements, 0);

    // Заповнення матриці
    random_device rnd;
    mt19937 gen(rnd());
    uniform_int_distribution<long long> dist(-9, 9);

    for (uint32_t r = 0; r < N; r++)
        for (uint32_t c = 0; c < N; c++)
        {
            int index = r * N + c;
            // Перевірка на побічну діагональ:
            // Рядок == N - 1 - стовпець
            if (c != N - 1 - r)
                matrix[index] = dist(gen);
            else
                matrix[index] = 1;
        }

    // Перед відправкою:
    cout << "Diagonal [0][N-1] before: " << matrix[0 * N + (N - 1)] << endl;

    // Кожен елемент матриці треба перетворити в мережевий формат
    for (auto &element: matrix)
        element = htonll(element);

    // Готуємо дані конфігурації
    ConfigPayload config;
    config.matrixSize = htonl(N);
    config.threadCount = htonl(threads);

    // Підготовка заголовка й даних
    PacketHeader data_ph;
    data_ph.commandId = htonl(static_cast<uint32_t>(Command::SEND_DATA));
    data_ph.payloadSize = htonl(sizeof(ConfigPayload) + static_cast<uint32_t>(matrix_bytes));

    // Відправка заголовка й усіх даних
    send(sock, reinterpret_cast<char *>(&data_ph), sizeof(data_ph), 0);
    send(sock, reinterpret_cast<char *>(&config), sizeof(config), 0);
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

    // Запукаємо обрахунок
    PacketHeader start_ph;
    start_ph.commandId = htonl(static_cast<uint32_t>(Command::START_PROCESSING));
    start_ph.payloadSize = htonl(0);
    send(sock, reinterpret_cast<char *>(&start_ph), sizeof(start_ph), 0);
    cout << "Start command sent!\n";

    // Опитуємо статус
    PacketHeader status_ph;
    status_ph.commandId = htonl(static_cast<uint32_t>(Command::GET_STATUS));
    status_ph.payloadSize = htonl(0);

    uint32_t isReady = 0;
    while (isReady == 0)
    {
        send(sock, reinterpret_cast<char *>(&status_ph), sizeof(status_ph), 0);
        recv(sock, reinterpret_cast<char *>(&isReady), sizeof(isReady), 0);
        isReady = ntohl(isReady);

        if (isReady == 0)
        {
            cout << "Still processing...\n";
            Sleep(500);
        }
    }
    cout << "Server is done!\n";

    // Отримуємо результат
    PacketHeader result_ph;
    result_ph.commandId = htonl(static_cast<uint32_t>(Command::GET_RESULT));
    result_ph.payloadSize = htonl(0);
    send(sock, reinterpret_cast<char *>(&result_ph), sizeof(result_ph), 0);

    // Викачуємо матрицю назад у той самий вектор
    auto ptr = reinterpret_cast<char *>(matrix.data());
    size_t total = 0;
    size_t target = matrix.size() * sizeof(long long);
    while (total < target)
    {
        int r = recv(sock, ptr + total, static_cast<int>(target - total), 0);
        if (r <= 0)
            break;
        total += r;
    }

    // Конвертуємо назад для перевірки
    for (auto &el: matrix) el = ntohll(el);
    cout << "Result received!\n";

    // Після отримання:
    cout << "Diagonal [0][N-1] after: " << matrix[0 * N + (N - 1)] << endl;

    closesocket(sock);
    WSACleanup();
    return 0;
}
