/**
 * @file stp_dbsync.h
 * @brief Заголовочный файл для синхронизации состояний с базами данных Redis [Sonic]
 *
 *
 * @details
 * Представляет собой вектора состояний VLAN и Port+Vlan. Используется для сбора статистики
 * и настройки логики работы, а также различных режимов защиты и пр.
 * Содержит структуры скелеты структур для мнгновенных векторов сотстояних и заголовки функций
 * для доступа к интерфейсам.
 */

#ifndef __STP_DBSYNC__
#define __STP_DBSYNC__

#ifdef __cplusplus
extern "C"
{
#endif

#define IS_BIT_SET(field, bit) ((field) & (1 << (bit)))					 // проверяет что бит взведен
#define SET_BIT(field, bit) ((field) |= (1 << (bit)))					 // взводит бит
#define SET_ALL_BITS(field) ((field) = 0xFFFFFFFF)						 // взводит все биты
#define RESET_BIT(field, bit) ((field) &= ((unsigned int)~(1 << (bit)))) // обнуляет все биты

#define STP_SYNC_BRIDGE_ID_LEN (20)		  // максимальный размер идентификатора бриджа
#define STP_SYNC_PORT_IDENTIFIER_LEN (16) // максимальный размер идентификатора порта
#define STP_SYNC_PORT_STATE_LEN (15)	  // максимальный размер идентификатора состояния

	/**
	 * @struct STP_VLAN_TABLE
	 * @brief Вектор состояния VLAN и их настроек.
	 */
	typedef struct
	{
		uint16_t vlan_id;							  // идентификатор vlan [num]
		char bridge_id[STP_SYNC_BRIDGE_ID_LEN];		  // идентификатор текущего? бриджа '32768.00.98192CE1FCC0' [string]
		uint8_t max_age;							  // настройка максимального времени жизни bpdu sec [num]
		uint8_t hello_time;							  // настройка периода отправки bpdu sec [num]
		uint8_t forward_delay;						  // задержка переключения состояния порта из начального в конечный режим(forwarding)[num]
		uint8_t hold_time;							  // интервал между отправкой двух последовательных Hello в секундах (3..600)
		uint32_t topology_change_time;				  // время с момента последнего изменения топологии [timestamp]
		uint32_t topology_change_count;				  // количество зафиксированных изменений топологии [num]
		char root_bridge_id[STP_SYNC_BRIDGE_ID_LEN];  // идентификатор корневого бриджа '32768.00.98192CE1FCC0' [string]
		uint32_t root_path_cost;					  // стоимость пути до корневого бриджа [num]
		char desig_bridge_id[STP_SYNC_BRIDGE_ID_LEN]; // идентификатор бриджа для связи с корневым '32768.00.98192CE1FCC0' [string]
		char root_port[IFNAMSIZ];					  // идентификатор корневого порта '32768.00.98192CE1FCC0' [string]
		uint8_t root_max_age;						  // настройка максимального времени жизни bpdu на root sec [num]
		uint8_t root_hello_time;					  // настройка максимального времени жизни bpdu на root sec [num]
		uint8_t root_forward_delay;					  // задержка переключения состояния порта из начального в конечный режим(forwarding) на root [num]
		uint16_t stp_instance;						  // номер иснтанса STP
		uint32_t modified_fields;					  // измененные поля?
	} STP_VLAN_TABLE;

	/**
	 * @struct STP_VLAN_PORT_TABLE
	 * @brief Вектор состояния портов и их настроек.
	 */
	typedef struct
	{
		char if_name[IFNAMSIZ];							// идентификатор текущего интерфейса 'Ethernet50' [string]
		uint16_t port_id;								// id интерфейса (для asic?)
		uint8_t port_priority;							// приоритет порта
		uint16_t vlan_id;								// идентификатор vlan
		uint32_t path_cost;								// стоимость передачи через интерфейс?
		char port_state[STP_SYNC_PORT_STATE_LEN];		// состояние порта
		uint32_t designated_cost;						// стоимость передачи до корневого интерфейса?
		char designated_root[STP_SYNC_BRIDGE_ID_LEN];	// идентификатор текущего порта рута '32768.00.98192CE1FCC0' [string]
		char designated_bridge[STP_SYNC_BRIDGE_ID_LEN]; // идентификатор текущего бриджа рута '32768.00.98192CE1FCC0' [string]
		char designated_port;							//??? номер порта для доступа в рут
		uint32_t forward_transitions;					//???
		uint32_t tx_config_bpdu;						// статистика отправки конфигурационных bpdu
		uint32_t rx_config_bpdu;						// статистика приема конфигурационных bpdu
		uint32_t tx_tcn_bpdu;							// статистика отправки tcn_bpdu
		uint32_t rx_tcn_bpdu;							// статистика приема tcn_bpdu
		uint32_t root_protect_timer;					// время реакции на пропажу рута
		uint8_t clear_stats;							// сброс статистики
		uint32_t modified_fields;						// измененные поля?
	} STP_VLAN_PORT_TABLE;

	extern void stpsync_add_vlan_to_instance(uint16_t vlan_id, uint16_t instance);			   // команда для добавления vlan в инстанс stg asic
	extern void stpsync_del_vlan_from_instance(uint16_t vlan_id, uint16_t instance);		   // команда для удаления vlan в инстанс stg asic
	extern void stpsync_update_stp_class(STP_VLAN_TABLE *stp_vlan);							   // обновление вектора состояния для vlan
	extern void stpsync_del_stp_class(uint16_t vlan_id);									   // удаление вектора состояния для vlan
	extern void stpsync_update_port_class(STP_VLAN_PORT_TABLE *stp_vlan_intf);				   // обновление вектора состояния для порта
	extern void stpsync_del_port_class(char *if_name, uint16_t vlan_id);					   // удаление порта из вектора состояния
	extern void stpsync_update_port_state(char *ifName, uint16_t instance, uint8_t state);	   // обновление состояния порта стп для таблицы stg asic
	extern void stpsync_del_port_state(char *ifName, uint16_t instance);					   // удалить порт из асик
	extern void stpsync_update_vlan_port_state(char *ifName, uint16_t vlan_id, uint8_t state); // обновить состояние vlan
	extern void stpsync_del_vlan_port_state(char *ifName, uint16_t vlan_id);				   // удалить порт из vlan
	extern void stpsync_update_fastage_state(uint16_t vlan_id, bool add);					   // обновить период перехода
	extern uint32_t stpsync_get_port_speed(char *ifName);									   // получить скорость интерфейса
	extern void stpsync_update_port_admin_state(char *ifName, bool up, bool physical);		   // обновить статус порта
	extern void stpsync_update_bpdu_guard_shutdown(char *ifName, bool enabled);				   // обновить статус защитника
	extern void stpsync_del_stp_port(char *ifName);											   // удалить порт из стп
	extern void stpsync_update_port_fast(char *ifName, bool enabled);						   // обновить порт быстро
	extern void stpsync_clear_appdb_stp_tables(void);										   // удалить таблицы из баз
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
