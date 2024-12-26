/**
 * @file applog.c
 * @author your name (you@domain.com)
 * @brief реализует подсистему для работы с журналированием в приложении.
 * Эта подсистема позволяет инициализировать, конфигурировать,
 * писать в системный журнал, а также завершать работу с логами.
 * @version 0.1
 * @date 2024-12-12
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

#include "applog.h"

int applog_level_map[APP_LOG_LEVEL_MAX + 2];     // Массив для сопоставления уровней пользовательских логов (например, APP_LOG_LEVEL_ERR) с уровнями системного журнала (LOG_ERR).
int applog_config_level = APP_LOG_LEVEL_DEFAULT; // Хранит текущий конфигурационный уровень логирования.
int applog_inited = 0;                           // Указывает, была ли подсистема логирования инициализирована

/**
 * @brief Инициализирует подсистему логирования.
 * Заполняет массив applog_level_map, связывая уровни пользовательских логов с уровнями системного журнала (syslog).
 * Вызывает функцию openlog, чтобы настроить системный журнал.
 * Устанавливает уровень логирования по умолчанию (APP_LOG_LEVEL_DEFAULT).
 * Устанавливает флаг applog_inited в 1, сигнализируя, что логирование инициализировано.
 *
 * @return int APP_LOG_STATUS_OK
 */
int applog_init()
{
  memset(applog_level_map, 0, sizeof(applog_level_map));

  /* match user level to system level */
  applog_level_map[APP_LOG_LEVEL_EMERG] = LOG_EMERG;
  applog_level_map[APP_LOG_LEVEL_ALERT] = LOG_ALERT;
  applog_level_map[APP_LOG_LEVEL_CRIT] = LOG_CRIT;
  applog_level_map[APP_LOG_LEVEL_ERR] = LOG_ERR;
  applog_level_map[APP_LOG_LEVEL_WARNING] = LOG_WARNING;
  applog_level_map[APP_LOG_LEVEL_NOTICE] = LOG_NOTICE;
  applog_level_map[APP_LOG_LEVEL_INFO] = LOG_INFO;
  applog_level_map[APP_LOG_LEVEL_DEBUG] = LOG_DEBUG;

  openlog("stpd", LOG_NDELAY | LOG_CONS, LOG_DAEMON);

  applog_inited = 1;
  applog_config_level = APP_LOG_LEVEL_DEFAULT;

  return APP_LOG_STATUS_OK;
}

/**
 * @brief Проверяет, была ли подсистема логирования инициализирована.
 *
 * @return int 1, если инициализировано, 0 — если нет
 */
int applog_get_init_status()
{
  return applog_inited;
}

/**
 * @brief Устанавливает конфигурационный уровень логирования.
 * Проверяет, что переданный уровень находится в допустимом диапазоне.
 * Если уровень валиден, сохраняет его в переменной applog_config_level.

 * @param level
 * @return int APP_LOG_STATUS_OK при успешной установке. APP_LOG_STATUS_INVALID_LEVEL при недопустимом уровне.
 */
int applog_set_config_level(int level)
{
  if (level < APP_LOG_LEVEL_NONE || level > APP_LOG_LEVEL_MAX)
  {
    return APP_LOG_STATUS_INVALID_LEVEL;
  }
  else
  {
    applog_config_level = level;
  }

  return APP_LOG_STATUS_OK;
}

/**
 * @brief Возвращает текущий уровень логирования (applog_config_level).
 *
 * @return int
 */
int applog_get_config_level()
{
  return applog_config_level;
}

/**
 * @brief Основная функция для записи в журнал
 * Проверяет, что переданный уровень валиден и не превышает текущий уровень конфигурации.
 * Преобразует уровень в системный приоритет через applog_level_map.
 * спользует vsyslog для записи форматированной строки в журнал
 *
 * @param level
 * @param fmt
 * @param ...
 * @return int APP_LOG_STATUS_OK при успешной записи.
 * APP_LOG_STATUS_INVALID_LEVEL, если уровень недопустим.
 * APP_LOG_STATUS_LEVEL_DISABLED, если уровень ниже текущего уровня конфигурации.
 */
int applog_write(int level, const char *fmt, ...)
{
  int priority;
  va_list ap;

  if (level < APP_LOG_LEVEL_MIN || level > APP_LOG_LEVEL_MAX)
  {
    return APP_LOG_STATUS_INVALID_LEVEL;
  }

  if (level > applog_config_level)
  {
    return APP_LOG_STATUS_LEVEL_DISABLED;
  }

  priority = applog_level_map[level];

  va_start(ap, fmt);
  vsyslog(priority, fmt, ap);
  va_end(ap);

  return APP_LOG_STATUS_OK;
}

/**
 * @brief Закрывает подсистему логирования.
 * Сбрасывает уровень конфигурации на значение по умолчанию.
 *
 * @return int 0
 */
int applog_deinit()
{
  int status = 0;

  closelog();

  applog_inited = 0;
  applog_config_level = APP_LOG_LEVEL_DEFAULT;

  return status;
}
