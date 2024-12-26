/**
 * @file applog.h
 * @brief Application logging library.
 *
 * This library provides an interface for logging messages in applications.
 * It supports various log levels and integrates with the system logger (syslog).
 *
 * Copyright 2019 Broadcom.
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef _APPLOG_H_
#define _APPLOG_H_

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>

int applog_init();
int applog_deinit();
int applog_set_config_level(int level);
int applog_get_config_level();
int applog_get_init_status();
int applog_write(int priority, const char *fmt, ...);

/**
 * @defgroup APP_LOG_LEVELS Log Levels
 * @brief Defines the log levels used by the application.
 * @{
 */
#define APP_LOG_LEVEL_NONE (-1)   /**< No logging. */
#define APP_LOG_LEVEL_EMERG (0)   /**< System is unusable. */
#define APP_LOG_LEVEL_ALERT (1)   /**< Immediate action required. */
#define APP_LOG_LEVEL_CRIT (2)    /**< Critical conditions. */
#define APP_LOG_LEVEL_ERR (3)     /**< Error conditions. */
#define APP_LOG_LEVEL_WARNING (4) /**< Warning conditions. */
#define APP_LOG_LEVEL_NOTICE (5)  /**< Normal but significant condition. */
#define APP_LOG_LEVEL_INFO (6)    /**< Informational messages. */
#define APP_LOG_LEVEL_DEBUG (7)   /**< Debug-level messages. */
/** @} */

#define APP_LOG_LEVEL_MIN (APP_LOG_LEVEL_EMERG)
#define APP_LOG_LEVEL_DEFAULT (APP_LOG_LEVEL_ERR)
#define APP_LOG_LEVEL_MAX (APP_LOG_LEVEL_DEBUG)

/**
 * @defgroup APP_LOG_STATUSES Log Status Codes
 * @brief Defines the status codes returned by logging functions.
 * @{
 */
#define APP_LOG_STATUS_OK (0)              /**< Operation successful. */
#define APP_LOG_STATUS_FAIL (-1)           /**< Operation failed. */
#define APP_LOG_STATUS_INVALID_LEVEL (-2)  /**< Invalid log level. */
#define APP_LOG_STATUS_LEVEL_DISABLED (-3) /**< Log level disabled. */
/** @} */

#define APP_LOG_INIT applog_init
#define APP_LOG_DEINIT applog_deinit
#define APP_LOG_SET_LEVEL(level) applog_set_config_level(level)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"

#ifdef APP_LOG_NO_FUNC_LINE
#define APP_LOG_DEBUG(...) applog_write(APP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define APP_LOG_INFO(...) applog_write(APP_LOG_LEVEL_INFO, __VA_ARGS__)
#define APP_LOG_ERR(...) applog_write(APP_LOG_LEVEL_ERR, __VA_ARGS__)
#define APP_LOG_WARNING(...) applog_write(APP_LOG_LEVEL_WARNING, __VA_ARGS__)
#define APP_LOG_EMERG(...) applog_write(APP_LOG_LEVEL_EMERG, __VA_ARGS__)
#define APP_LOG_CRITICAL(...) applog_write(APP_LOG_LEVEL_CRIT, __VA_ARGS__)
#define APP_LOG_NOTICE(...) applog_write(APP_LOG_LEVEL_NOTICE, __VA_ARGS__)
#define APP_LOG_ALERT(...) applog_write(APP_LOG_LEVEL_ALERT, __VA_ARGS__)
#else
#define APP_LOG_DEBUG(MSG, ...) applog_write(APP_LOG_LEVEL_DEBUG, "%s:%u:" MSG " ", __func__, __LINE__, ##__VA_ARGS__)
#define APP_LOG_INFO(MSG, ...) applog_write(APP_LOG_LEVEL_INFO, "%s:%u:" MSG " ", __func__, __LINE__, ##__VA_ARGS__)
#define APP_LOG_ERR(MSG, ...) applog_write(APP_LOG_LEVEL_ERR, "%s:%u:" MSG " ", __func__, __LINE__, ##__VA_ARGS__)
#define APP_LOG_WARNING(MSG, ...) applog_write(APP_LOG_LEVEL_WARNING, "%s:%u:" MSG " ", __func__, __LINE__, ##__VA_ARGS__)
#define APP_LOG_EMERG(MSG, ...) applog_write(APP_LOG_LEVEL_EMERG, "%s:%u:" MSG " ", __func__, __LINE__, ##__VA_ARGS__)
#define APP_LOG_CRITICAL(MSG, ...) applog_write(APP_LOG_LEVEL_CRIT, "%s:%u:" MSG " ", __func__, __LINE__, ##__VA_ARGS__)
#define APP_LOG_NOTICE(MSG, ...) applog_write(APP_LOG_LEVEL_NOTICE, "%s:%u:" MSG " ", __func__, __LINE__, ##__VA_ARGS__)
#define APP_LOG_ALERT(MSG, ...) applog_write(APP_LOG_LEVEL_ALERT, "%s:%u:" MSG " ", __func__, __LINE__, ##__VA_ARGS__)
#endif

/* Use below one for Syslog */
#define APP_SYS_LOG_INFO(...) applog_write(APP_LOG_LEVEL_INFO, __VA_ARGS__)

#pragma GCC diagnostic pop

#endif /*_APPLOG_H_*/
