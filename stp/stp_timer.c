/**
 * @file stp_timer.c
 * @brief Реализация таймеров для протокола STP (Spanning Tree Protocol).
 *
 * Этот файл содержит функции управления таймерами, которые используются
 * для различных операций протокола STP, таких как управление задержками
 * и отслеживание времени.
 *
 * @details
 * Реализованные функции:
 * - Запуск и остановка таймеров.
 * - Проверка активности таймеров.
 * - Проверка истечения времени таймеров.
 * - Получение текущего значения таймера.
 *
 * @author
 * Broadcom, 2019. Лицензия Apache License 2.0.
 */
#include "stp_inc.h"

/**
 * @brief Возвращает текущее время в секундах.
 *
 * Эта функция получает текущее время с использованием системного таймера
 * (CLOCK_MONOTONIC) и возвращает его в секундах.
 *
 * @return Текущее время в секундах.
 */
uint32_t sys_get_seconds()
{
	struct timespec ts = {0, 0};
	if (-1 == clock_gettime(CLOCK_MONOTONIC, &ts))
	{
		STP_LOG_CRITICAL("clock_gettime Failed : %s", strerror(errno));
		sys_assert(0);
	}
	return ts.tv_sec;
}

/**
 * @brief Запускает таймер с заданным значением.
 *
 * Эта функция активирует таймер и инициализирует его начальным значением.
 *
 * @param timer Указатель на структуру таймера.
 * @param value Начальное значение таймера.
 */
void start_timer(TIMER *timer, UINT32 value)
{
	timer->active = true;
	timer->value = value;
}

/**
 * @brief Останавливает таймер.
 *
 * Эта функция деактивирует таймер и сбрасывает его значение.
 *
 * @param timer Указатель на структуру таймера.
 */
void stop_timer(TIMER *timer)
{
	timer->active = false;
	timer->value = 0;
}

/**
 * @brief Проверяет, истёк ли таймер.
 *
 * Эта функция увеличивает значение таймера, если он активен. Если значение
 * таймера превышает заданный лимит, таймер останавливается, и возвращается
 * true.
 *
 * @param timer Указатель на структуру таймера.
 * @param timer_limit Лимит времени, после которого таймер считается истекшим.
 * @return true, если таймер истёк; иначе false.
 */
bool timer_expired(TIMER *timer, UINT32 timer_limit)
{
	if (timer->active)
	{
		timer->value++;
		if (timer->value >= timer_limit)
		{
			stop_timer(timer);
			return true;
		}
	}

	return false;
}

/**
 * @brief Проверяет, активен ли таймер.
 *
 * Эта функция возвращает состояние активности таймера.
 *
 * @param timer Указатель на структуру таймера.
 * @return true, если таймер активен; иначе false.
 */
bool is_timer_active(TIMER *timer)
{
	return ((timer->active) ? true : false);
}

/**
 * @brief Получает текущее значение таймера.
 *
 * Эта функция возвращает текущее значение таймера, если он активен.
 *
 * @param timer Указатель на структуру таймера.
 * @param value_in_ticks Указатель для записи текущего значения таймера в тиках.
 * @return true, если таймер активен и значение успешно получено; иначе false.
 */
bool get_timer_value(TIMER *timer, UINT32 *value_in_ticks)
{
	if (!timer->active)
		return false;

	*value_in_ticks = timer->value;
	return true;
}
