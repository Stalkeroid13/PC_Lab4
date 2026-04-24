#ifndef PC_LAB4_PROTOCOL_H
#define PC_LAB4_PROTOCOL_H

#include <winsock2.h>
#include <cstdint>

// Константи для команд
enum class Command : uint32_t
{
    SET_CONFIG = 1,         // Надіслати розмір матриці та кількість потоків
    SEND_DATA = 2,          // Надіслати саму матрицю
    START_PROCESSING = 3,   // Команда на початок обчислень
    GET_STATUS = 4,         // Запит статусу
    GET_RESULT = 5          // Запит готової матриці
};

// Заголовок пакету
#pragma pack(push, 1)       // Вимикаємо вирівнювання пам'яті для точної передачі по мережі
struct PacketHeader
{
    uint32_t commandId;     // ID команди (з enum Command)
    uint32_t payloadSize;   // Розмір даних, що йдуть ПІСЛЯ цього заголовка
};
#pragma pack(pop)

constexpr int DEFAULT_PORT = 7500;

#endif
