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

    // Підготовка заголовка
    PacketHeader ph;
    ph.commandId = htonl(static_cast<uint32_t>(Command::SET_CONFIG));
    ph.payloadSize = htonl(0);

    // Відправка заголовка
    send(sock, reinterpret_cast<char *>(&ph), sizeof(ph), 0);
    cout << "Header sent!\n";

    closesocket(sock);
    WSACleanup();
    return 0;
}
