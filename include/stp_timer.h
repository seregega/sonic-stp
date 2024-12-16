/**
 * @file stp_timer.h
 * @brief Заголовочный файл для работы с таймерами linux
 *
 * Содержит определения структур, констант, перечислений и макросов,
 * используемых для управления таймерами и задержками
 *
 * @copyright 2019 Broadcom.
 * @license Apache License, Version 2.0.
 */

#ifndef __STP_TIMER_H__
#define __STP_TIMER_H__

/*#include "kstart.h"*/

/* Application Timers Library
 * - Allow applications to use one system timer tick to process
 *   multiple application timers
 * - Applications would register to be notified about the system clock tick
 *   maybe every 100 ms. For a 100ms timer, this implementation can
 *   time upto 54 minutes.
 * - Applications requiring a longer time can use this functionality by making
 *   their tick function coarser. For an example, look at stptimer_tick().
 * - For an example routine, look at the comment at the end of this file
 */

/**
 * @struct TIMER
 * @brief Структура для управления таймерами.
 */
typedef struct TIMER
{
	UINT32 active : 1; // флаг статуса таймера
	UINT32 value : 31; // поле тиков таймера
} TIMER;

uint32_t sys_get_seconds(); // функция для получения значения секунд из тиков?
/*
 * start_timer()
 *		this function initializes the timer to the input start_value_in_ticks
 *		and marks it as an active timer.
 */

/**
 * @brief Функция для инициализации и запуска таймера с заданным начальным значением
 *
 * @param timer указатель на структуру таймера
 * @param start_value_in_ticks начальное значение
 */
void start_timer(TIMER *timer, UINT32 start_value_in_ticks);

/**
 * @brief останавливает таймер и обновляет статус
 *
 * @param timer указатель на структуру таймера
 */
void stop_timer(TIMER *timer);

/*
 * timer_expired()
 *		can be called every system tick.
 *		- if the timer is inactive, this function will return FALSE
 *		- if the timer is active, this function increments the timer value
 *		  by 1. after incrementing, checks if the timer value exceeds the
 *		  timer_limit_in_ticks. if it exceeds or equal to the limit, stops the
 *		  timer and returns TRUE, other wise returns FALSE
 */

/**
 * @brief вызывается каждый системный тик
 *
 * @param timer указатель на структуру таймера
 * @param timer_limit_in_ticks if the timer is active, this function increments the timer value
 * by 1. after incrementing, checks if the timer value exceeds the
 * timer_limit_in_ticks. if it exceeds or equal to the limit, stops the
 * timer and returns TRUE, other wise returns FALSE
 * @return true
 * @return false if the timer is inactive, this function will return FALSE
 */
bool timer_expired(TIMER *timer, UINT32 timer_limit_in_ticks);

/**
 * @brief возвращает состояние таймера
 *
 * @param timer указатель на структуру таймера
 * @return true returns TRUE if the timer is active
 * @return false  else returns FALSE
 */
bool is_timer_active(TIMER *timer);

/**
 * @brief Get the timer value object
 *
 * @param timer указатель на структуру таймера
 * @param value_in_ticks fills in the the current value of the timer in ticks
 * @return true
 * @return false return FALSE if the timer is inactive
 */
bool get_timer_value(TIMER *timer, UINT32 *value_in_ticks);

/* USAGE EXAMPLE
 * ---------------------------------------------------------------------------
 *
 * - when system tick arrives, call application timer processing routine
 *   for example, app_tick()
 *
 *	void app_tick()
 *	{
 *		if (timer_expired(&timer1, 10)) // timer expiry every second
 *		{
 *			// do timer expiry routine for timer1
 *			timer1_expiry();
 *		}
 *
 *		if (timer_expired(&timer2, 100)) // timer expiry every 10 seconds
 *		{
 *			// do timer expiry routine for timer2
 *		}
 *	}
 *
 *	void timer1_expiry()
 *	{
 *		// processing
 *		start_timer(&timer1, 0); // restart timer
 *	}
 *
 *	void timer2_expiry()
 *	{
 *		if (condition)
 *		{
 *			// processing - do not restart timer
 *		}
 *		else
 *		{
 *			// else processing
 *			start_timer(&timer2, 10); // restart timer from second 1
 *		}
 *	}
 */
#endif /* __STP_TIMER_H__ */
