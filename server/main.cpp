#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602 // Вказуємо Windows 8 або вище
#endif

#include <iostream>
#include <thread>
#include <atomic>
#include "protocol.h"

using namespace std;

#include <future>

// Паралельне обчислення добутків стовпців для побічної діагоналі
void CountMatrix(vector<long long> &matrix, uint32_t N, uint32_t threadCount, atomic<bool> &stopFlag)
{
    auto worker = [&](uint32_t startCol, uint32_t endCol)
    {
        for (uint32_t c = startCol; c < endCol; c++)
        {
            if (stopFlag.load())
                return;

            long long product = 1;
            for (uint32_t r = 0; r < N; r++)
                // Побічна діагональ знаходиться за індексом:
                // row = N - 1 - col
                if (r != (N - 1 - c))
                    product *= matrix[r * N + c];
            // Записуємо результат у побічну діагональ стовпця
            matrix[(N - 1 - c) * N + c] = product;
        }
    };

    vector<thread> threads;
    uint32_t colsPerThread = N / threadCount;

    for (uint32_t i = 0; i < threadCount; i++)
    {
        uint32_t start = i * colsPerThread;
        uint32_t end = (i == threadCount - 1) ? N : (i + 1) * colsPerThread;
        threads.emplace_back(worker, start, end);
    }

    for (auto &t: threads)
        t.join();
}

void handleClient(SOCKET client_socket)
{
    uint32_t N = 0, threads = 0;
    vector<long long> matrix;
    future<void> processingFuture;

    // Прапорець зупинки
    auto stopFlag = make_shared<atomic<bool> >(false);

    while (true)
    {
        PacketHeader ph;
        int hr = recv(client_socket, reinterpret_cast<char *>(&ph), sizeof(ph), 0);
        if (hr <= 0)
        {
            stopFlag->store(true);
            cout << "Client disconnected. Ending tasks...\n";
            break;
        }

        Command cmd = static_cast<Command>(ntohl(ph.commandId));
        uint32_t payloadSize = ntohl(ph.payloadSize);

        switch (cmd)
        {
            case Command::SEND_DATA:
            {
                // Отримуємо конфігурацію
                ConfigPayload config;
                if (recv(client_socket, reinterpret_cast<char *>(&config), sizeof(config), 0) <= 0)
                    break;
                N = ntohl(config.matrixSize);
                threads = ntohl(config.threadCount);

                // Викачуємо матрицю
                auto elements = (size_t) (N * N);
                matrix.assign(elements, 0);
                auto ptr = reinterpret_cast<char *>(matrix.data());
                size_t total = 0;
                size_t target = elements * sizeof(long long);

                while (total < target)
                {
                    int r = recv(client_socket, ptr + total, static_cast<int>(target - total), 0);
                    if (r <= 0)
                        break;
                    total += r;
                }

                for (auto &el: matrix)
                    el = ntohll(el);
                cout << "Matrix " << N << "x" << N << " received!\n";

                break;
            }
            case Command::START_PROCESSING:
            {
                if (matrix.empty())
                    break;
                if (processingFuture.valid() && processingFuture.wait_for(0s) != future_status::ready)
                    break;
                *stopFlag = false;

                // Запускаємо обчислення асинхронно через async
                processingFuture = async(launch::async, CountMatrix, ref(matrix), N, threads, ref(*stopFlag));
                cout << "Processing started...\n";

                break;
            }
            case Command::GET_STATUS:
            {
                // Перевіряємо статус через future без блокування
                bool done = processingFuture.valid() &&
                            processingFuture.wait_for(chrono::seconds(0)) == future_status::ready;

                uint32_t status = done ? 1 : 0;
                uint32_t netStatus = htonl(status);
                send(client_socket, reinterpret_cast<char *>(&netStatus), sizeof(netStatus), 0);

                break;
            }
            case Command::GET_RESULT:
            {
                // Чекаємо фінального завершення, якщо ще не готово
                if (processingFuture.valid())
                    processingFuture.get();

                // Відправляємо матрицю назад клієнту
                for (auto &el: matrix)
                    el = htonll(el);
                send(client_socket, reinterpret_cast<char *>(matrix.data()),
                     static_cast<int>(matrix.size() * sizeof(long long)), 0);
                cout << "Result sent to client!\n";

                break;
            }
            default: ;
        }
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
