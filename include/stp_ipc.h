/**
 * @file stp_ipc.h
 * @brief Заголовочный файл для межпроцесного взаимодействия
 *
 *
 * @details
 * Содержит основные рабочие структуры и определения для ipc
 */

#ifndef __STP_IPC_H__
#define __STP_IPC_H__

#define STPD_SOCK_NAME "/var/run/stpipc.sock"

/**
 * @enum L2_PROTO_MODE
 * @brief Определяет режимы работы протоколов уровня 2 (L2).
 *
 * Перечисление используется для указания активного протокола уровня 2 в системе.
 * Это может быть протокол PVSTP или отсутствие протокола.
 *
 * @var L2_PROTO_MODE::L2_NONE
 * Указывает, что протокол уровня 2 не используется.
 *
 * @var L2_PROTO_MODE::L2_PVSTP
 * Указывает, что активен протокол Per VLAN Spanning Tree Protocol (PVSTP).
 */
typedef enum L2_PROTO_MODE
{
    L2_NONE,  /**< Протокол уровня 2 отсутствует. */
    L2_PVSTP, /**< Активен протокол Per VLAN Spanning Tree Protocol (PVSTP). */
} L2_PROTO_MODE;

/**
 * @enum STP_MSG_TYPE
 * @brief Определяет типы сообщений, используемых в протоколе STP.
 *
 * Перечисление описывает типы сообщений, которые могут быть отправлены или
 * обработаны в контексте Spanning Tree Protocol (STP). Используется для
 * идентификации и обработки различных операций и конфигураций.
 *
 * @var STP_MSG_TYPE::STP_INVALID_MSG
 * Неверное или нераспознанное сообщение.
 *
 * @var STP_MSG_TYPE::STP_INIT_READY
 * Сообщение о готовности к инициализации STP.
 *
 * @var STP_MSG_TYPE::STP_BRIDGE_CONFIG
 * Сообщение о конфигурации моста.
 *
 * @var STP_MSG_TYPE::STP_VLAN_CONFIG
 * Сообщение о конфигурации VLAN.
 *
 * @var STP_MSG_TYPE::STP_VLAN_PORT_CONFIG
 * Сообщение о конфигурации порта VLAN.
 *
 * @var STP_MSG_TYPE::STP_PORT_CONFIG
 * Сообщение о конфигурации порта.
 *
 * @var STP_MSG_TYPE::STP_VLAN_MEM_CONFIG
 * Сообщение о конфигурации членов VLAN.
 *
 * @var STP_MSG_TYPE::STP_STPCTL_MSG
 * Сообщение, отправляемое через STPCTL для управления STP.
 *
 * @var STP_MSG_TYPE::STP_MAX_MSG
 * Максимальное значение для сообщений STP (служит для проверки границ).
 */
typedef enum STP_MSG_TYPE
{
    STP_INVALID_MSG,      /**< Неверное или нераспознанное сообщение. */
    STP_INIT_READY,       /**< Сообщение о готовности к инициализации STP. */
    STP_BRIDGE_CONFIG,    /**< Сообщение о конфигурации моста. */
    STP_VLAN_CONFIG,      /**< Сообщение о конфигурации VLAN. */
    STP_VLAN_PORT_CONFIG, /**< Сообщение о конфигурации порта VLAN. */
    STP_PORT_CONFIG,      /**< Сообщение о конфигурации порта. */
    STP_VLAN_MEM_CONFIG,  /**< Сообщение о конфигурации членов VLAN. */
    STP_STPCTL_MSG,       /**< Сообщение через STPCTL для управления STP. */
    STP_MAX_MSG           /**< Максимальное значение для сообщений STP. */
} STP_MSG_TYPE;

/**
 * @enum STP_CTL_TYPE
 * @brief Типы управляющих команд для STPCTL.
 *
 * Перечисление определяет команды, которые могут быть отправлены через интерфейс STPCTL
 * для управления и диагностики протокола Spanning Tree Protocol (STP).
 *
 * @var STP_CTL_TYPE::STP_CTL_HELP
 * Выводит справочную информацию о доступных командах.
 *
 * @var STP_CTL_TYPE::STP_CTL_DUMP_ALL
 * Выводит информацию обо всех состояниях STP.
 *
 * @var STP_CTL_TYPE::STP_CTL_DUMP_GLOBAL
 * Выводит глобальную информацию STP.
 *
 * @var STP_CTL_TYPE::STP_CTL_DUMP_VLAN_ALL
 * Выводит информацию о всех VLAN в контексте STP.
 *
 * @var STP_CTL_TYPE::STP_CTL_DUMP_VLAN
 * Выводит информацию об определенном VLAN.
 *
 * @var STP_CTL_TYPE::STP_CTL_DUMP_INTF
 * Выводит информацию об интерфейсе.
 *
 * @var STP_CTL_TYPE::STP_CTL_SET_LOG_LVL
 * Устанавливает уровень логирования.
 *
 * @var STP_CTL_TYPE::STP_CTL_DUMP_NL_DB
 * Выводит информацию из базы данных Netlink.
 *
 * @var STP_CTL_TYPE::STP_CTL_DUMP_NL_DB_INTF
 * Выводит информацию об интерфейсах из базы данных Netlink.
 *
 * @var STP_CTL_TYPE::STP_CTL_DUMP_LIBEV_STATS
 * Выводит статистику событий libevent.
 *
 * @var STP_CTL_TYPE::STP_CTL_SET_DBG
 * Включает или выключает режим отладки.
 *
 * @var STP_CTL_TYPE::STP_CTL_CLEAR_ALL
 * Очищает всю статистику STP.
 *
 * @var STP_CTL_TYPE::STP_CTL_CLEAR_VLAN
 * Очищает статистику для определенного VLAN.
 *
 * @var STP_CTL_TYPE::STP_CTL_CLEAR_INTF
 * Очищает статистику для интерфейса.
 *
 * @var STP_CTL_TYPE::STP_CTL_CLEAR_VLAN_INTF
 * Очищает статистику для интерфейса в контексте VLAN.
 *
 * @var STP_CTL_TYPE::STP_CTL_MAX
 * Максимальное значение для проверок диапазона значений.
 */
typedef enum STP_CTL_TYPE
{
    STP_CTL_HELP,             /**< Вывод справочной информации. */
    STP_CTL_DUMP_ALL,         /**< Вывод полной информации STP. */
    STP_CTL_DUMP_GLOBAL,      /**< Вывод глобальной информации STP. */
    STP_CTL_DUMP_VLAN_ALL,    /**< Вывод информации о всех VLAN. */
    STP_CTL_DUMP_VLAN,        /**< Вывод информации о конкретном VLAN. */
    STP_CTL_DUMP_INTF,        /**< Вывод информации об интерфейсе. */
    STP_CTL_SET_LOG_LVL,      /**< Установка уровня логирования. */
    STP_CTL_DUMP_NL_DB,       /**< Вывод базы данных Netlink. */
    STP_CTL_DUMP_NL_DB_INTF,  /**< Вывод информации об интерфейсах Netlink. */
    STP_CTL_DUMP_LIBEV_STATS, /**< Вывод статистики libevent. */
    STP_CTL_SET_DBG,          /**< Включение/выключение отладки. */
    STP_CTL_CLEAR_ALL,        /**< Очистка всей статистики STP. */
    STP_CTL_CLEAR_VLAN,       /**< Очистка статистики для VLAN. */
    STP_CTL_CLEAR_INTF,       /**< Очистка статистики для интерфейса. */
    STP_CTL_CLEAR_VLAN_INTF,  /**< Очистка статистики интерфейса в контексте VLAN. */
    STP_CTL_MAX               /**< Максимальное значение для проверок диапазона. */
} STP_CTL_TYPE;

/**
 * @struct STP_IPC_MSG
 * @brief Представляет сообщение, отправляемое через IPC для управления STP.
 *
 * Эта структура используется для передачи сообщений между различными компонентами
 * системы через IPC (Inter-Process Communication). Сообщение включает тип,
 * длину данных и сами данные.
 *
 * @var STP_IPC_MSG::msg_type
 * Тип сообщения, определяющий, какая операция или действие требуется.
 *
 * @var STP_IPC_MSG::msg_len
 * Длина полезной нагрузки (данных), передаваемой в сообщении.
 *
 * @var STP_IPC_MSG::data
 * Поле данных переменной длины, содержащее полезную нагрузку сообщения.
 * Размер определяется параметром `msg_len`.
 */
typedef struct STP_IPC_MSG
{
    int msg_type;         /**< Тип сообщения. */
    unsigned int msg_len; /**< Длина полезной нагрузки (в байтах). */
    char data[0];         /**< Поле данных переменной длины. */
} STP_IPC_MSG;

#define STP_SET_COMMAND 1
#define STP_DEL_COMMAND 0

/**
 * @struct STP_INIT_READY_MSG
 * @brief Сообщение для инициализации готовности STP.
 *
 * Эта структура используется для передачи информации о готовности STP к работе.
 * Она содержит информацию об операции (включение/отключение) и максимальном
 * количестве экземпляров STP, поддерживаемых системой.
 *
 * @var STP_INIT_READY_MSG::opcode
 * Операция, связанная с инициализацией. Возможные значения:
 * - `1` — Включить.
 * - `0` — Отключить.
 *
 * @var STP_INIT_READY_MSG::max_stp_instances
 * Максимальное количество поддерживаемых экземпляров STP.
 */
typedef struct STP_INIT_READY_MSG
{
    uint8_t opcode; /**< Код операции: включение/отключение. */ // enable/disable
    uint16_t max_stp_instances;                                 /**< Максимальное количество STP экземпляров. */
} __attribute__((packed)) STP_INIT_READY_MSG;

/**
 * @struct STP_BRIDGE_CONFIG_MSG
 * @brief Сообщение для настройки параметров моста STP.
 *
 * Эта структура используется для передачи параметров конфигурации моста в контексте
 * Spanning Tree Protocol (STP). Содержит информацию об операции, режиме STP, времени
 * ожидания защиты корня и базовом MAC-адресе.
 *
 * @var STP_BRIDGE_CONFIG_MSG::opcode
 * Операция для моста. Возможные значения:
 * - `1` — Включить STP.
 * - `0` — Отключить STP.
 *
 * @var STP_BRIDGE_CONFIG_MSG::stp_mode
 * Режим работы STP. Возможные значения:
 * - `0` — STP.
 * - `1` — RSTP.
 * - `2` — MSTP.
 *
 * @var STP_BRIDGE_CONFIG_MSG::rootguard_timeout
 * Тайм-аут (в секундах) для защиты корневого моста (Root Guard). Если тайм-аут установлен
 * в 0, защита корня отключена.
 *
 * @var STP_BRIDGE_CONFIG_MSG::base_mac_addr
 * Базовый MAC-адрес моста. Используется для идентификации моста в сети.
 */
typedef struct STP_BRIDGE_CONFIG_MSG
{
    uint8_t opcode; /**< Операция: включение/отключение STP. */ // enable/disable
    uint8_t stp_mode;                                           /**< Режим работы STP. */
    int rootguard_timeout;                                      /**< Тайм-аут защиты корневого моста (в секундах). */
    uint8_t base_mac_addr[6];                                   /**< Базовый MAC-адрес моста. */
} __attribute__((packed)) STP_BRIDGE_CONFIG_MSG;

/**
 * @struct PORT_ATTR
 * @brief Атрибуты порта для конфигурации и управления.
 *
 * Эта структура используется для описания основных характеристик порта,
 * таких как имя интерфейса, режим работы и статус активности.
 *
 * @var PORT_ATTR::intf_name
 * Имя интерфейса, связанного с портом. Соответствует системному обозначению
 * (например, "Ethernet50").
 *
 * @var PORT_ATTR::mode
 * Режим работы порта. Возможные значения:
 * - `0` — Неопределённый режим.
 * - `1` — Режим доступа.
 * - `2` — Транк.
 * - Другие значения могут использоваться в зависимости от контекста.
 *
 * @var PORT_ATTR::enabled
 * Статус активности порта:
 * - `1` — Порт включён.
 * - `0` — Порт выключен.
 */
typedef struct PORT_ATTR
{
    char intf_name[IFNAMSIZ]; /**< Имя интерфейса. */
    int8_t mode;              /**< Режим работы порта. */
    uint8_t enabled;          /**< Статус активности порта. */
} PORT_ATTR;

/**
 * @struct STP_VLAN_CONFIG_MSG
 * @brief Сообщение для конфигурации VLAN в контексте STP.
 *
 * Эта структура используется для передачи параметров конфигурации VLAN в рамках
 * протокола Spanning Tree Protocol (STP). Она включает информацию о VLAN, параметры
 * STP, а также список портов, относящихся к VLAN.
 *
 * @var STP_VLAN_CONFIG_MSG::opcode
 * Операция конфигурации VLAN:
 * - `1` — Включить VLAN.
 * - `0` — Отключить VLAN.
 *
 * @var STP_VLAN_CONFIG_MSG::newInstance
 * Указывает, является ли это новый экземпляр VLAN:
 * - `1` — Новый экземпляр.
 * - `0` — Существующий экземпляр.
 *
 * @var STP_VLAN_CONFIG_MSG::vlan_id
 * Идентификатор VLAN (например, 1–4094).
 *
 * @var STP_VLAN_CONFIG_MSG::inst_id
 * Идентификатор экземпляра STP для данного VLAN.
 *
 * @var STP_VLAN_CONFIG_MSG::forward_delay
 * Задержка пересылки (в секундах) для VLAN.
 *
 * @var STP_VLAN_CONFIG_MSG::hello_time
 * Время между Hello BPDU (в секундах).
 *
 * @var STP_VLAN_CONFIG_MSG::max_age
 * Максимальный возраст сообщений (в секундах).
 *
 * @var STP_VLAN_CONFIG_MSG::priority
 * Приоритет моста для данного VLAN.
 *
 * @var STP_VLAN_CONFIG_MSG::count
 * Количество портов, включённых в список `port_list`.
 *
 * @var STP_VLAN_CONFIG_MSG::port_list
 * Список портов, связанных с VLAN. Это поле имеет переменную длину, и его размер
 * определяется значением `count`.
 */
typedef struct STP_VLAN_CONFIG_MSG
{
    uint8_t opcode; /**< Операция: включение/отключение VLAN. */ // enable/disable
    uint8_t newInstance;                                         /**< Новый экземпляр VLAN или существующий. */
    int vlan_id;                                                 /**< Идентификатор VLAN. */
    int inst_id;                                                 /**< Идентификатор экземпляра STP. */
    int forward_delay;                                           /**< Задержка пересылки в секундах. */
    int hello_time;                                              /**< Интервал Hello в секундах. */
    int max_age;                                                 /**< Максимальный возраст сообщений в секундах. */
    int priority;                                                /**< Приоритет моста для VLAN. */
    int count;                                                   /**< Количество портов в списке. */
    PORT_ATTR port_list[0];                                      /**< Список портов переменной длины. */
} __attribute__((packed)) STP_VLAN_CONFIG_MSG;

/**
 * @struct STP_VLAN_PORT_CONFIG_MSG
 * @brief Сообщение для конфигурации порта VLAN в контексте STP.
 *
 * Эта структура используется для передачи параметров конфигурации порта VLAN
 * в рамках протокола Spanning Tree Protocol (STP). Содержит информацию о VLAN,
 * порте и его параметрах, таких как стоимость пути и приоритет.
 *
 * @var STP_VLAN_PORT_CONFIG_MSG::opcode
 * Операция конфигурации порта:
 * - `1` — Включить порт.
 * - `0` — Отключить порт.
 *
 * @var STP_VLAN_PORT_CONFIG_MSG::vlan_id
 * Идентификатор VLAN, к которому относится порт.
 *
 * @var STP_VLAN_PORT_CONFIG_MSG::intf_name
 * Имя интерфейса, связанного с портом (например, "Ethernet50").
 *
 * @var STP_VLAN_PORT_CONFIG_MSG::inst_id
 * Идентификатор экземпляра STP для данного порта.
 *
 * @var STP_VLAN_PORT_CONFIG_MSG::path_cost
 * Стоимость пути для данного порта. Используется в алгоритмах STP.
 *
 * @var STP_VLAN_PORT_CONFIG_MSG::priority
 * Приоритет порта в рамках STP. Более низкое значение означает более высокий приоритет.
 */
typedef struct STP_VLAN_PORT_CONFIG_MSG
{
    uint8_t opcode; /**< Операция: включение/отключение порта. */ // enable/disable
    int vlan_id;                                                  /**< Идентификатор VLAN. */
    char intf_name[IFNAMSIZ];                                     /**< Имя интерфейса порта. */
    int inst_id;                                                  /**< Идентификатор экземпляра STP. */
    int path_cost;                                                /**< Стоимость пути для порта. */
    int priority;                                                 /**< Приоритет порта. */
} __attribute__((packed)) STP_VLAN_PORT_CONFIG_MSG;

/**
 * @struct VLAN_ATTR
 * @brief Атрибуты VLAN для конфигурации и управления.
 *
 * Эта структура используется для описания базовых атрибутов VLAN, таких как
 * идентификатор экземпляра STP, VLAN ID и режим работы.
 *
 * @var VLAN_ATTR::inst_id
 * Идентификатор экземпляра STP, связанного с VLAN.
 *
 * @var VLAN_ATTR::vlan_id
 * Идентификатор VLAN (например, 1–4094).
 *
 * @var VLAN_ATTR::mode
 * Режим работы VLAN. Возможные значения:
 * - `0` — Неопределённый режим.
 * - `1` — VLAN работает в режиме доступа.
 * - `2` — VLAN работает в режиме транка.
 * - Другие значения могут использоваться в зависимости от контекста.
 */
typedef struct VLAN_ATTR
{
    int inst_id; /**< Идентификатор экземпляра STP. */
    int vlan_id; /**< Идентификатор VLAN. */
    int8_t mode; /**< Режим работы VLAN. */
} VLAN_ATTR;

/**
 * @struct STP_PORT_CONFIG_MSG
 * @brief Сообщение для конфигурации порта в контексте STP.
 *
 * Эта структура используется для передачи параметров конфигурации порта STP.
 * Включает информацию об имени интерфейса, различных механизмах защиты, параметрах
 * STP (стоимость пути, приоритет) и списке VLAN, ассоциированных с портом.
 *
 * @var STP_PORT_CONFIG_MSG::opcode
 * Операция для порта:
 * - `1` — Включить порт.
 * - `0` — Отключить порт.
 *
 * @var STP_PORT_CONFIG_MSG::intf_name
 * Имя интерфейса, связанного с портом (например, "Ethernet50").
 *
 * @var STP_PORT_CONFIG_MSG::enabled
 * Статус порта:
 * - `1` — Порт включён.
 * - `0` — Порт выключен.
 *
 * @var STP_PORT_CONFIG_MSG::root_guard
 * Статус функции Root Guard:
 * - `1` — Включена.
 * - `0` — Отключена.
 *
 * @var STP_PORT_CONFIG_MSG::bpdu_guard
 * Статус функции BPDU Guard:
 * - `1` — Включена.
 * - `0` — Отключена.
 *
 * @var STP_PORT_CONFIG_MSG::bpdu_guard_do_disable
 * Указывает, должен ли порт быть отключён при срабатывании BPDU Guard:
 * - `1` — Отключить порт.
 * - `0` — Не отключать порт.
 *
 * @var STP_PORT_CONFIG_MSG::portfast
 * Статус функции PortFast:
 * - `1` — Включена.
 * - `0` — Отключена.
 *
 * @var STP_PORT_CONFIG_MSG::uplink_fast
 * Статус функции UplinkFast:
 * - `1` — Включена.
 * - `0` — Отключена.
 *
 * @var STP_PORT_CONFIG_MSG::path_cost
 * Стоимость пути для данного порта. Используется в алгоритмах STP.
 *
 * @var STP_PORT_CONFIG_MSG::priority
 * Приоритет порта в рамках STP. Более низкое значение означает более высокий приоритет.
 *
 * @var STP_PORT_CONFIG_MSG::count
 * Количество VLAN, связанных с портом.
 *
 * @var STP_PORT_CONFIG_MSG::vlan_list
 * Список VLAN, связанных с портом. Поле имеет переменную длину, и размер определяется
 * значением `count`.
 */
typedef struct STP_PORT_CONFIG_MSG
{
    uint8_t opcode; /**< Операция: включение/отключение порта. */ // enable/disable
    char intf_name[IFNAMSIZ];                                     /**< Имя интерфейса порта. */
    uint8_t enabled;                                              /**< Статус порта (включён/выключен). */
    uint8_t root_guard;                                           /**< Функция Root Guard (включена/выключена). */
    uint8_t bpdu_guard;                                           /**< Функция BPDU Guard (включена/выключена). */
    uint8_t bpdu_guard_do_disable;                                /**< Отключение порта при срабатывании BPDU Guard. */
    uint8_t portfast;                                             /**< Функция PortFast (включена/выключена). */
    uint8_t uplink_fast;                                          /**< Функция UplinkFast (включена/выключена). */
    int path_cost;                                                /**< Стоимость пути для порта. */
    int priority;                                                 /**< Приоритет порта. */
    int count;                                                    /**< Количество VLAN, связанных с портом. */
    VLAN_ATTR vlan_list[0];                                       /**< Список VLAN переменной длины. */
} __attribute__((packed)) STP_PORT_CONFIG_MSG;

/**
 * @struct STP_VLAN_MEM_CONFIG_MSG
 * @brief Сообщение для конфигурации члена VLAN в контексте STP.
 *
 * Эта структура используется для настройки параметров интерфейса, принадлежащего
 * VLAN, в рамках протокола Spanning Tree Protocol (STP). Содержит информацию о VLAN,
 * параметрах STP и интерфейсе.
 *
 * @var STP_VLAN_MEM_CONFIG_MSG::opcode
 * Операция для члена VLAN:
 * - `1` — Включить.
 * - `0` — Отключить.
 *
 * @var STP_VLAN_MEM_CONFIG_MSG::vlan_id
 * Идентификатор VLAN, к которому принадлежит интерфейс.
 *
 * @var STP_VLAN_MEM_CONFIG_MSG::inst_id
 * Идентификатор экземпляра STP для данного члена VLAN.
 *
 * @var STP_VLAN_MEM_CONFIG_MSG::intf_name
 * Имя интерфейса, связанного с членом VLAN (например, "Ethernet50").
 *
 * @var STP_VLAN_MEM_CONFIG_MSG::enabled
 * Статус интерфейса:
 * - `1` — Включён.
 * - `0` — Выключен.
 *
 * @var STP_VLAN_MEM_CONFIG_MSG::mode
 * Режим работы интерфейса:
 * - `0` — Неопределённый режим.
 * - `1` — Режим доступа.
 * - `2` — Режим транка.
 *
 * @var STP_VLAN_MEM_CONFIG_MSG::path_cost
 * Стоимость пути для данного интерфейса. Используется в алгоритмах STP.
 *
 * @var STP_VLAN_MEM_CONFIG_MSG::priority
 * Приоритет интерфейса в рамках STP. Более низкое значение означает более высокий приоритет.
 */
typedef struct STP_VLAN_MEM_CONFIG_MSG
{
    uint8_t opcode; /**< Операция: включение/отключение члена VLAN. */ // enable/disable
    int vlan_id;                                                       /**< Идентификатор VLAN. */
    int inst_id;                                                       /**< Идентификатор экземпляра STP. */
    char intf_name[IFNAMSIZ];                                          /**< Имя интерфейса члена VLAN. */
    uint8_t enabled;                                                   /**< Статус интерфейса (включён/выключен). */
    int8_t mode;                                                       /**< Режим работы интерфейса. */
    int path_cost;                                                     /**< Стоимость пути для интерфейса. */
    int priority;                                                      /**< Приоритет интерфейса. */
} __attribute__((packed)) STP_VLAN_MEM_CONFIG_MSG;

/**
 * @struct STP_DEBUG_OPT
 * @brief Опции отладки для STP (Spanning Tree Protocol).
 *
 * Эта структура используется для управления параметрами отладки протокола STP.
 * Параметры включают в себя флаги управления уровнями отладки и включения/отключения
 * различных типов сообщений.
 *
 * @var STP_DEBUG_OPT::flags
 * Флаги, определяющие включённые параметры отладки:
 * - `STPCTL_DBG_SET_ENABLED (0x0001)` — Включить отладку.
 * - `STPCTL_DBG_SET_VERBOSE (0x0002)` — Включить подробные сообщения.
 * - `STPCTL_DBG_SET_BPDU_RX (0x0004)` — Включить отладку приёмных BPDU.
 * - `STPCTL_DBG_SET_BPDU_TX (0x0008)` — Включить отладку отправляемых BPDU.
 * - `STPCTL_DBG_SET_EVENT (0x0010)` — Включить отладку событий.
 * - `STPCTL_DBG_SET_PORT (0x0020)` — Включить отладку портов.
 * - `STPCTL_DBG_SET_VLAN (0x0040)` — Включить отладку VLAN.
 * - `STPCTL_DBG_SHOW (0x0080)` — Показать текущие параметры отладки.
 *
 * @var STP_DEBUG_OPT::enabled
 * Флаг включения отладки:
 * - `1` — Отладка включена.
 * - `0` — Отладка выключена.
 *
 * @var STP_DEBUG_OPT::verbose
 * Флаг включения подробных сообщений:
 * - `1` — Подробные сообщения включены.
 * - `0` — Подробные сообщения выключены.
 *
 * @var STP_DEBUG_OPT::bpdu_rx
 * Флаг включения отладки приёмных BPDU:
 * - `1` — Включено.
 * - `0` — Выключено.
 *
 * @var STP_DEBUG_OPT::bpdu_tx
 * Флаг включения отладки отправляемых BPDU:
 * - `1` — Включено.
 * - `0` — Выключено.
 *
 * @var STP_DEBUG_OPT::event
 * Флаг включения отладки событий:
 * - `1` — Включено.
 * - `0` — Выключено.
 *
 * @var STP_DEBUG_OPT::port
 * Флаг включения отладки портов:
 * - `1` — Включено.
 * - `0` — Выключено.
 *
 * @var STP_DEBUG_OPT::vlan
 * Флаг включения отладки VLAN:
 * - `1` — Включено.
 * - `0` — Выключено.
 *
 * @var STP_DEBUG_OPT::spare
 * Зарезервированное поле для выравнивания:
 * - `1` — Не используется.
 * - `0` — Не используется.
 */
typedef struct STP_DEBUG_OPT
{
#define STPCTL_DBG_SET_ENABLED 0x0001
#define STPCTL_DBG_SET_VERBOSE 0x0002
#define STPCTL_DBG_SET_BPDU_RX 0x0004
#define STPCTL_DBG_SET_BPDU_TX 0x0008
#define STPCTL_DBG_SET_EVENT 0x0010
#define STPCTL_DBG_SET_PORT 0x0020
#define STPCTL_DBG_SET_VLAN 0x0040
#define STPCTL_DBG_SHOW 0x0080
    uint16_t flags;      /**< Флаги, управляющие параметрами отладки. */
    uint8_t enabled : 1; /**< Включение отладки. */
    uint8_t verbose : 1; /**< Подробные сообщения. */
    uint8_t bpdu_rx : 1; /**< Отладка приёмных BPDU. */
    uint8_t bpdu_tx : 1; /**< Отладка отправляемых BPDU. */
    uint8_t event : 1;   /**< Отладка событий. */
    uint8_t port : 1;    /**< Отладка портов. */
    uint8_t vlan : 1;    /**< Отладка VLAN. */
    uint8_t spare : 1;   /**< Зарезервированное поле для выравнивания. */
} STP_DEBUG_OPT;

/**
 * @struct STP_CTL_MSG
 * @brief Сообщение управления для протокола STP.
 *
 * Эта структура используется для передачи управляющих команд и параметров в контексте
 * Spanning Tree Protocol (STP). Включает информацию о типе команды, идентификаторе VLAN,
 * имени интерфейса, уровне и параметрах отладки.
 *
 * @var STP_CTL_MSG::cmd_type
 * Тип команды, определяющий выполняемую операцию.
 *
 * @var STP_CTL_MSG::vlan_id
 * Идентификатор VLAN, к которому относится команда. Если не применяется, значение может быть
 * задано как -1.
 *
 * @var STP_CTL_MSG::intf_name
 * Имя интерфейса, связанного с командой (например, "Ethernet50"). Если не используется, поле может быть пустым.
 *
 * @var STP_CTL_MSG::level
 * Уровень для команды (например, уровень отладки).
 *
 * @var STP_CTL_MSG::dbg
 * Параметры отладки. Использует структуру `STP_DEBUG_OPT`, которая включает настройки для различных
 * режимов отладки.
 */
typedef struct STP_CTL_MSG
{
    int cmd_type;             /**< Тип команды. */
    int vlan_id;              /**< Идентификатор VLAN. */
    char intf_name[IFNAMSIZ]; /**< Имя интерфейса. */
    int level;                /**< Уровень команды. */
    STP_DEBUG_OPT dbg;        /**< Параметры отладки. */
} __attribute__((packed)) STP_CTL_MSG;

#endif
