/**
 * @file l2.h
 * @brief Заголовочный файл для работы с сетевыми структурами уровня L2.
 *
 * Содержит определения типов, структур, перечислений и макросов, используемых
 * для обработки пакетов уровня L2 (включая VLAN, MAC-адреса, LLC, SNAP).
 *
 * @copyright 2019 Broadcom.
 * @license Apache License, Version 2.0.
 */

#ifndef __L2_H__
#define __L2_H__

/* definitions -------------------------------------------------------------- */

#undef BYTE
#define BYTE unsigned char
#undef USHORT
#define USHORT unsigned short
#undef UINT8
#define UINT8 uint8_t
#undef UINT16
#define UINT16 uint16_t
#undef UINT32
#define UINT32 uint32_t
#undef UINT
#define UINT unsigned int
#undef INT32
#define INT32 int32_t

// ---------------------------
// VLAN_ID structure (32-bit value)
// ---------------------------
typedef UINT16 VLAN_ID;

/**
 * @def MIN_VLAN_ID
 * @brief Минимальный допустимый VLAN ID.
 */
#define MIN_VLAN_ID 1

/**
 * @def MAX_VLAN_ID
 * @brief Максимальный допустимый VLAN ID.
 */
#define MAX_VLAN_ID 4095
#define VLAN_ID_INVALID (MAX_VLAN_ID + 1)
#define L2_ETH_ADD_LEN 6

#define VLAN_ID_TAG_BITS 0xFFF
/**
 * @def GET_VLAN_ID_TAG
 * @brief Извлекает VLAN ID из 32-битного значения.
 *
 * @param vlan_id Значение VLAN ID.
 * @return Чистый VLAN ID.
 */
#define GET_VLAN_ID_TAG(vlan_id) (vlan_id & VLAN_ID_TAG_BITS)
/**
 * @def IS_VALID_VLAN
 * @brief Проверяет валидность VLAN ID.
 *
 * @param vlan_id VLAN ID для проверки.
 * @return true, если VLAN ID валиден, иначе false.
 */
#define IS_VALID_VLAN(vlan_id) ((GET_VLAN_ID_TAG(vlan_id) >= MIN_VLAN_ID) && (GET_VLAN_ID_TAG(vlan_id) < MAX_VLAN_ID))

/* enums -------------------------------------------------------------------- */

/**
 * @enum L2_PORT_STATE
 * @brief Состояния порта L2.
 */
enum L2_PORT_STATE
{
	DISABLED = 0,		  /**< Порт отключён. */
	BLOCKING = 1,		  /**< Блокировка трафика. */
	LISTENING = 2,		  /**< Ожидание. */
	LEARNING = 3,		  /**< Изучение. */
	FORWARDING = 4,		  /**< Пересылка трафика. */
	L2_MAX_PORT_STATE = 5 /**< Максимальное состояние. */
};

/**
 * @enum SNAP_PROTOCOL_ID
 * @brief Идентификаторы протоколов SNAP.
 */
enum SNAP_PROTOCOL_ID
{
	SNAP_CISCO_PVST_ID = (0x010b) /**< Cisco PVST протокол. */
};

/**
 * @enum LLC_FRAME_TYPE
 * @brief Типы кадров LLC.
 */
enum LLC_FRAME_TYPE
{
	UNNUMBERED_INFORMATION = 3 /**< Кадр с ненумерованной информацией. */
};

/**
 * @enum SAP_TYPES
 * @brief Типы SAP.
 */
enum SAP_TYPES
{
	LSAP_SNAP_LLC = 0xaa,					  /**< SNAP LLC SAP. */
	LSAP_BRIDGE_SPANNING_TREE_PROTOCOL = 0x42 /**< SAP для STP. */
};

/* structures --------------------------------------------------------------- */

/**
 * @struct LLC_HEADER
 * @brief Заголовок LLC.
 */
typedef struct LLC_HEADER
{
	BYTE destination_address_DSAP; /**< DSAP адрес назначения. */
	BYTE source_address_SSAP;	   /**< SSAP адрес источника. */
	BYTE llc_frame_type;		   /**< Тип LLC-кадра. */
} __attribute__((__packed__)) LLC_HEADER;

/**
 * @struct SNAP_HEADER
 * @brief Заголовок SNAP.
 */
typedef struct SNAP_HEADER
{
	BYTE destination_address_DSAP; /**< DSAP адрес назначения. */
	BYTE source_address_SSAP;	   /**< SSAP адрес источника. */
	BYTE llc_frame_type;		   /**< Тип LLC-кадра. */
	BYTE protocol_id_filler[3];	   /**< Заполнитель для ID протокола. */
	UINT16 protocol_id;			   /**< Идентификатор протокола. */
} __attribute__((__packed__)) SNAP_HEADER;

/**
 * @struct MAC_ADDRESS
 * @brief Представление MAC-адреса.
 */
typedef struct MAC_ADDRESS
{
	UINT32 _ulong;	/**< Старшие 4 байта адреса. */
	UINT16 _ushort; /**< Младшие 2 байта адреса. */
} __attribute__((__packed__)) MAC_ADDRESS;

#define COPY_MAC(sptr_mac2, sptr_mac1) \
	(memcpy(sptr_mac2, sptr_mac1, sizeof(MAC_ADDRESS)))

#define NET_TO_HOST_MAC(sptr_dst_mac, sptr_src_mac)                                           \
	((MAC_ADDRESS *)(sptr_dst_mac))->_ulong = ntohl(((MAC_ADDRESS *)(sptr_src_mac))->_ulong); \
	((MAC_ADDRESS *)(sptr_dst_mac))->_ushort = ntohs(((MAC_ADDRESS *)(sptr_src_mac))->_ushort);

/* copies src mac to dst mac and converts to network byte ordering */
#define HOST_TO_NET_MAC(sptr_dst_mac, sptr_src_mac)                                           \
	((MAC_ADDRESS *)(sptr_dst_mac))->_ulong = htonl(((MAC_ADDRESS *)(sptr_src_mac))->_ulong); \
	((MAC_ADDRESS *)(sptr_dst_mac))->_ushort = htons(((MAC_ADDRESS *)(sptr_src_mac))->_ushort);

typedef struct MAC_HEADER
{
	MAC_ADDRESS destination_address;
	MAC_ADDRESS source_address;

	USHORT length;
} __attribute__((__packed__)) MAC_HEADER;

#define STP_BPDU_OFFSET (sizeof(MAC_HEADER) + sizeof(LLC_HEADER))
#define PVST_BPDU_OFFSET (sizeof(MAC_HEADER) + sizeof(SNAP_HEADER))

#define STP_MAX_PKT_LEN 68
#define VLAN_HEADER_LEN 4 // 4 bytes

#endif //__L2_H__
