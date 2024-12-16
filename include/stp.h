/**
 * @file stp.h
 * @brief Заголовочный файл для реализации протокола STP (Spanning Tree Protocol).
 *
 * Содержит определения структур, констант, перечислений и макросов,
 * используемых для управления STP на уровне VLAN и портов.
 *
 * @copyright 2019 Broadcom.
 * @license Apache License, Version 2.0.
 */

#ifndef __STP_H__
#define __STP_H__

/* #defines ----------------------------------------------------------------- */
#define PORT_MASK BITMAP_T
#define VLAN_MASK BITMAP_T

/**
 * @def STP_VERSION_ID
 * @brief Версия протокола STP.
 */
#define STP_VERSION_ID 0

#define STP_MESSAGE_AGE_INCREMENT 1
#define STP_INVALID_PORT 0xFFF

#define STP_OK 0
#define STP_ERR -1

/**
 * @def STP_DFLT_PRIORITY
 * @brief Значение приоритета моста по умолчанию.
 */
#define STP_DFLT_PRIORITY 32768

/**
 * @def STP_MIN_PRIORITY
 * @brief Минимально допустимое значение приоритета.
 */
#define STP_MIN_PRIORITY 0
/**
 * @def STP_MAX_PRIORITY
 * @brief Максимально допустимое значение приоритета.
 */
#define STP_MAX_PRIORITY 65535

#define STP_DFLT_FORWARD_DELAY 15
#define STP_MIN_FORWARD_DELAY 4
#define STP_MAX_FORWARD_DELAY 30

#define STP_DFLT_MAX_AGE 20
#define STP_MIN_MAX_AGE 6
#define STP_MAX_MAX_AGE 40

#define STP_DFLT_HELLO_TIME 2
#define STP_MIN_HELLO_TIME 1
#define STP_MAX_HELLO_TIME 10

#define STP_DFLT_HOLD_TIME 1

#define STP_DFLT_ROOT_PROTECT_TIMEOUT 30
#define STP_MIN_ROOT_PROTECT_TIMEOUT 5
#define STP_MAX_ROOT_PROTECT_TIMEOUT 600

#define STP_DFLT_PORT_PRIORITY 128
#define STP_MIN_PORT_PRIORITY 0
#define STP_MAX_PORT_PRIORITY 240

#define STP_FASTSPAN_FORWARD_DELAY 2
#define STP_FASTUPLINK_FORWARD_DELAY 1

// 802.1d path costs - for backward compatibility with BI

#define STP_LEGACY_MIN_PORT_PATH_COST 1
#define STP_LEGACY_MAX_PORT_PATH_COST 65535
#define STP_LEGACY_PORT_PATH_COST_10M 100
#define STP_LEGACY_PORT_PATH_COST_100M 19
#define STP_LEGACY_PORT_PATH_COST_1G 4
#define STP_LEGACY_PORT_PATH_COST_10G 2
#define STP_LEGACY_PORT_PATH_COST_25G 1
#define STP_LEGACY_PORT_PATH_COST_40G 1
#define STP_LEGACY_PORT_PATH_COST_100G 1
#define STP_LEGACY_PORT_PATH_COST_400G 1

// 802.1t path costs - calculated as 20,000,000,000 / LinkSpeedInKbps
/**
 * @def STP_MIN_PORT_PATH_COST
 * @brief Минимальная стоимость пути для порта.
 */
#define STP_MIN_PORT_PATH_COST 1

/**
 * @def STP_MAX_PORT_PATH_COST
 * @brief Максимальная стоимость пути для порта.
 */
#define STP_MAX_PORT_PATH_COST 200000000
#define STP_PORT_PATH_COST_1M 20000000
#define STP_PORT_PATH_COST_10M 2000000
#define STP_PORT_PATH_COST_100M 200000
#define STP_PORT_PATH_COST_1G 20000
#define STP_PORT_PATH_COST_10G 2000
#define STP_PORT_PATH_COST_25G 800
#define STP_PORT_PATH_COST_40G 500
#define STP_PORT_PATH_COST_100G 200
#define STP_PORT_PATH_COST_400G 50
#define STP_PORT_PATH_COST_1T 20
#define STP_PORT_PATH_COST_10T 2

#define STP_SIZEOF_CONFIG_BPDU 35
#define STP_SIZEOF_TCN_BPDU 4
#define STP_BULK_MESG_LENGTH 350

#define g_stp_instances stp_global.max_instances
#define g_stp_active_instances stp_global.active_instances
#define g_stp_class_array stp_global.class_array
#define g_stp_port_array stp_global.port_array
#define g_stp_tick_id stp_global.tick_id
#define g_stp_bpdu_sync_tick_id stp_global.bpdu_sync_tick_id

#define g_stp_config_bpdu stp_global.config_bpdu
#define g_stp_tcn_bpdu stp_global.tcn_bpdu
#define g_stp_pvst_config_bpdu stp_global.pvst_config_bpdu
#define g_stp_pvst_tcn_bpdu stp_global.pvst_tcn_bpdu

#define g_fastspan_mask stp_global.fastspan_mask
#define g_fastspan_config_mask stp_global.fastspan_admin_mask
#define g_fastuplink_mask stp_global.fastuplink_admin_mask

#define g_stp_protect_mask stp_global.protect_mask
#define g_stp_protect_do_disable_mask stp_global.protect_do_disable_mask
#define g_stp_root_protect_mask stp_global.root_protect_mask
#define g_stp_protect_disabled_mask stp_global.protect_disabled_mask
#define g_stp_enable_mask stp_global.enable_mask
#define g_stp_enable_config_mask stp_global.enable_admin_mask

#define g_sstp_enabled stp_global.sstp_enabled

#define STP_GET_MIN_PORT_PATH_COST ((UINT32)(g_stpd_extend_mode ? STP_MIN_PORT_PATH_COST : STP_LEGACY_MIN_PORT_PATH_COST))
#define STP_GET_MAX_PORT_PATH_COST ((UINT32)(g_stpd_extend_mode ? STP_MAX_PORT_PATH_COST : STP_LEGACY_MAX_PORT_PATH_COST))

#define GET_STP_INDEX(stp_class) ((stp_class) - g_stp_class_array)
#define GET_STP_CLASS(stp_index) (g_stp_class_array + (stp_index))
#define GET_STP_PORT_CLASS(class, port) stpdata_get_port_class(class, port)
#define GET_STP_PORT_IFNAME(port) stp_intf_get_port_name(port->port_id.number)

#define STP_TICKS_TO_SECONDS(x) ((x) >> 1)
#define STP_SECONDS_TO_TICKS(x) ((x) << 1)

#define STP_IS_FASTSPAN_ENABLED(port) is_member(g_fastspan_mask, (port))
#define STP_IS_ENABLED(port) is_member(g_stp_enable_mask, (port))

#define STP_IS_FASTUPLINK_CONFIGURED(port) is_member(g_fastuplink_mask, (port))
#define STP_IS_FASTSPAN_CONFIGURED(port) is_member(g_fastspan_config_mask, (port))
#define STP_IS_PROTECT_CONFIGURED(_port_) IS_MEMBER(stp_global.protect_mask, (_port_))
#define STP_IS_PROTECT_DO_DISABLE_CONFIGURED(_port_) IS_MEMBER(stp_global.protect_do_disable_mask, (_port_))
#define STP_IS_PROTECT_DO_DISABLED(_port_) IS_MEMBER(stp_global.protect_disabled_mask, (_port_))
#define STP_IS_ROOT_PROTECT_CONFIGURED(_p_) IS_MEMBER(stp_global.root_protect_mask, (_p_))

// _port_ is a pointer to either STP_PORT_CLASS or RSTP_PORT_CLASS
#define STP_ROOT_PROTECT_TIMER_EXPIRED(_port_) \
	timer_expired(&((_port_)->root_protect_timer), STP_SECONDS_TO_TICKS(stp_global.root_protect_timeout))

#define IS_STP_PER_VLAN_FLAG_SET(_stp_port_class, _flag) (_stp_port_class->flags & _flag)
#define SET_STP_PER_VLAN_FLAG(_stp_port_class, _flag) (_stp_port_class->flags |= _flag)
#define CLR_STP_PER_VLAN_FLAG(_stp_port_class, _flag) (_stp_port_class->flags &= ~(_flag))

// debug macros
/**
 * @struct DEBUG_STP
 * @brief Структура для управления настройками отладки STP.
 */
typedef struct
{
	UINT8 enabled : 1;	  /**< Включение отладки. */
	UINT8 verbose : 1;	  /**< Режим подробной отладки. */
	UINT8 bpdu_rx : 1;	  /**< Логирование получения BPDU. */
	UINT8 bpdu_tx : 1;	  /**< Логирование отправки BPDU. */
	UINT8 event : 1;	  /**< Логирование событий. */
	UINT8 all_vlans : 1;  /**< Отладка для всех VLAN. */
	UINT8 all_ports : 1;  /**< Отладка для всех портов. */
	UINT8 spare : 1;	  /**< Зарезервировано. */
	BITMAP_T *vlan_mask;  /**< Маска VLAN для отладки. */
	PORT_MASK *port_mask; /**< Маска портов для отладки. */
} DEBUG_STP;

/**
 * @struct DEBUG_GLOBAL
 * @brief Структура - отладочный вектор stp
 */
typedef struct DEBUG_GLOBAL_TAG
{
	DEBUG_STP stp; // отладочный вектор stp
} DEBUG_GLOBAL;
extern DEBUG_GLOBAL debugGlobal;

#define STP_DEBUG_VP(vlan_id, port_number)                                           \
	((debugGlobal.stp.enabled) &&                                                    \
	 (debugGlobal.stp.all_vlans || is_member(debugGlobal.stp.vlan_mask, vlan_id)) && \
	 (debugGlobal.stp.all_ports || is_member(debugGlobal.stp.port_mask, port_number)))

#define STP_DEBUG_BPDU_RX(vlan_id, port_number) \
	(debugGlobal.stp.bpdu_rx && STP_DEBUG_VP(vlan_id, port_number))

#define STP_DEBUG_BPDU_TX(vlan_id, port_number) \
	(debugGlobal.stp.bpdu_tx && STP_DEBUG_VP(vlan_id, port_number))

#define STP_DEBUG_EVENT(vlan_id, port_number) \
	(debugGlobal.stp.event && STP_DEBUG_VP(vlan_id, port_number))

#define STP_DEBUG_VERBOSE debugGlobal.stp.verbose

/* logging defines */

#define stplog_new_root(stp_class, src) \
	// printf("\n stplog_new_root\n")

#define stplog_root_change(stp_class, src) \
	// printf("\n stplog_root_change\n")

#define stplog_port_state_change(stp_class, port_number, src) \
	// printf("\n stplog_port_state_change\n")

#define stplog_topo_change(stp_class, port_number, src) \
	// printf("\n stplog_topo_change\n")

#define stplog_root_port_change(stp_class, port_number, src) \
	// printf("\n stplog_root_port_change\n")

/**
 * @enum STP_CLASS_STATE
 * @brief Состояния экземпляра STP.
 */
enum STP_CLASS_STATE
{
	STP_CLASS_FREE = 0,	  /**< Экземпляр свободен. */
	STP_CLASS_CONFIG = 1, /**< Экземпляр настроен. */
	STP_CLASS_ACTIVE = 2  /**< Экземпляр активен. */
};

/*
 * NOTE: if fields are added to this enum, please also add them
 * to stp_log_msg_src_string defined in stp_debug.c
 */
/**
 * @enum STP_LOG_MSG_SRC
 * @brief состояния интерфейса в логе STP
 */
enum STP_LOG_MSG_SRC
{
	STP_SRC_NOT_IMPORTANT = 0,
	STP_DISABLE_PORT,
	STP_CHANGE_PRIORITY,
	STP_MESSAGE_AGE_EXPIRY,
	STP_FWD_DLY_EXPIRY,
	STP_BPDU_RECEIVED,
	STP_MAKE_BLOCKING,
	STP_MAKE_FORWARDING,
	STP_ROOT_SELECTION
};

/**
 * @enum STP_KERNEL_STATE
 * @brief состояния интерфейса в ядре STP
 */
enum STP_KERNEL_STATE
{
	STP_KERNEL_STATE_FORWARD = 1,
	STP_KERNEL_STATE_BLOCKING
};

/**
 * @struct BRIDGE_DATA
 * @brief Представление данных моста в протоколе STP.
 *
 * Эта структура содержит информацию о корневом мосте, портах, стоимости пути и других параметрах,
 * связанных с топологией и состоянием моста.
 */
typedef struct BRIDGE_DATA
{
	BRIDGE_IDENTIFIER root_id;			/**< Идентификатор корневого моста. */
	UINT32 root_path_cost;				/**< Стоимость пути до корневого моста. */
	PORT_ID root_port;					/**< Номер порта, через который достигается корневой мост. */
	UINT8 max_age;						/**< Максимальный возраст сообщений BPDU (в секундах). */
	UINT8 hello_time;					/**< Интервал отправки сообщений Hello (в секундах). */
	UINT8 forward_delay;				/**< Задержка пересылки (в секундах). */
	UINT8 bridge_max_age;				/**< Максимальный возраст сообщений для текущего моста. */
	UINT8 bridge_hello_time;			/**< Интервал Hello для текущего моста. */
	UINT8 bridge_forward_delay;			/**< Задержка пересылки для текущего моста. */
	BRIDGE_IDENTIFIER bridge_id;		/**< Идентификатор текущего моста. */
	UINT32 topology_change_count;		/**< Счётчик изменений топологии. */
	UINT32 topology_change_tick;		/**< Время последнего изменения топологии. */
	UINT8 hold_time : 6;				/**< Время удержания BPDU для предотвращения переполнения буфера. */
	UINT8 topology_change_detected : 1; /**< Флаг обнаружения изменения топологии. */
	UINT8 topology_change : 1;			/**< Флаг, указывающий на активное изменение топологии. */
	UINT8 topology_change_time;			/**< Время изменения топологии (задержка пересылки + максимальный возраст). */
#define STP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT 0
#define STP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT 1
#define STP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT 2
#define STP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT 3
#define STP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT 4
#define STP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT 5
#define STP_BRIDGE_DATA_MEMBER_BRIDGE_MAX_AGE_BIT 6
#define STP_BRIDGE_DATA_MEMBER_BRIDGE_HELLO_TIME_BIT 7
#define STP_BRIDGE_DATA_MEMBER_BRIDGE_FWD_DELAY_BIT 8
#define STP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT 9
#define STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_COUNT_BIT 10
#define STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_TIME_BIT 11
#define STP_BRIDGE_DATA_MEMBER_HOLD_TIME_BIT 12
	UINT32 modified_fields; /**< Поля, которые были изменены. */
} __attribute__((__packed__)) BRIDGE_DATA;

/**
 * @struct STP_CLASS
 * @brief Представление экземпляра STP для конкретного VLAN.
 *
 * Структура описывает параметры и состояние экземпляра STP, связанные с VLAN,
 * включая информацию о мосте, маски портов, таймеры и статистику.
 */
typedef struct STP_CLASS
{
	VLAN_ID vlan_id;			 /**< Идентификатор VLAN для экземпляра (тип UINT16). */
	UINT16 fast_aging : 1;		 /**< Флаг быстрого старения записей. */
	UINT16 spare : 11;			 /**< Зарезервированные биты. */
	UINT16 state : 4;			 /**< Состояние класса (см. перечисление `STP_CLASS_STATE`). */
	BRIDGE_DATA bridge_info;	 /**< Информация о мосте для данного экземпляра. */
	PORT_MASK *enable_mask;		 /**< Маска включённых портов. */
	PORT_MASK *control_mask;	 /**< Маска портов с включённым управлением STP. */
	PORT_MASK *untag_mask;		 /**< Маска портов без тегов VLAN. */
	TIMER hello_timer;			 /**< Таймер Hello сообщений. */
	TIMER tcn_timer;			 /**< Таймер сообщений TCN (Topology Change Notification). */
	TIMER topology_change_timer; /**< Таймер для отслеживания изменений топологии. */
	UINT32 last_expiry_time;	 /**< Время последнего истечения таймера (для логирования событий задержки). */
	UINT32 last_bpdu_rx_time;	 /**< Время получения последнего BPDU (для логирования задержек приема). */
	UINT32 rx_drop_bpdu;		 /**< Количество отброшенных BPDU. */
#define STP_CLASS_MEMBER_VLAN_BIT 0
#define STP_CLASS_MEMBER_BRIDEGINFO_BIT 1
#define STP_CLASS_MEMBER_ALL_PORT_CLASS_BIT 31
	UINT32 modified_fields; /**< Поля, которые были изменены, обозначаются соответствующими битами. */
} __attribute__((__packed__)) STP_CLASS;

/**
 * @struct STP_PORT_CLASS
 * @brief Вектор состояния для порта в  stp
 */
typedef struct STP_PORT_CLASS
{
	PORT_IDENTIFIER port_id;			   /**< Уникальный идентификатор порта. */
	UINT8 state;						   /**< Текущее состояние порта (например, BLOCKING, FORWARDING). */
	UINT8 topology_change_acknowledge : 1; /**< Флаг подтверждения изменения топологии. */
	UINT8 config_pending : 1;			   /**< Флаг ожидания применения конфигурации. */
	UINT8 change_detection_enabled : 1;	   /**< Флаг включения обнаружения изменений. */
	UINT8 self_loop : 1;				   /**< Флаг наличия самопетли (loopback). */
	UINT8 auto_config : 1;				   /**< Флаг автоматической конфигурации. */
	UINT8 operEdge : 1;					   /**< Указывает, является ли порт операционным "краем" (Edge). */
	UINT8 kernel_state : 2;				   /**< Состояние ядра (STP_KERNEL_STATE). */
	UINT32 path_cost;					   /**< Стоимость пути для данного порта. */
	BRIDGE_IDENTIFIER designated_root;	   /**< Идентификатор корневого моста. */
	UINT32 designated_cost;				   /**< Назначенная стоимость пути. */
	BRIDGE_IDENTIFIER designated_bridge;   /**< Идентификатор назначенного моста. */
	PORT_IDENTIFIER designated_port;	   /**< Идентификатор назначенного порта. */
	TIMER message_age_timer;			   /**< Таймер для возраста сообщений. */
	TIMER forward_delay_timer;			   /**< Таймер задержки пересылки. */
	TIMER hold_timer;					   /**< Таймер удержания сообщений. */
	TIMER root_protect_timer;			   /**< Таймер защиты корневого порта. */
	UINT32 forward_transitions;			   /**< Количество переходов в состояние пересылки. */
	UINT32 rx_config_bpdu;				   /**< Количество полученных BPDU конфигурации. */
	UINT32 tx_config_bpdu;				   /**< Количество отправленных BPDU конфигурации. */
	UINT32 rx_tcn_bpdu;					   /**< Количество полученных TCN BPDU. */
	UINT32 tx_tcn_bpdu;					   /**< Количество отправленных TCN BPDU. */
	UINT32 rx_delayed_bpdu;				   /**< Количество задержанных BPDU. */
	UINT32 rx_drop_bpdu;				   /**< Количество отброшенных BPDU. */
#define STP_CLASS_PORT_PRI_FLAG 0x0001
#define STP_CLASS_PATH_COST_FLAG 0x0002
	UINT16 flags; /**< Флаги состояния порта. */
#define STP_PORT_CLASS_MEMBER_PORT_ID_BIT 0
#define STP_PORT_CLASS_MEMBER_PORT_STATE_BIT 1
#define STP_PORT_CLASS_MEMBER_PATH_COST_BIT 2
#define STP_PORT_CLASS_MEMBER_DESIGN_ROOT_BIT 3
#define STP_PORT_CLASS_MEMBER_DESIGN_COST_BIT 4
#define STP_PORT_CLASS_MEMBER_DESIGN_BRIDGE_BIT 5
#define STP_PORT_CLASS_MEMBER_DESIGN_PORT_BIT 6
#define STP_PORT_CLASS_MEMBER_FWD_TRANSITIONS_BIT 7
#define STP_PORT_CLASS_MEMBER_BPDU_SENT_BIT 8
#define STP_PORT_CLASS_MEMBER_BPDU_RECVD_BIT 9
#define STP_PORT_CLASS_MEMBER_TC_SENT_BIT 10
#define STP_PORT_CLASS_MEMBER_TC_RECVD_BIT 11
#define STP_PORT_CLASS_MEMBER_PORT_PRIORITY_BIT 12
/* These are not member of STP Port class but port level information that need to be synced */
#define STP_PORT_CLASS_UPLINK_FAST_BIT 13
#define STP_PORT_CLASS_PORT_FAST_BIT 14
#define STP_PORT_CLASS_ROOT_PROTECT_BIT 15
#define STP_PORT_CLASS_BPDU_PROTECT_BIT 16
#define STP_PORT_CLASS_CLEAR_STATS_BIT 17
	UINT32 modified_fields; /**< Поля, которые были модифицированы. */
} __attribute__((__packed__)) STP_PORT_CLASS;

/**
 * @struct STP_GLOBAL
 * @brief Глобальная структура данных для управления протоколом STP.
 *
 * Содержит общие параметры и данные, необходимые для работы протокола STP,
 * включая настройки глобального состояния, статистику и маски портов.
 */
typedef struct STP_GLOBAL
{
	UINT16 max_instances;				/**< Максимальное количество экземпляров STP. */
	UINT16 active_instances;			/**< Количество активных экземпляров STP. */
	STP_CLASS *class_array;				/**< Указатель на массив экземпляров STP. */
	STP_PORT_CLASS *port_array;			/**< Указатель на массив классов портов. */
	STP_CONFIG_BPDU config_bpdu;		/**< Структура конфигурационного BPDU. */
	STP_TCN_BPDU tcn_bpdu;				/**< Структура BPDU уведомления об изменении топологии. */
	PVST_CONFIG_BPDU pvst_config_bpdu;	/**< Конфигурационный BPDU для PVST. */
	PVST_TCN_BPDU pvst_tcn_bpdu;		/**< BPDU уведомления об изменении топологии для PVST. */
	UINT8 tick_id;						/**< Идентификатор текущего тика. */
	UINT8 bpdu_sync_tick_id;			/**< Идентификатор тика для синхронизации BPDU. */
	UINT8 fast_span : 1;				/**< Флаг быстрого охвата. */
	UINT8 enable : 1;					/**< Флаг включения STP. */
	UINT8 sstp_enabled : 1;				/**< Флаг включения SSTP. */
	UINT8 pvst_protect_do_disable : 1;	/**< Конфигурация защиты PVST (отключение порта при получении PVST BPDU). */
	UINT8 spare1 : 4;					/**< Зарезервированные биты. */
	PORT_MASK *enable_mask;				/**< Маска включённых портов. */
	PORT_MASK *enable_admin_mask;		/**< Маска административно включённых портов. */
	PORT_MASK *fastspan_mask;			/**< Маска портов с функцией Fast Span. */
	PORT_MASK *fastspan_admin_mask;		/**< Административная маска портов Fast Span. */
	PORT_MASK *fastuplink_admin_mask;	/**< Административная маска портов Fast Uplink. */
	PORT_MASK *protect_mask;			/**< Маска портов с включённой защитой. */
	PORT_MASK *protect_do_disable_mask; /**< Маска портов, которые должны быть отключены из-за защиты. */
	PORT_MASK *protect_disabled_mask;	/**< Маска отключённых портов защиты. */
	PORT_MASK *root_protect_mask;		/**< Маска портов с защитой корневого порта. */
	uint16_t root_protect_timeout;		/**< Тайм-аут защиты корневого порта. */
	L2_PROTO_MODE proto_mode;			/**< Режим работы протокола STP. */
	UINT32 stp_drop_count;				/**< Количество отброшенных BPDU для STP. */
	UINT32 tcn_drop_count;				/**< Количество отброшенных TCN BPDU. */
	UINT32 pvst_drop_count;				/**< Количество отброшенных PVST BPDU. */
} __attribute__((__packed__)) STP_GLOBAL;

#define INVALID_STP_PARAM ((UINT32)0xffffffff)

/**
 * @enum STP_RAS_EVENTS
 * @brief События протокола STP для RAS.
 */
typedef enum
{
	STP_RAS_BLOCKING = 1,			   /**< Блокировка порта. */
	STP_RAS_FORWARDING,				   /**< Пересылка данных. */
	STP_RAS_INFERIOR_BPDU_RCVD,		   /**< Получение BPDU с низким приоритетом. */
	STP_RAS_MES_AGE_TIMER_EXPIRY,	   /**< Таймер истечения времени жизни сообщения. */
	STP_RAS_ROOT_PROTECT_TIMER_EXPIRY, /**< Таймер защиты корня истёк. */
	STP_RAS_ROOT_PROTECT_VIOLATION,	   /**< Нарушение защиты корня. */
	STP_RAS_ROOT_ROLE,				   /**< Роль корня. */
	STP_RAS_DESIGNATED_ROLE,		   /**< Роль назначения. */
	STP_RAS_MP_RX_DELAY_EVENT,		   /**< Событие задержки приема MP. */
	STP_RAS_TIMER_DELAY_EVENT,		   /**< Событие задержки таймера. */
	STP_RAS_TCM_DETECTED			   /**< Обнаружено изменение топологии. */
} STP_RAS_EVENTS;

#endif //__STP_H__
