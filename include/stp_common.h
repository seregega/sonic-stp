/**
 * @file stp_common.h
 * @brief Общие определения для работы с протоколом STP.
 *
 * Этот заголовочный файл содержит структуры, перечисления и макросы, используемые
 * для обработки BPDU сообщений и работы с данными STP.
 */

#ifndef __STP_COMMON_H__
#define __STP_COMMON_H__

/*****************************************************************************/
/* definitions and macros                                                    */
/*****************************************************************************/
/**
 * @enum SORT_RETURN
 * @brief Результаты сортировки.
 */
typedef enum SORT_RETURN
{
	LESS_THAN = -1,	 /**< Меньше. */
	EQUAL_TO = 0,	 /**< Равно. */
	GREATER_THAN = 1 /**< Больше. */
} SORT_RETURN;

/**
 * @def PORT_MASK
 * @brief Определение маски портов.
 */
#define PORT_MASK BITMAP_T

/**
 * @def STP_INDEX
 * @brief Тип индекса STP.
 */
#define STP_INDEX UINT16
/**
 * @def STP_INDEX_INVALID
 * @brief Некорректный индекс STP.
 */
#define STP_INDEX_INVALID 0xFFFF

/**
 * @def IS_STP_MAC
 * @brief Проверяет, является ли MAC-адрес STP-адресом.
 *
 * @param _mac_ptr_ Указатель на MAC-адрес.
 * @return true, если адрес совпадает с STP MAC.
 */
#define IS_STP_MAC(_mac_ptr_) (SAME_MAC_ADDRESS((_mac_ptr_), &bridge_group_address))
/**
 * @def IS_PVST_MAC
 * @brief Проверяет, является ли MAC-адрес PVST-адресом.
 *
 * @param _mac_ptr_ Указатель на MAC-адрес.
 * @return true, если адрес совпадает с PVST MAC.
 */
#define IS_PVST_MAC(_mac_ptr_) (SAME_MAC_ADDRESS((_mac_ptr_), &pvst_bridge_group_address))

/*****************************************************************************/
/* enum definitions                                                          */
/*****************************************************************************/

/**
 * @enum STP_BPDU_TYPE
 * @brief Типы BPDU в протоколе STP.
 */
typedef enum STP_BPDU_TYPE
{
	CONFIG_BPDU_TYPE = 0, /**< Конфигурационный BPDU. */
	RSTP_BPDU_TYPE = 2,	  /**< BPDU для RSTP. */
	TCN_BPDU_TYPE = 128	  /**< BPDU уведомления об изменении топологии. */
} STP_BPDU_TYPE;

/*****************************************************************************/
/* structure definitions                                                     */
/*****************************************************************************/

/**
 * @struct BRIDGE_BPDU_FLAGS
 * @brief Флаги BPDU сообщения.
 */
typedef struct BRIDGE_BPDU_FLAGS
{
#if __BYTE_ORDER == __BIG_ENDIAN
	UINT8 topology_change_acknowledgement : 1; /**< Подтверждение изменения топологии. */
	UINT8 blank : 6;						   /**< Зарезервировано. */
	UINT8 topology_change : 1;				   /**< Изменение топологии. */
#else
	UINT8 topology_change : 1;				   /**< Изменение топологии. */
	UINT8 blank : 6;						   /**< Зарезервировано. */
	UINT8 topology_change_acknowledgement : 1; /**< Подтверждение изменения топологии. */
#endif
} __attribute__((packed)) BRIDGE_BPDU_FLAGS;

/**
 * @struct BRIDGE_IDENTIFIER
 * @brief Идентификатор моста.
 */
typedef struct BRIDGE_IDENTIFIER
{
#if __BYTE_ORDER == __BIG_ENDIAN
	UINT16 priority : 4;   /**< Приоритет. */
	UINT16 system_id : 12; /**< Системный идентификатор. */
#else
	UINT16 system_id : 12; /**< Системный идентификатор. */
	UINT16 priority : 4;   /**< Приоритет. */
#endif
	MAC_ADDRESS address; /**< MAC-адрес. */
} __attribute__((packed)) BRIDGE_IDENTIFIER;

/**
 * @struct PORT_IDENTIFIER
 * @brief Идентификатор порта.
 */
typedef struct PORT_IDENTIFIER
{
#if __BYTE_ORDER == __BIG_ENDIAN
	UINT16 priority : 4; /**< Приоритет порта. */
	UINT16 number : 12;	 /**< Номер порта. */
#else
	UINT16 number : 12;	 /**< Номер порта. */
	UINT16 priority : 4; /**< Приоритет порта. */
#endif
} __attribute__((packed)) PORT_IDENTIFIER;

/**
 * @struct STP_CONFIG_BPDU
 * @brief Конфигурационный BPDU для протокола STP.
 */
typedef struct STP_CONFIG_BPDU
{
	MAC_HEADER mac_header;		 /**< Заголовок MAC. */
	LLC_HEADER llc_header;		 /**< Заголовок LLC. */
	UINT16 protocol_id;			 /**< Идентификатор протокола. */
	UINT8 protocol_version_id;	 /**< Версия протокола. */
	UINT8 type;					 /**< Тип BPDU. */
	BRIDGE_BPDU_FLAGS flags;	 /**< Флаги BPDU. */
	BRIDGE_IDENTIFIER root_id;	 /**< Идентификатор корневого моста. */
	UINT32 root_path_cost;		 /**< Стоимость пути до корня. */
	BRIDGE_IDENTIFIER bridge_id; /**< Идентификатор моста. */
	PORT_IDENTIFIER port_id;	 /**< Идентификатор порта. */
	UINT16 message_age;			 /**< Возраст сообщения. */
	UINT16 max_age;				 /**< Максимальный возраст. */
	UINT16 hello_time;			 /**< Время Hello. */
	UINT16 forward_delay;		 /**< Задержка пересылки. */
} __attribute__((packed)) STP_CONFIG_BPDU;

/**
 * @struct STP_TCN_BPDU
 * @brief Структура для представления BPDU уведомления об изменении топологии (Topology Change Notification).
 *
 * Используется для передачи информации об изменении топологии в сети STP.
 * Это упрощённая структура BPDU, содержащая минимальный набор полей.
 */
typedef struct STP_TCN_BPDU
{
	MAC_HEADER mac_header;	   /**< Заголовок уровня MAC, содержащий адреса источника и назначения. */
	LLC_HEADER llc_header;	   /**< Заголовок уровня LLC, содержащий тип кадра. */
	UINT16 protocol_id;		   /**< Идентификатор протокола STP (должен быть 0x0000). */
	UINT8 protocol_version_id; /**< Версия протокола STP (обычно 0). */
	UINT8 type;				   /**< Тип BPDU, указывающий на уведомление об изменении топологии (TCN). */
	UINT8 padding[3];
} __attribute__((packed)) STP_TCN_BPDU;

/**
 * @struct PVST_CONFIG_BPDU
 * @brief Структура конфигурационного BPDU для протокола PVST (Per VLAN Spanning Tree).
 *
 * Используется для передачи информации об управлении топологией в рамках протокола PVST.
 * Включает заголовки MAC и SNAP, а также поля для идентификации VLAN.
 */
typedef struct PVST_CONFIG_BPDU
{
	MAC_HEADER mac_header;		 /**< Заголовок уровня MAC, содержащий адреса источника и назначения. */
	SNAP_HEADER snap_header;	 /**< Заголовок SNAP (Subnetwork Access Protocol). */
	UINT16 protocol_id;			 /**< Идентификатор протокола (должен быть 0x0000 для STP). */
	UINT8 protocol_version_id;	 /**< Версия протокола (обычно 0). */
	UINT8 type;					 /**< Тип BPDU, указывающий на конфигурацию (PVST). */
	BRIDGE_BPDU_FLAGS flags;	 /**< Флаги BPDU, такие как изменение топологии. */
	BRIDGE_IDENTIFIER root_id;	 /**< Идентификатор корневого моста. */
	UINT32 root_path_cost;		 /**< Стоимость пути до корневого моста. */
	BRIDGE_IDENTIFIER bridge_id; /**< Идентификатор текущего моста. */
	PORT_IDENTIFIER port_id;	 /**< Идентификатор порта. */
	UINT16 message_age;			 /**< Возраст сообщения (в десятых долях секунды). */
	UINT16 max_age;				 /**< Максимальный возраст сообщения. */
	UINT16 hello_time;			 /**< Интервал Hello сообщений. */
	UINT16 forward_delay;		 /**< Задержка пересылки. */
	UINT8 padding[3];			 /**< Заполнитель для выравнивания. */
	UINT16 tag_length;			 /**< Длина VLAN тега. */
	UINT16 vlan_id;				 /**< Идентификатор VLAN. */
} __attribute__((__packed__)) PVST_CONFIG_BPDU;

/**
 * @struct PVST_TCN_BPDU
 * @brief Структура BPDU для уведомления об изменении топологии в протоколе PVST.
 *
 * Используется для передачи информации об изменении топологии в сети PVST (Per VLAN Spanning Tree).
 * Эта структура представляет упрощённый BPDU, дополненный полем для выравнивания.
 */
typedef struct PVST_TCN_BPDU
{
	MAC_HEADER mac_header;	   /**< Заголовок уровня MAC, содержащий адреса источника и назначения. */
	SNAP_HEADER snap_header;   /**< Заголовок SNAP (Subnetwork Access Protocol). */
	UINT16 protocol_id;		   /**< Идентификатор протокола (должен быть 0x0000 для STP/PVST). */
	UINT8 protocol_version_id; /**< Версия протокола (обычно 0). */
	UINT8 type;				   /**< Тип BPDU (обычно используется для TCN). */
	UINT8 padding[38];		   /**< Заполнитель для обеспечения согласованности структуры. */
} __attribute__((__packed__)) PVST_TCN_BPDU;

#endif //__STP_COMMON_H__
