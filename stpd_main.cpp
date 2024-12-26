/**
 * @file stpd_main.cpp
 * @author your name (you@domain.com)
 * @brief Главный файл, содержащий точку входа для демона STP. Выполняет общую инициализацию и запускает основные модули.
 * Основные задачи:
 * Настройка окружения (сигналы, логирование).
 * Запуск потока обработки пакетов BPDU.
 * Управление жизненным циклом демона.
 * @version 0.1
 * @date 2024-11-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <iostream>

extern "C" void stpd_main();

/**
 * @brief Основная функция, запускающая демон. принимает аргументы командной строки, инициализирует логирование и вызывает вспомогательные функции.
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char **argv)
{
    stpd_main();
    return 0;
}
