/**
 * @file stpctl.h
 * @brief Заголовочный файл для клиентских команд управления STP.
 *
 * Содержит определения макросов, структур и глобальных переменных,
 * используемых для взаимодействия с демоном STP через IPC-сокет.
 *
 * @copyright 2019 Broadcom
 * @license Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>
#include "stp_ipc.h"
#include <stdint.h>
#include <sys/un.h>
#include <stddef.h>
#include <errno.h>

/**
 * @def STP_CLIENT_SOCK
 * @brief Путь к UNIX-сокету клиента STP.
 *
 * Этот сокет используется для связи клиента с демоном STP.
 */
#define STP_CLIENT_SOCK "/var/run/client.sock"

/**
 * @def stpout
 * @brief Макрос для вывода сообщений в стандартный поток вывода.
 *
 * @param fmt Формат строки вывода, совместимый с printf.
 * @param ... Дополнительные аргументы для форматирования строки.
 */
#define stpout(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)

/**
 * @var stpd_fd
 * @brief Дескриптор сокета для связи с демоном STP.
 *
 * Используется для отправки команд и получения ответов от демона STP.
 */
int stpd_fd;

/**
 * @struct CMD_LIST
 * @brief Структура для хранения информации о командах STP.
 *
 * Используется для управления и обработки клиентских команд.
 */
typedef struct CMD_LIST
{
    char cmd_name[32]; /**< Имя команды (максимум 32 символа). */
    int cmd_type;      /**< Тип команды (определённый числовой код). */
} CMD_LIST;
