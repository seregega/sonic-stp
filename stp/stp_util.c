/**
 * @file stp_util.c
 * @brief Вспомогательные функции для реализации Spanning Tree Protocol (STP).
 *
 * Этот файл содержит набор утилитарных функций, которые используются для
 * поддержки работы протокола STP. Включает функции для обработки идентификаторов мостов,
 * преобразования форматов данных, управления таймерами и отладки.
 *
 * @author
 * Broadcom, 2019. Лицензия Apache License 2.0.
 *
 * @see stp_util.h
 */

#include "stp_inc.h"
#include <stdlib.h>

/**
 * @brief Преобразует идентификатор моста (Bridge ID) в строку.
 *
 * Функция форматирует идентификатор моста в текстовое представление, удобное
 * для вывода или отладки. Результат сохраняется в указанный буфер.
 *
 * @param bridge_id Указатель на идентификатор моста, который необходимо преобразовать.
 * @param buffer Указатель на буфер, куда будет записано текстовое представление идентификатора.
 * @param size Размер буфера, выделенного для строки.
 *
 * @return void
 */
void stputil_bridge_to_string(BRIDGE_IDENTIFIER *bridge_id, UINT8 *buffer, UINT16 size)
{
    union
    {
        UINT8 b[6];
        MAC_ADDRESS s;
    } mac_address;
    uint16_t tmp;

    HOST_TO_NET_MAC(&(mac_address.s), &(bridge_id->address));
    tmp = (bridge_id->priority << 12) | (bridge_id->system_id);

    snprintf(buffer, size, "%04x%02x%02x%02x%02x%02x%02x",
             tmp,
             mac_address.b[0],
             mac_address.b[1],
             mac_address.b[2],
             mac_address.b[3],
             mac_address.b[4],
             mac_address.b[5]);
}

/**
 * @brief Получает значение стоимости пути (Path Cost) по умолчанию для указанного порта.
 *
 * Функция вычисляет значение стоимости пути по умолчанию на основе скорости
 * порта и флага расширенного режима. Значение стоимости пути используется
 * в алгоритмах выбора корневого моста и определения топологии STP.
 *
 * @param port_number Идентификатор порта, для которого требуется определить стоимость пути.
 * @param extend Флаг, указывающий использование расширенного диапазона стоимости пути:
 *               - `true` для использования расширенного диапазона.
 *               - `false` для стандартного диапазона.
 *
 * @return UINT32
 *         - Значение стоимости пути по умолчанию.
 *         - `0`, если порт не найден или произошла ошибка.
 */
UINT32 stputil_get_default_path_cost(PORT_ID port_number, bool extend)
{
    STP_PORT_SPEED port_speed;
    UINT32 path_cost;
    port_speed = stp_intf_get_speed(port_number);

    path_cost = stputil_get_path_cost(port_speed, extend);

    if (!path_cost)
        STP_LOG_ERR("zero path cost %d for intf:%d", path_cost, port_number);

    return path_cost;
}

/**
 * @brief Вычисляет стоимость пути (Path Cost) на основе скорости порта.
 *
 * Функция определяет значение стоимости пути в зависимости от скорости
 * порта и флага расширенного режима. Стоимость пути используется
 * в алгоритмах STP для выбора корневого моста и определения ролей портов.
 *
 * @param port_speed Скорость порта, представленная в виде значения `STP_PORT_SPEED`.
 * @param extend Флаг, указывающий использование расширенного диапазона стоимости пути:
 *               - `true` для использования расширенного диапазона.
 *               - `false` для стандартного диапазона.
 *
 * @return UINT32
 *         - Значение стоимости пути.
 *         - `0`, если скорость порта некорректна.
 */
UINT32 stputil_get_path_cost(STP_PORT_SPEED port_speed, bool extend)
{
    switch (port_speed)
    {
    case STP_SPEED_10M:
        return ((extend) ? STP_PORT_PATH_COST_10M : STP_LEGACY_PORT_PATH_COST_10M);

    case STP_SPEED_100M:
        return ((extend) ? STP_PORT_PATH_COST_100M : STP_LEGACY_PORT_PATH_COST_100M);

    case STP_SPEED_1G:
        return ((extend) ? STP_PORT_PATH_COST_1G : STP_LEGACY_PORT_PATH_COST_1G);

    case STP_SPEED_10G:
        return ((extend) ? STP_PORT_PATH_COST_10G : STP_LEGACY_PORT_PATH_COST_10G);

    case STP_SPEED_25G:
        return ((extend) ? STP_PORT_PATH_COST_25G : STP_LEGACY_PORT_PATH_COST_25G);

    case STP_SPEED_40G:
        return ((extend) ? STP_PORT_PATH_COST_40G : STP_LEGACY_PORT_PATH_COST_40G);

    case STP_SPEED_100G:
        return ((extend) ? STP_PORT_PATH_COST_100G : STP_LEGACY_PORT_PATH_COST_100G);

    case STP_SPEED_400G:
        return ((extend) ? STP_PORT_PATH_COST_400G : STP_LEGACY_PORT_PATH_COST_400G);

    default:
        break;
    }

    STP_LOG_ERR("unknown port speed %d", port_speed);
    return 0; /* error */
}

/**
 * @brief Устанавливает флаг изменения топологии для указанного VLAN.
 *
 * Функция активирует флаг изменения топологии для указанного экземпляра STP.
 * Используется для сигнализации о произошедших изменениях в топологии сети.
 *
 * @param stp_class Указатель на структуру STP_CLASS, представляющую VLAN, для которого
 *                  необходимо установить флаг изменения топологии.
 *
 * @return void
 */
void stputil_set_vlan_topo_change(STP_CLASS *stp_class)
{
    /* The stp fast aging bit keeps track if the fast aging is enabled/disabled.
    Returns if there is no topology change and fast aging is not enabled or if
    there is a topology change and fast aging is already enabled. */
    if (stp_class->bridge_info.topology_change == stp_class->fast_aging)
        return;

    stpsync_update_fastage_state(stp_class->vlan_id, stp_class->bridge_info.topology_change);
    stp_class->fast_aging = stp_class->bridge_info.topology_change;
}

/**
 * @brief Устанавливает состояние порта моста в ядре (kernel bridge port state).
 *
 * Функция синхронизирует состояние порта STP с состоянием порта моста в ядре
 * операционной системы. Используется для управления состояниями портов (например,
 * блокирующий, слушающий, обучающий или пересылающий) на уровне ядра.
 *
 * @param stp_class Указатель на экземпляр структуры `STP_CLASS`, представляющий VLAN.
 * @param stp_port_class Указатель на структуру `STP_PORT_CLASS`, представляющую порт STP,
 *                       состояние которого необходимо установить.
 *
 * @return bool
 *         - `true`, если состояние успешно обновлено.
 *         - `false`, если произошла ошибка при обновлении состояния.
 */
bool stputil_set_kernel_bridge_port_state(STP_CLASS *stp_class, STP_PORT_CLASS *stp_port_class)
{
    char cmd_buff[100];
    int ret;
    char *if_name = GET_STP_PORT_IFNAME(stp_port_class);
    char *tagged;

    if (is_member(stp_class->untag_mask, stp_port_class->port_id.number))
        tagged = "untagged";
    else
        tagged = "tagged";

    if (stp_port_class->state == FORWARDING && stp_port_class->kernel_state != STP_KERNEL_STATE_FORWARD)
    {
        stp_port_class->kernel_state = STP_KERNEL_STATE_FORWARD;
        snprintf(cmd_buff, 100, "/sbin/bridge vlan add vid %u dev %s %s", stp_class->vlan_id, if_name, tagged);
    }
    else if (stp_port_class->state != FORWARDING && stp_port_class->kernel_state != STP_KERNEL_STATE_BLOCKING)
    {
        stp_port_class->kernel_state = STP_KERNEL_STATE_BLOCKING;
        snprintf(cmd_buff, 100, "/sbin/bridge vlan del vid %u dev %s %s", stp_class->vlan_id, if_name, tagged);
    }
    else
    {
        return true; // no-op
    }

    ret = system(cmd_buff);
    if (ret == -1)
    {
        STP_LOG_ERR("Error: cmd - %s strerr - %s", cmd_buff, strerror(errno));
        return false;
    }

    return true;
}

/**
 * @brief Устанавливает состояние порта STP.
 *
 * Функция изменяет состояние указанного порта в соответствии с логикой
 * Spanning Tree Protocol (STP). Она обновляет локальные данные порта и
 * синхронизирует новое состояние с ядром операционной системы, если это требуется.
 *
 * @param stp_class Указатель на экземпляр структуры `STP_CLASS`, представляющий VLAN.
 * @param stp_port_class Указатель на структуру `STP_PORT_CLASS`, представляющую порт,
 *                       состояние которого необходимо установить.
 *
 * @return bool
 *         - `true`, если состояние успешно обновлено.
 *         - `false`, если произошла ошибка при установке состояния.
 */
bool stputil_set_port_state(STP_CLASS *stp_class, STP_PORT_CLASS *stp_port_class)
{
    stputil_set_kernel_bridge_port_state(stp_class, stp_port_class);
    stpsync_update_port_state(GET_STP_PORT_IFNAME(stp_port_class), GET_STP_INDEX(stp_class), stp_port_class->state);
    return true;
}

/**
 * @brief Проверяет, включен ли указанный протокол STP.
 *
 * Функция определяет, активирован ли определённый протокол (например, STP, RSTP, MSTP)
 * в заданном режиме работы уровня 2 (L2).
 *
 * @param proto_mode Режим работы протокола уровня 2 (`L2_PROTO_MODE`), например, STP, RSTP или MSTP.
 *
 * @return bool
 *         - `true`, если протокол включён.
 *         - `false`, если протокол отключён.
 */
bool stputil_is_protocol_enabled(L2_PROTO_MODE proto_mode)
{
    if (stp_global.enable == true && stp_global.proto_mode == proto_mode)
        return true;

    return false;
}

/**
 * @brief Получает экземпляр STP_CLASS для указанного VLAN.
 *
 * Функция возвращает указатель на структуру `STP_CLASS`, связанную с заданным
 * идентификатором VLAN. Используется для доступа к данным конфигурации и состоянию
 * VLAN в контексте STP.
 *
 * @param vlan_id Идентификатор VLAN, для которого требуется получить экземпляр STP_CLASS.
 *
 * @return STP_CLASS*
 *         - Указатель на экземпляр `STP_CLASS`, если VLAN найден.
 *         - `NULL`, если VLAN не найден.
 */
STP_CLASS *stputil_get_class_from_vlan(VLAN_ID vlan_id)
{
    STP_INDEX stp_index;
    STP_CLASS *stp_class;

    for (stp_index = 0; stp_index < g_stp_instances; stp_index++)
    {
        stp_class = GET_STP_CLASS(stp_index);
        if (stp_class->state == STP_CLASS_FREE)
            continue;

        if (stp_class->vlan_id == vlan_id)
            return stp_class;
    }

    return NULL;
}

/**
 * @brief Проверяет, является ли порт нетегированным (untagged) для указанного VLAN.
 *
 * Функция определяет, входит ли порт в список нетегированных портов для заданного VLAN.
 * Используется для управления и диагностики конфигурации портов в контексте STP.
 *
 * @param vlan_id Идентификатор VLAN, для которого выполняется проверка.
 * @param port_id Идентификатор порта, который необходимо проверить.
 *
 * @return bool
 *         - `true`, если порт является нетегированным для данного VLAN.
 *         - `false`, если порт не является нетегированным или VLAN не найден.
 */
bool stputil_is_port_untag(VLAN_ID vlan_id, PORT_ID port_id)
{
    STP_CLASS *stp_class = 0;

    stp_class = stputil_get_class_from_vlan(vlan_id);
    if (stp_class)
        return is_member(stp_class->untag_mask, port_id);

    return false;
}

/**
 * @brief Получает индекс STP (STP_INDEX) для указанного VLAN.
 *
 * Функция выполняет поиск STP-индекса, соответствующего указанному идентификатору VLAN.
 * Индекс используется для доступа к данным STP, связанным с этим VLAN.
 *
 * @param vlan_id Идентификатор VLAN, для которого необходимо получить индекс.
 * @param stp_index Указатель на переменную, в которую будет записан найденный индекс STP.
 *
 * @return bool
 *         - `true`, если индекс найден и записан в переменную.
 *         - `false`, если VLAN не найден или произошла ошибка.
 */
bool stputil_get_index_from_vlan(VLAN_ID vlan_id, STP_INDEX *stp_index)
{
    STP_CLASS *stp_class;
    STP_INDEX i = 0;

    for (i = 0; i < g_stp_instances; i++)
    {
        stp_class = GET_STP_CLASS(i);
        if (stp_class->state == STP_CLASS_FREE)
            continue;

        if (stp_class->vlan_id == vlan_id)
        {
            *stp_index = i;
            return true;
        }
    }
    return false;
}

/**
 * @brief Сравнивает два MAC-адреса.
 *
 * Функция выполняет побайтовое сравнение двух MAC-адресов и определяет их относительный порядок.
 * Используется для сортировки, поиска и сравнения адресов в протоколе STP.
 *
 * @param mac1 Указатель на первый MAC-адрес для сравнения.
 * @param mac2 Указатель на второй MAC-адрес для сравнения.
 *
 * @return SORT_RETURN
 *         - `LESS_THAN`, если `mac1` меньше `mac2`.
 *         - `EQUAL_TO`, если `mac1` равно `mac2`.
 *         - `GREATER_THAN`, если `mac1` больше `mac2`.
 */
enum SORT_RETURN stputil_compare_mac(MAC_ADDRESS *mac1, MAC_ADDRESS *mac2)
{

    if (mac1->_ulong > mac2->_ulong)
        return (GREATER_THAN);

    if (mac1->_ulong == mac2->_ulong)
    {
        if (mac1->_ushort > mac2->_ushort)
            return (GREATER_THAN);
        if (mac1->_ushort == mac2->_ushort)
            return (EQUAL_TO);
    }

    return (LESS_THAN);
}

/**
 * @brief Сравнивает два идентификатора моста (Bridge ID).
 *
 * Функция выполняет сравнение двух идентификаторов мостов в соответствии с правилами
 * STP. Сравнение используется для определения приоритета мостов при построении
 * топологии сети.
 *
 * @param id1 Указатель на первый идентификатор моста для сравнения.
 * @param id2 Указатель на второй идентификатор моста для сравнения.
 *
 * @return SORT_RETURN
 *         - `LESS_THAN`, если `id1` меньше `id2`.
 *         - `EQUAL_TO`, если `id1` равно `id2`.
 *         - `GREATER_THAN`, если `id1` больше `id2`.
 */
enum SORT_RETURN stputil_compare_bridge_id(BRIDGE_IDENTIFIER *id1, BRIDGE_IDENTIFIER *id2)
{
    UINT16 priority1, priority2;

    priority1 = stputil_get_bridge_priority(id1);
    priority2 = stputil_get_bridge_priority(id2);

    if (priority1 > priority2)
        return (GREATER_THAN);

    if (priority1 < priority2)
        return (LESS_THAN);

    return stputil_compare_mac(&id1->address, &id2->address);
}

/**
 * @brief Сравнивает два идентификатора порта (Port ID).
 *
 * Функция выполняет сравнение двух идентификаторов портов в соответствии с правилами STP.
 * Сравнение используется для определения приоритета портов в рамках одного моста.
 *
 * @param port_id1 Указатель на первый идентификатор порта для сравнения.
 * @param port_id2 Указатель на второй идентификатор порта для сравнения.
 *
 * @return SORT_RETURN
 *         - `LESS_THAN`, если `port_id1` меньше `port_id2`.
 *         - `EQUAL_TO`, если `port_id1` равно `port_id2`.
 *         - `GREATER_THAN`, если `port_id1` больше `port_id2`.
 */
enum SORT_RETURN stputil_compare_port_id(PORT_IDENTIFIER *port_id1, PORT_IDENTIFIER *port_id2)
{
    UINT16 port1 = *((UINT16 *)port_id1);
    UINT16 port2 = *((UINT16 *)port_id2);

    if (port1 > port2)
        return (GREATER_THAN);

    if (port1 < port2)
        return (LESS_THAN);

    return (EQUAL_TO);
}

/**
 * @brief Извлекает приоритет моста из идентификатора моста.
 *
 * Функция возвращает значение приоритета моста, которое содержится
 * в указанной структуре идентификатора моста (Bridge ID). Приоритет используется
 * в алгоритмах STP для выбора корневого моста.
 *
 * @param id Указатель на структуру `BRIDGE_IDENTIFIER`, содержащую информацию о мосте.
 *
 * @return UINT16
 *         - Значение приоритета моста.
 *         - `0`, если указатель `id` равен NULL.
 */
UINT16 stputil_get_bridge_priority(BRIDGE_IDENTIFIER *id)
{
    if (g_stpd_extend_mode)
        return (id->priority << 12);
    else
        return ((id->priority << 12) | (id->system_id));
}

/**
 * @brief Устанавливает приоритет моста и обновляет идентификатор моста (Bridge ID).
 *
 * Функция задаёт новое значение приоритета моста и обновляет идентификатор моста (Bridge ID)
 * с учётом указанного VLAN. Приоритет используется в алгоритмах STP для определения
 * роли моста в топологии сети.
 *
 * @param id Указатель на структуру `BRIDGE_IDENTIFIER`, которую необходимо обновить.
 * @param priority Новое значение приоритета моста (тип `UINT16`).
 * @param vlan_id Идентификатор VLAN (тип `VLAN_ID`), который включается в идентификатор моста.
 *
 * @return void
 */
void stputil_set_bridge_priority(BRIDGE_IDENTIFIER *id, UINT16 priority, VLAN_ID vlan_id)
{
    if (g_stpd_extend_mode)
    {
        id->priority = priority >> 12;
        id->system_id = vlan_id & 0xFFF;
    }
    else
    {
        id->priority = priority >> 12;
        id->system_id = priority & 0xFFF;
    }
}

/**
 * @brief Изменяет глобальную маску включения портов STP.
 *
 * Функция добавляет или удаляет указанный порт в глобальной маске включения портов,
 * используемой для управления участием портов в работе протокола STP.
 *
 * @param port_id Идентификатор порта (тип `PORT_ID`), который необходимо добавить или удалить из маски.
 * @param add Флаг действия:
 *            - `1` (true) для добавления порта в маску.
 *            - `0` (false) для удаления порта из маски.
 *
 * @return void
 */
void stputil_set_global_enable_mask(PORT_ID port_id, uint8_t add)
{
    if (add)
        set_mask_bit(g_stp_enable_mask, port_id);
    else
        clear_mask_bit(g_stp_enable_mask, port_id);
}

/**
 * @brief Проверяет корректность BPDU (Bridge Protocol Data Unit).
 *
 * Функция выполняет валидацию структуры BPDU, чтобы убедиться, что данные в ней
 * соответствуют спецификации протокола STP. Используется для проверки входящих
 * BPDU перед их обработкой.
 *
 * @param bpdu Указатель на структуру `STP_CONFIG_BPDU`, содержащую данные BPDU для проверки.
 *
 * @return bool
 *         - `true`, если BPDU валиден.
 *         - `false`, если BPDU содержит ошибки или некорректные данные.
 */
bool stputil_validate_bpdu(STP_CONFIG_BPDU *bpdu)
{
    if (bpdu->llc_header.destination_address_DSAP != LSAP_BRIDGE_SPANNING_TREE_PROTOCOL ||
        bpdu->llc_header.source_address_SSAP != LSAP_BRIDGE_SPANNING_TREE_PROTOCOL ||
        bpdu->llc_header.llc_frame_type != UNNUMBERED_INFORMATION)
    {
        return false;
    }

    if (bpdu->type != CONFIG_BPDU_TYPE &&
        bpdu->type != TCN_BPDU_TYPE)
    {
        return false;
    }

    if (bpdu->type != TCN_BPDU_TYPE)
    {
        if (ntohs(bpdu->hello_time) < STP_MIN_HELLO_TIME << 8)
        {
            // reset to default if received bpdu is incorrect.
            bpdu->hello_time = htons(STP_DFLT_HELLO_TIME << 8);
        }
    }

    return true;
}

/**
 * @brief Проверяет корректность PVST BPDU (Per VLAN Spanning Tree Protocol BPDU).
 *
 * Функция выполняет валидацию структуры PVST BPDU, чтобы убедиться, что данные
 * соответствуют спецификации протокола PVST. Используется для проверки входящих
 * PVST BPDU перед их обработкой.
 *
 * @param bpdu Указатель на структуру `PVST_CONFIG_BPDU`, содержащую данные BPDU для проверки.
 *
 * @return bool
 *         - `true`, если BPDU валиден.
 *         - `false`, если BPDU содержит ошибки или некорректные данные.
 */
bool stputil_validate_pvst_bpdu(PVST_CONFIG_BPDU *bpdu)
{
    if (bpdu->snap_header.destination_address_DSAP != LSAP_SNAP_LLC ||
        bpdu->snap_header.source_address_SSAP != LSAP_SNAP_LLC ||
        bpdu->snap_header.llc_frame_type != (enum LLC_FRAME_TYPE)UNNUMBERED_INFORMATION ||
        bpdu->snap_header.protocol_id_filler[0] != 0x00 ||
        bpdu->snap_header.protocol_id_filler[1] != 0x00 ||
        bpdu->snap_header.protocol_id_filler[2] != 0x0c ||
        ntohs(bpdu->snap_header.protocol_id) != SNAP_CISCO_PVST_ID ||
        ntohs(bpdu->protocol_id) != 0)
    {
        return false;
    }

    if (bpdu->type != CONFIG_BPDU_TYPE &&
        bpdu->type != TCN_BPDU_TYPE)
    {
        return false;
    }

    if (bpdu->type != TCN_BPDU_TYPE)
    {
        bpdu->vlan_id = ntohs(bpdu->vlan_id);
        bpdu->tag_length = ntohs(bpdu->tag_length);

        if ((bpdu->tag_length != 2) ||
            (bpdu->vlan_id < MIN_VLAN_ID || bpdu->vlan_id > MAX_VLAN_ID))
        {
            if (STP_DEBUG_BPDU_RX(bpdu->vlan_id, bpdu->port_id.number))
            {
                STP_PKTLOG("Discarding PVST BPDU with invalid VLAN:%u Port:%d",
                           bpdu->vlan_id, bpdu->port_id.number);
            }
            return false;
        }

        if (ntohs(bpdu->hello_time) < (STP_MIN_HELLO_TIME << 8))
        {
            // reset to default if received bpdu is incorrect.
            bpdu->hello_time = htons(STP_DFLT_HELLO_TIME << 8);
        }
    }

    return true;
}

/**
 * @brief Проверяет, истёк ли таймер защиты корневого порта (Root Protect Timer) для указанного порта.
 *
 * Функция используется для проверки состояния таймера защиты корневого порта
 * на конкретном порту. Если таймер истёк, возвращает `true`, что может инициировать
 * дополнительные действия для обработки событий STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую VLAN, связанный с портом.
 * @param port_number Идентификатор порта, для которого выполняется проверка таймера.
 *
 * @return bool
 *         - `true`, если таймер защиты корневого порта истёк.
 *         - `false`, если таймер ещё активен.
 */
static bool stputil_root_protect_timer_expired(STP_CLASS *stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS *stp_port;

    if (stp_intf_is_port_up(port_number))
    {
        STP_SYSLOG("STP: Root Guard interface %s, VLAN %u consistent (Timeout) ",
                   stp_intf_get_port_name(port_number), stp_class->vlan_id);
        stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
        SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_ROOT_PROTECT_BIT);
    }

    make_forwarding(stp_class, port_number);

    return true;
}

/**
 * @brief Проверяет наличие нарушения защиты корневого порта (Root Protect Violation) на указанном порту.
 *
 * Функция определяет, было ли нарушение защиты корневого порта, проверяя состояние
 * порта и связанные параметры STP. Используется для обработки ситуаций, когда порт
 * пытается получить роль корневого порта в обход настроек защиты.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую VLAN, связанный с портом.
 * @param port_number Идентификатор порта, для которого выполняется проверка на нарушение защиты.
 *
 * @return bool
 *         - `true`, если обнаружено нарушение защиты корневого порта.
 *         - `false`, если нарушение не обнаружено.
 */
static bool stputil_root_protect_violation(STP_CLASS *stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS *stp_port;

    stp_port = GET_STP_PORT_CLASS(stp_class, port_number);

    make_blocking(stp_class, port_number);
    if (!is_timer_active(&stp_port->root_protect_timer))
    {
        // log message
        STP_SYSLOG("STP: Root Guard interface %s, VLAN %u inconsistent (Received superior BPDU) ",
                   stp_intf_get_port_name(port_number), stp_class->vlan_id);
        SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_ROOT_PROTECT_BIT);
    }

    // start/reset timer
    start_timer(&stp_port->root_protect_timer, 0);
    return true;
}

/**
 * @brief Выполняет проверку защиты корневого порта (Root Protect) для входящего BPDU.
 *
 * Функция анализирует BPDU, полученное на указанном порту, чтобы определить, нарушает ли оно
 * настройки защиты корневого порта (Root Protect). Используется для предотвращения
 * назначения порта корневым в обход защиты.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую VLAN, связанный с портом.
 * @param port_number Идентификатор порта, для которого выполняется проверка.
 * @param bpdu Указатель на структуру `STP_CONFIG_BPDU`, содержащую входящее BPDU.
 *
 * @return bool
 *         - `true`, если BPDU нарушает правила защиты корневого порта.
 *         - `false`, если BPDU соответствует настройкам защиты.
 */
static bool stputil_root_protect_validate(STP_CLASS *stp_class, PORT_ID port_number, STP_CONFIG_BPDU *bpdu)
{

    if (bpdu->type != TCN_BPDU_TYPE &&
        supercedes_port_info(stp_class, port_number, bpdu))
    {
        // received superior bpdu on root-protect port
        stputil_root_protect_violation(stp_class, port_number);
        STP_LOG_INFO("STP_RAS_ROOT_PROTECT_VIOLATION I:%lu P:%lu V:%lu", GET_STP_INDEX(stp_class), port_number, stp_class->vlan_id);
        return false;
    }

    return true;
}

/**
 * @brief Проверяет, подходит ли указанный порт для функции быстрого uplink (Fast Uplink).
 *
 * Функция анализирует состояние указанного порта и проверяет, может ли он быть
 * использован в качестве быстрого uplink-порта для указанного экземпляра STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую VLAN или экземпляр STP.
 * @param input_port Идентификатор порта (тип `PORT_ID`), который необходимо проверить.
 *
 * @return bool
 *         - `true`, если порт подходит для функции быстрого uplink.
 *         - `false`, если порт не подходит.
 */
bool stputil_is_fastuplink_ok(STP_CLASS *stp_class, PORT_ID input_port)
{
    PORT_ID port_number;
    STP_PORT_CLASS *stp_port;

    if (!is_member(g_fastuplink_mask, input_port))
        return false;

    port_number = port_mask_get_first_port(stp_class->enable_mask);
    while (port_number != BAD_PORT_ID)
    {
        if ((is_member(g_fastuplink_mask, port_number)) &&
            (port_number != input_port))
        {
            stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
            if ((stp_port->state != BLOCKING) &&
                (stp_port->state != DISABLED))
            {
                return false;
            }
        }

        port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
    }

    return true;
}

/* STP PACKET TX AND RX ROUTINES -------------------------------------------- */

/**
 * @brief Кодирует данные BPDU в формате, пригодном для отправки.
 *
 * Функция преобразует структуру `STP_CONFIG_BPDU`, содержащую данные BPDU,
 * в формат, который может быть отправлен по сети. Кодирование включает
 * упаковку полей в соответствии со спецификацией STP.
 *
 * @param bpdu Указатель на структуру `STP_CONFIG_BPDU`, содержащую данные BPDU для кодирования.
 *
 * @return void
 */
void stputil_encode_bpdu(STP_CONFIG_BPDU *bpdu)
{
    bpdu->protocol_id = htons(bpdu->protocol_id);
    if (bpdu->type == CONFIG_BPDU_TYPE ||
        bpdu->type == RSTP_BPDU_TYPE)
    {
        HOST_TO_NET_MAC(&bpdu->root_id.address, &bpdu->root_id.address);
        *((UINT16 *)&bpdu->root_id) = htons(*((UINT16 *)&bpdu->root_id));
        bpdu->root_path_cost = htonl(bpdu->root_path_cost);
        HOST_TO_NET_MAC(&bpdu->bridge_id.address, &bpdu->bridge_id.address);
        *((UINT16 *)&bpdu->bridge_id) = htons(*((UINT16 *)&bpdu->bridge_id));
        *((UINT16 *)&bpdu->port_id) = htons(*((UINT16 *)&bpdu->port_id));

        bpdu->message_age = htons(bpdu->message_age);
        bpdu->max_age = htons(bpdu->max_age);
        bpdu->hello_time = htons(bpdu->hello_time);
        bpdu->forward_delay = htons(bpdu->forward_delay);
    }
}

/**
 * @brief Декодирует полученные данные BPDU в структуру `STP_CONFIG_BPDU`.
 *
 * Функция принимает данные BPDU, полученные по сети, и преобразует их в
 * структуру `STP_CONFIG_BPDU` для дальнейшей обработки. Распаковка данных
 * выполняется в соответствии со спецификацией STP.
 *
 * @param bpdu Указатель на структуру `STP_CONFIG_BPDU`, куда будут записаны декодированные данные BPDU.
 *
 * @return void
 */
void stputil_decode_bpdu(STP_CONFIG_BPDU *bpdu)
{
    bpdu->protocol_id = ntohs(bpdu->protocol_id);
    if (bpdu->type == CONFIG_BPDU_TYPE ||
        bpdu->type == RSTP_BPDU_TYPE)
    {
        NET_TO_HOST_MAC(&bpdu->root_id.address, &bpdu->root_id.address);
        *((UINT16 *)&bpdu->root_id) = ntohs(*((UINT16 *)&bpdu->root_id));
        bpdu->root_path_cost = ntohl(bpdu->root_path_cost);
        NET_TO_HOST_MAC(&bpdu->bridge_id.address, &bpdu->bridge_id.address);
        *((UINT16 *)&bpdu->bridge_id) = ntohs(*((UINT16 *)&bpdu->bridge_id));
        *((UINT16 *)&bpdu->port_id) = ntohs(*((UINT16 *)&bpdu->port_id));

        bpdu->message_age = ntohs(bpdu->message_age) >> 8;
        bpdu->max_age = ntohs(bpdu->max_age) >> 8;
        bpdu->hello_time = ntohs(bpdu->hello_time) >> 8;
        bpdu->forward_delay = ntohs(bpdu->forward_delay) >> 8;
    }
}

/**
 * @brief Получает идентификатор VLAN, который является нетегированным для указанного порта.
 *
 * Функция определяет VLAN, настроенный как нетегированный (untagged) для указанного порта,
 * что важно для обработки трафика и управления STP.
 *
 * @param port_id Идентификатор порта (тип `PORT_ID`), для которого нужно определить нетегированный VLAN.
 *
 * @return VLAN_ID
 *         - Идентификатор VLAN, настроенного как нетегированный для порта.
 *         - Значение по умолчанию (например, 0), если нетегированный VLAN не настроен.
 */
VLAN_ID stputil_get_untag_vlan(PORT_ID port_id)
{
    STP_INDEX index;
    STP_CLASS *stp_class;

    for (index = 0; index < g_stp_instances; index++)
    {
        stp_class = GET_STP_CLASS(index);
        if (is_member(stp_class->untag_mask, port_id))
            return stp_class->vlan_id;
    }
    return VLAN_ID_INVALID;
}

/**
 * @brief Отправляет BPDU (Bridge Protocol Data Unit) на указанный порт.
 *
 * Функция формирует и отправляет BPDU указанного типа (например, Configuration BPDU или TCN BPDU)
 * для указанного экземпляра STP и порта. Используется для передачи информации протокола STP
 * между мостами в сети.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP (например, для VLAN).
 * @param port_number Идентификатор порта (тип `PORT_ID`), на который нужно отправить BPDU.
 * @param type Тип BPDU (перечисление `STP_BPDU_TYPE`), который нужно отправить:
 *             - `CONFIG_BPDU_TYPE`: Конфигурационное BPDU.
 *             - `TCN_BPDU_TYPE`: BPDU уведомления о топологических изменениях.
 *
 * @return void
 */
void stputil_send_bpdu(STP_CLASS *stp_class, PORT_ID port_number, enum STP_BPDU_TYPE type)
{
    UINT8 *bpdu;
    UINT16 bpdu_size;
    VLAN_ID vlan_id;
    STP_PORT_CLASS *stp_port_class;
    MAC_ADDRESS port_mac = {0, 0};

    stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    stp_intf_get_mac(port_number, &port_mac);

    if (type == CONFIG_BPDU_TYPE)
    {
        // stputil_encode_bpdu(&g_stp_config_bpdu);
        COPY_MAC(&g_stp_config_bpdu.mac_header.source_address, &port_mac);

        bpdu = (UINT8 *)&g_stp_config_bpdu;
        bpdu_size = sizeof(STP_CONFIG_BPDU);
        (stp_port_class->tx_config_bpdu)++;
    }
    else
    {
        if (STP_DEBUG_BPDU_TX(stp_class->vlan_id, port_number))
        {
            STP_PKTLOG("Sending %s BPDU on Vlan:%d Port:%d", (type == CONFIG_BPDU_TYPE ? "Config" : "TCN"),
                       stp_class->vlan_id, port_number);
        }

        COPY_MAC(&g_stp_tcn_bpdu.mac_header.source_address, &port_mac);

        bpdu = (UINT8 *)&g_stp_tcn_bpdu;
        bpdu_size = sizeof(STP_TCN_BPDU);
        (stp_port_class->tx_tcn_bpdu)++;
    }

    vlan_id = stputil_get_untag_vlan(port_number);
    // to send pkt untagged
    if (vlan_id == VLAN_ID_INVALID)
    {
        // port is strictly tagged. can't send out untagged.
        return;
    }

    if (-1 == stp_pkt_tx_handler(port_number, vlan_id, (void *)bpdu, bpdu_size, false))
    {
        // Handle send err
        STP_LOG_ERR("Send STP-BPDU Failed");
    }
}

/**
 * @brief Отправляет PVST BPDU (Per VLAN Spanning Tree Protocol BPDU) на указанный порт.
 *
 * Функция формирует и отправляет BPDU указанного типа (например, Configuration BPDU или TCN BPDU)
 * в контексте протокола PVST для указанного экземпляра STP и порта. Используется для передачи
 * информации протокола PVST между мостами в сети.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP (например, для VLAN).
 * @param port_number Идентификатор порта (тип `PORT_ID`), на который нужно отправить PVST BPDU.
 * @param type Тип BPDU (перечисление `STP_BPDU_TYPE`), который нужно отправить:
 *             - `CONFIG_BPDU_TYPE`: Конфигурационное BPDU.
 *             - `TCN_BPDU_TYPE`: BPDU уведомления о топологических изменениях.
 *
 * @return void
 */
void stputil_send_pvst_bpdu(STP_CLASS *stp_class, PORT_ID port_number, enum STP_BPDU_TYPE type)
{
    UINT8 *bpdu, *bpdu_tmp, *pvst_tmp;
    UINT16 bpdu_size, pvst_inc, bpdu_inc;
    VLAN_ID vlan_id;
    MAC_ADDRESS port_mac = {0, 0};
    STP_PORT_CLASS *stp_port_class;
    bool untagged;

    stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    stp_intf_get_mac(port_number, &port_mac);
    if (type == CONFIG_BPDU_TYPE)
    {
        stputil_encode_bpdu(&g_stp_config_bpdu);

        COPY_MAC(&g_stp_pvst_config_bpdu.mac_header.source_address, &port_mac);

        g_stp_pvst_config_bpdu.vlan_id = htons(GET_VLAN_ID_TAG(stp_class->vlan_id));

        bpdu_inc = sizeof(MAC_HEADER) + sizeof(LLC_HEADER);
        pvst_inc = sizeof(MAC_HEADER) + sizeof(SNAP_HEADER);

        pvst_tmp = (UINT8 *)(&g_stp_pvst_config_bpdu) + pvst_inc;
        bpdu_tmp = (UINT8 *)(&g_stp_config_bpdu) + bpdu_inc;
        memcpy(pvst_tmp, bpdu_tmp, STP_SIZEOF_CONFIG_BPDU);

        bpdu = (UINT8 *)&g_stp_pvst_config_bpdu;
        bpdu_size = sizeof(PVST_CONFIG_BPDU);
        stp_port_class->tx_config_bpdu++;
    }
    else
    {
        if (STP_DEBUG_BPDU_TX(stp_class->vlan_id, port_number))
        {
            STP_PKTLOG("Sending PVST TCN BPDU on Vlan:%d Port:%d", stp_class->vlan_id, port_number);
        }

        COPY_MAC(&g_stp_pvst_tcn_bpdu.mac_header.source_address, &port_mac);

        bpdu = (UINT8 *)&g_stp_pvst_tcn_bpdu;
        bpdu_size = sizeof(PVST_TCN_BPDU);
        stp_port_class->tx_tcn_bpdu++;
    }

    vlan_id = stp_class->vlan_id;

    untagged = stputil_is_port_untag(vlan_id, port_number);

    if (-1 == stp_pkt_tx_handler(port_number, vlan_id, (void *)bpdu, bpdu_size, !untagged))
    {
        // Handle send err
        STP_LOG_ERR("Send PVST-BPDU Failed Vlan %u Port %u", vlan_id, port_number);
    }

    // PVST+ compatibility
    // send an untagged IEEE BPDU when sending a PVST BPDU for VLAN 1
    if (stp_class->vlan_id == 1)
    {
        stputil_send_bpdu(stp_class, port_number, type);
    }
}

/**
 * @brief Обрабатывает входящее BPDU (Bridge Protocol Data Unit) на указанном порту.
 *
 * Функция принимает BPDU, полученное на порту, и выполняет его анализ и обработку
 * в контексте указанного экземпляра STP. Это включает в себя декодирование BPDU,
 * валидацию данных и выполнение соответствующих действий в зависимости от типа BPDU.
 *
 * @param stp_index Индекс экземпляра STP (тип `STP_INDEX`), к которому относится порт.
 * @param port_number Идентификатор порта (тип `PORT_ID`), на котором было получено BPDU.
 * @param buffer Указатель на буфер, содержащий данные BPDU, полученные по сети.
 *
 * @return void
 */
void stputil_process_bpdu(STP_INDEX stp_index, PORT_ID port_number, void *buffer)
{
    STP_CONFIG_BPDU *bpdu = (STP_CONFIG_BPDU *)buffer;
    STP_CLASS *stp_class = GET_STP_CLASS(stp_index);
    UINT32 last_bpdu_rx_time = 0;
    UINT32 current_time = 0;

    // disable fast span on this port
    if (STP_IS_FASTSPAN_ENABLED(port_number))
    {
        stputil_update_mask(g_fastspan_mask, port_number, false);
        stpsync_update_port_fast(stp_intf_get_port_name(port_number), false);
    }

    if (STP_IS_ROOT_PROTECT_CONFIGURED(port_number) &&
        stputil_root_protect_validate(stp_class, port_number, bpdu) == false)
    {
        // root protect code was triggered, return without any more processing.
        stp_class->rx_drop_bpdu++;
        return;
    }

    last_bpdu_rx_time = stp_class->last_bpdu_rx_time;
    current_time = sys_get_seconds();
    stp_class->last_bpdu_rx_time = current_time;

    if (current_time < last_bpdu_rx_time)
    {
        last_bpdu_rx_time = last_bpdu_rx_time - current_time - 1;
        current_time = (UINT32)-1;
    }

    if (((current_time - last_bpdu_rx_time) > (stp_class->bridge_info.hello_time + 1)) && (last_bpdu_rx_time != 0))
    {
        // RAS logging - Rx Delay Event
        if (debugGlobal.stp.enabled)
        {
            if (STP_DEBUG_VP(stp_class->vlan_id, port_number))
            {
                STP_LOG_INFO("Inst:%u Port:%u Vlan:%u Ev:%d Cur:%d Last:%d",
                             stp_index, port_number, stp_class->vlan_id, STP_RAS_MP_RX_DELAY_EVENT,
                             current_time, last_bpdu_rx_time);
            }
        }
        else
        {
            STP_LOG_INFO("Inst:%u Port:%u Vlan:%u Ev:%d Cur:%d Last:%d",
                         stp_index, port_number, stp_class->vlan_id, STP_RAS_MP_RX_DELAY_EVENT,
                         current_time, last_bpdu_rx_time);
        }
    }

    if (bpdu->type == TCN_BPDU_TYPE)
    {
        received_tcn_bpdu(stp_class, port_number, (STP_TCN_BPDU *)bpdu);
#if 0
        if(root_bridge(stp_class))
            vlanutils_send_layer2_protocol_tcn_event (stp_class->vlan_id, L2_STP, true);
#endif
    }
    else
        received_config_bpdu(stp_class, port_number, bpdu); // both RSTP and CONFIG bpdu
}

/**
 * @brief Обновляет маску портов, добавляя или удаляя указанный порт.
 *
 * Функция модифицирует указанную маску портов (PORT_MASK), добавляя или удаляя порт
 * в зависимости от флага действия. Используется для управления состоянием портов,
 * задействованных в протоколе STP.
 *
 * @param mask Указатель на маску портов (`PORT_MASK`), которую нужно обновить.
 * @param port_number Идентификатор порта (тип `PORT_ID`), который нужно добавить или удалить.
 * @param add Флаг действия:
 *            - `true` для добавления порта в маску.
 *            - `false` для удаления порта из маски.
 *
 * @return void
 */
void stputil_update_mask(PORT_MASK *mask, PORT_ID port_number, bool add)
{
    if (add)
        set_mask_bit(mask, port_number);
    else
        clear_mask_bit(mask, port_number);
    return;
}

/**
 * @brief Синхронизирует состояние таймеров порта с параметрами протокола STP.
 *
 * Функция обновляет и синхронизирует таймеры, связанные с указанным портом,
 * на основе текущего состояния экземпляра STP. Используется для согласования
 * временных параметров, таких как возраст сообщения и задержка пересылки.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP (например, VLAN).
 * @param stp_port Указатель на структуру `STP_PORT_CLASS`, представляющую порт, для которого выполняется синхронизация таймеров.
 *
 * @return void
 */
void stptimer_sync_port_class(STP_CLASS *stp_class, STP_PORT_CLASS *stp_port)
{
    STP_VLAN_PORT_TABLE stp_vlan_intf = {0};
    char *ifname;
    UINT32 timer_value = 0;

    if (!stp_port->modified_fields)
        return;

    ifname = stp_intf_get_port_name(stp_port->port_id.number);
    if (ifname == NULL)
        return;

    strcpy(stp_vlan_intf.if_name, ifname);

    stp_vlan_intf.vlan_id = stp_class->vlan_id;

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PORT_ID_BIT))
    {
        stp_vlan_intf.port_id = stp_port->port_id.number;
    }
    else
    {
        stp_vlan_intf.port_id = 0xFFFF;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PORT_PRIORITY_BIT))
    {
        stp_vlan_intf.port_priority = stp_port->port_id.priority;
    }
    else
    {
        stp_vlan_intf.port_priority = -1;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_ROOT_BIT))
    {
        stputil_bridge_to_string(&stp_port->designated_root, stp_vlan_intf.designated_root, STP_SYNC_BRIDGE_ID_LEN);
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_COST_BIT))
    {
        stp_vlan_intf.designated_cost = stp_port->designated_cost;
    }
    else
    {
        stp_vlan_intf.designated_cost = 0xFFFFFFFF;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_BRIDGE_BIT))
    {
        stputil_bridge_to_string(&stp_port->designated_bridge, stp_vlan_intf.designated_bridge, STP_SYNC_BRIDGE_ID_LEN);
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PORT_STATE_BIT))
    {
        get_timer_value(&stp_port->root_protect_timer, &timer_value);
        if (timer_value != 0 && stp_port->state == BLOCKING)
            strcpy(stp_vlan_intf.port_state, "ROOT-INC");
        else
            strcpy(stp_vlan_intf.port_state, l2_port_state_to_string(stp_port->state, stp_port->port_id.number));

        if (stp_port->state == DISABLED)
        {
            stp_vlan_intf.designated_cost = 0;
            strncpy(stp_vlan_intf.designated_bridge, "0000000000000000", STP_SYNC_BRIDGE_ID_LEN);
            strncpy(stp_vlan_intf.designated_root, "0000000000000000", STP_SYNC_BRIDGE_ID_LEN);
        }
    }
    else
    {
        stp_vlan_intf.port_state[0] = '\0';
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PATH_COST_BIT))
    {
        stp_vlan_intf.path_cost = stp_port->path_cost;
    }
    else
    {
        stp_vlan_intf.path_cost = 0xFFFFFFFF;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_PORT_BIT))
    {
        stp_vlan_intf.designated_port = (stp_port->designated_port.priority << 12 | stp_port->designated_port.number);
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_FWD_TRANSITIONS_BIT))
    {
        stp_vlan_intf.forward_transitions = stp_port->forward_transitions;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_BPDU_SENT_BIT))
    {
        stp_vlan_intf.tx_config_bpdu = stp_port->tx_config_bpdu;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_BPDU_RECVD_BIT))
    {
        stp_vlan_intf.rx_config_bpdu = stp_port->rx_config_bpdu;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_TC_SENT_BIT))
    {
        stp_vlan_intf.tx_tcn_bpdu = stp_port->tx_tcn_bpdu;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_TC_RECVD_BIT))
    {
        stp_vlan_intf.rx_tcn_bpdu = stp_port->rx_tcn_bpdu;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_ROOT_PROTECT_BIT))
    {
        get_timer_value(&stp_port->root_protect_timer, &timer_value);
        if (timer_value != 0)
            stp_vlan_intf.root_protect_timer = (stp_global.root_protect_timeout - STP_TICKS_TO_SECONDS(timer_value));
        else
            stp_vlan_intf.root_protect_timer = 0;
    }
    else
    {
        stp_vlan_intf.root_protect_timer = -1;
    }

    if (IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_CLEAR_STATS_BIT))
    {
        stp_vlan_intf.clear_stats = 1;
    }

    stp_port->modified_fields = 0;
    stpsync_update_port_class(&stp_vlan_intf);
}

/**
 * @brief Синхронизирует таймеры экземпляра STP с текущими параметрами протокола.
 *
 * Функция обновляет состояние всех таймеров, связанных с экземпляром STP, включая
 * таймеры для всех связанных портов, чтобы они соответствовали текущей конфигурации
 * и состояниям протокола STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP (например, VLAN).
 *
 * @return void
 */
void stptimer_sync_stp_class(STP_CLASS *stp_class)
{
    STP_PORT_CLASS *stp_port;
    STP_VLAN_TABLE stp_vlan_table = {0};
    char *ifname;

    if (!stp_class->modified_fields && !stp_class->bridge_info.modified_fields)
        return;

    stp_vlan_table.vlan_id = stp_class->vlan_id;

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT))
    {
        stputil_bridge_to_string(&stp_class->bridge_info.root_id, stp_vlan_table.root_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
        if (root_bridge(stp_class))
        {
            strcpy(stp_vlan_table.desig_bridge_id, stp_vlan_table.root_bridge_id);
        }
        else
        {
            stp_port = GET_STP_PORT_CLASS(stp_class, stp_class->bridge_info.root_port);
            if (stp_port)
            {
                stputil_bridge_to_string(&stp_port->designated_bridge, stp_vlan_table.desig_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
            }
        }
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT))
    {
        stp_vlan_table.root_path_cost = stp_class->bridge_info.root_path_cost;
    }
    else
    {
        stp_vlan_table.root_path_cost = 0xFFFFFFFF;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT))
    {
        if (root_bridge(stp_class))
        {
            strcpy(stp_vlan_table.root_port, "Root");
            strcpy(stp_vlan_table.desig_bridge_id, stp_vlan_table.root_bridge_id);
        }
        else
        {
            ifname = stp_intf_get_port_name(stp_class->bridge_info.root_port);
            if (ifname)
                snprintf(stp_vlan_table.root_port, IFNAMSIZ, "%s", ifname);

            stp_port = GET_STP_PORT_CLASS(stp_class, stp_class->bridge_info.root_port);
            if (stp_port)
            {
                stputil_bridge_to_string(&stp_port->designated_bridge, stp_vlan_table.desig_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
            }
        }
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT))
    {
        stp_vlan_table.root_max_age = stp_class->bridge_info.max_age;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT))
    {
        stp_vlan_table.root_hello_time = stp_class->bridge_info.hello_time;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT))
    {
        stp_vlan_table.root_forward_delay = stp_class->bridge_info.forward_delay;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_HOLD_TIME_BIT))
    {
        stp_vlan_table.hold_time = stp_class->bridge_info.hold_time;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_MAX_AGE_BIT))
    {
        stp_vlan_table.max_age = stp_class->bridge_info.bridge_max_age;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_HELLO_TIME_BIT))
    {
        stp_vlan_table.hello_time = stp_class->bridge_info.bridge_hello_time;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_FWD_DELAY_BIT))
    {
        stp_vlan_table.forward_delay = stp_class->bridge_info.bridge_forward_delay;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT))
    {
        stputil_bridge_to_string(&stp_class->bridge_info.bridge_id, stp_vlan_table.bridge_id, STP_SYNC_BRIDGE_ID_LEN);
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_COUNT_BIT))
    {
        stp_vlan_table.topology_change_count = stp_class->bridge_info.topology_change_count;
    }

    if (IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_TIME_BIT))
    {
        stp_vlan_table.topology_change_time = (stp_class->bridge_info.topology_change_tick ? (sys_get_seconds() - stp_class->bridge_info.topology_change_tick) : 0);
    }

    stp_vlan_table.stp_instance = GET_STP_INDEX(stp_class);

    stp_class->modified_fields = 0;
    stp_class->bridge_info.modified_fields = 0;

    stpsync_update_stp_class(&stp_vlan_table);
}

/**
 * @brief Синхронизирует таймеры базы данных STP с текущим состоянием экземпляра STP.
 *
 * Функция обновляет все таймеры, связанные с базой данных STP, включая таймеры
 * для экземпляра STP и его портов. Обеспечивает согласованность временных параметров
 * с текущей конфигурацией протокола.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP (например, VLAN).
 *
 * @return void
 */
void stptimer_sync_db(STP_CLASS *stp_class)
{
    PORT_ID port_number;
    STP_PORT_CLASS *stp_port_class;

    stptimer_sync_stp_class(stp_class);

    if (!stp_class->control_mask)
        return;

    for (port_number = port_mask_get_first_port(stp_class->control_mask);
         port_number != BAD_PORT_ID;
         port_number = port_mask_get_next_port(stp_class->control_mask, port_number))
    {
        stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
        stptimer_sync_port_class(stp_class, stp_port_class);
    }
}

/**
 * @brief Синхронизирует счётчики порта с текущим состоянием экземпляра STP.
 *
 * Функция обновляет статистику порта, включая входящие и исходящие BPDU, изменения
 * топологии и другие ключевые параметры. Используется для отслеживания активности
 * порта и согласования состояния счётчиков с данными экземпляра STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP (например, VLAN).
 * @param stp_port Указатель на структуру `STP_PORT_CLASS`, представляющую порт, для которого выполняется синхронизация счётчиков.
 *
 * @return void
 */
void stputil_sync_port_counters(STP_CLASS *stp_class, STP_PORT_CLASS *stp_port)
{
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_BPDU_SENT_BIT);
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_BPDU_RECVD_BIT);
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_TC_SENT_BIT);
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_TC_RECVD_BIT);

    if (is_timer_active(&stp_port->root_protect_timer))
        SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_ROOT_PROTECT_BIT);

    stptimer_sync_port_class(stp_class, stp_port);
}

/**
 * @brief Синхронизирует счётчики BPDU (Bridge Protocol Data Unit) для экземпляра STP.
 *
 * Функция обновляет статистику BPDU, связанную с указанным экземпляром STP,
 * включая количество полученных и отправленных BPDU. Используется для обеспечения
 * точного отслеживания активности сети и работы протокола STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP (например, VLAN).
 *
 * @return void
 */
void stptimer_sync_bpdu_counters(STP_CLASS *stp_class)
{
    PORT_ID port_number;
    STP_PORT_CLASS *stp_port_class;

    /* Sync topology change tick count */
    if (stp_class->bridge_info.topology_change_tick)
    {
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_TIME_BIT);
        stptimer_sync_stp_class(stp_class);
    }

    for (port_number = port_mask_get_first_port(stp_class->control_mask);
         port_number != BAD_PORT_ID;
         port_number = port_mask_get_next_port(stp_class->control_mask, port_number))
    {
        stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
        stputil_sync_port_counters(stp_class, stp_port_class);
    }
}

/* STP TIMER ROUTINES ------------------------------------------------------- */

/* FUNCTION
 *		stptimer_tick()
 *
 * SYNOPSIS
 *		This function is invoked every 100 ms. The STP instances are
 *		divided into 5 groups with one group getting serviced per
 *		stp_timer_tick() call. Each STP instance will therefore be
 *		serviced every 500 ms. This is done by keeping track of each
 *		100 ms tick (using g_stp_tick_id).
 *
 *		The following table applies -
 *
 *		g_stp_tick_id   schedule STP instances
 *		      0                0,5,10 ...
 *		      1                1,6,11 ...
 *		      2                2,7,12 ...
 *		      3                3,8,13 ...
 *		      4                4,9,14 ...
 *
 */

/**
 * @brief Обрабатывает единичный тик таймера для протокола STP.
 *
 * Функция вызывается периодически (обычно каждые 100 мс или другой фиксированный интервал),
 * чтобы обновить состояние таймеров, связанных с протоколом STP. Отвечает за обработку событий,
 * связанных с истечением времени, таких как управление топологическими изменениями и
 * обработка таймеров портов.
 *
 * @return void
 */
void stptimer_tick()
{
    STP_CLASS *stp_class;
    UINT16 i, start_instance;

    // handle stp timer
    if (g_stp_active_instances)
    {
        for (i = g_stp_tick_id; i < g_stp_instances; i += 5)
        {
            stp_class = GET_STP_CLASS(i);

            if (stp_class->state == STP_CLASS_ACTIVE)
                stptimer_update(stp_class);

            if (stp_class->state == STP_CLASS_ACTIVE || stp_class->state == STP_CLASS_CONFIG)
                //TODO! refactor когда отлучение
                stptimer_sync_db(stp_class);
        }

        if (g_stp_bpdu_sync_tick_id % 10 == 0)
        {
            start_instance = g_stp_bpdu_sync_tick_id / 10;
            for (i = start_instance; i < g_stp_instances; i += 10)
            {
                stp_class = GET_STP_CLASS(i);

                if (stp_class->state == STP_CLASS_ACTIVE)
                    stptimer_sync_bpdu_counters(stp_class);
            }
        }
    }

    g_stp_bpdu_sync_tick_id++;
    if (g_stp_bpdu_sync_tick_id >= 100)
    {
        g_stp_bpdu_sync_tick_id = 0;
    }

    g_stp_tick_id++;
    if (g_stp_tick_id >= 5)
    {
        g_stp_tick_id = 0;
    }
}

/* FUNCTION
 *		stptimer_update()
 *
 * SYNOPSIS
 *		this is called every 500ms for every stp class. it updates the values
 *		of the timers. if any timer has expired, it also executes the timer
 *		expiry routine. this is also currently the point when a topology
 *		change is indicated to the parent vlan.
 */

/**
 * @brief Обновляет состояние всех таймеров для указанного экземпляра STP.
 *
 * Функция проходит по всем таймерам, связанным с указанным экземпляром STP,
 * и обновляет их состояние на основе текущего времени и конфигурации. Обрабатывает
 * события истечения таймеров и вызывает соответствующие обработчики.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP (например, VLAN).
 *
 * @return void
 */
void stptimer_update(STP_CLASS *stp_class)
{
    UINT32 forward_delay;
    PORT_ID port_number;
    STP_PORT_CLASS *stp_port_class;

    if (stptimer_expired(&stp_class->hello_timer, stp_class->bridge_info.hello_time))
    {
        hello_timer_expiry(stp_class);
    }

    if (stptimer_expired(&stp_class->topology_change_timer, stp_class->bridge_info.topology_change_time))
    {
        topology_change_timer_expiry(stp_class);
    }

    if (stptimer_expired(&stp_class->tcn_timer, stp_class->bridge_info.hello_time))
    {
        tcn_timer_expiry(stp_class);
    }

    port_number = port_mask_get_first_port(stp_class->enable_mask);
    while (port_number != BAD_PORT_ID)
    {
        stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

        if (STP_IS_FASTSPAN_ENABLED(port_number))
        {
            forward_delay = STP_FASTSPAN_FORWARD_DELAY;
        }
        else if (stputil_is_fastuplink_ok(stp_class, port_number))
        {
            /* With uplink fast transition to forwarding should happen in 1 sec */
            if (stp_port_class->state == LISTENING)
                forward_delay = STP_FASTUPLINK_FORWARD_DELAY;
            else
                forward_delay = 0;
        }
        else
        {
            forward_delay = stp_class->bridge_info.forward_delay;
        }

        if (stptimer_expired(&stp_port_class->forward_delay_timer, forward_delay))
        {
            forwarding_delay_timer_expiry(stp_class, port_number);
        }

        if (stptimer_expired(&stp_port_class->message_age_timer, stp_class->bridge_info.max_age))
        {
            message_age_timer_expiry(stp_class, port_number);

            if (debugGlobal.stp.enabled)
            {
                if (STP_DEBUG_VP(stp_class->vlan_id, port_number))
                {
                    STP_LOG_INFO("I:%lu P:%u V:%u Ev:%d", GET_STP_INDEX(stp_class), port_number,
                                 stp_class->vlan_id, STP_RAS_MES_AGE_TIMER_EXPIRY);
                }
            }
            else
            {
                STP_LOG_INFO("I:%lu P:%u V:%u Ev:%d", GET_STP_INDEX(stp_class), port_number,
                             stp_class->vlan_id, STP_RAS_MES_AGE_TIMER_EXPIRY);
            }

            /* Sync to APP DB */
            SET_ALL_BITS(stp_class->bridge_info.modified_fields);
            SET_ALL_BITS(stp_class->modified_fields);
        }

        if (stptimer_expired(&stp_port_class->hold_timer,
                             stp_class->bridge_info.hold_time))
        {
            hold_timer_expiry(stp_class, port_number);
        }

        if (stptimer_expired(&stp_port_class->root_protect_timer, stp_global.root_protect_timeout) ||
            (stp_port_class->root_protect_timer.active && !STP_IS_ROOT_PROTECT_CONFIGURED(port_number)))
        {
            stp_port_class->root_protect_timer.active = false;
            stputil_root_protect_timer_expired(stp_class, port_number);

            if (debugGlobal.stp.enabled)
            {
                if (STP_DEBUG_VP(stp_class->vlan_id, port_number))
                {
                    STP_LOG_INFO("I:%lu P:%u V:%u Ev:%d", GET_STP_INDEX(stp_class), port_number,
                                 stp_class->vlan_id, STP_RAS_ROOT_PROTECT_TIMER_EXPIRY);
                }
            }
            else
            {
                STP_LOG_INFO("I:%lu P:%u V:%u Ev:%d", GET_STP_INDEX(stp_class), port_number,
                             stp_class->vlan_id, STP_RAS_ROOT_PROTECT_TIMER_EXPIRY);
            }
        }

        port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
    }

    // initiate fast-aging on vlan if the stp instance is in topology change
    stputil_set_vlan_topo_change(stp_class);
}

/* FUNCTION
 *		stptimer_start()
 *
 * SYNOPSIS
 *		activates timer and sets the initial value to the input start value.
 */
/**
 * @brief Запускает таймер с указанным стартовым значением.
 *
 * Функция инициализирует и запускает таймер, устанавливая его начальное значение
 * в секундах. Используется для управления временными событиями в протоколе STP.
 *
 * @param timer Указатель на структуру `TIMER`, представляющую таймер, который нужно запустить.
 * @param start_value_in_seconds Начальное значение таймера в секундах.
 *
 * @return void
 */
void stptimer_start(TIMER *timer, UINT32 start_value_in_seconds)
{
    start_timer(timer, STP_SECONDS_TO_TICKS(start_value_in_seconds));
}

/**
 * @brief Останавливает указанный таймер.
 *
 * Функция завершает работу таймера, делая его неактивным. Используется для остановки
 * временных событий, которые больше не актуальны в рамках работы протокола STP.
 *
 * @param timer Указатель на структуру `TIMER`, представляющую таймер, который нужно остановить.
 *
 * @return void
 */
void stptimer_stop(TIMER *timer)
{
    stop_timer(timer);
}

/**
 * @brief Проверяет, истёк ли указанный таймер.
 *
 * Функция сравнивает текущее значение таймера с заданным лимитом, чтобы определить,
 * истёк ли таймер. Используется для проверки состояния таймера в рамках работы протокола STP.
 *
 * @param timer Указатель на структуру `TIMER`, представляющую таймер для проверки.
 * @param timer_limit_in_seconds Лимит времени в секундах, по истечении которого таймер считается истёкшим.
 *
 * @return `true`, если таймер истёк, иначе `false`.
 */
bool stptimer_expired(TIMER *timer, UINT32 timer_limit_in_seconds)
{
    return timer_expired(timer, STP_SECONDS_TO_TICKS(timer_limit_in_seconds));
}

/**
 * @brief Проверяет, активен ли указанный таймер.
 *
 * Функция определяет, находится ли таймер в активном состоянии, то есть запущен ли он
 * и имеет ли ненулевое значение. Используется для проверки текущего состояния таймера
 * в рамках работы протокола STP.
 *
 * @param timer Указатель на структуру `TIMER`, представляющую таймер для проверки.
 *
 * @return `true`, если таймер активен, иначе `false`.
 */
bool stptimer_is_active(TIMER *timer)
{
    return (is_timer_active(timer));
}

/**
 * @brief Преобразует битовую маску в строковое представление.
 *
 * Функция конвертирует содержимое битовой маски (`BITMAP_T`) в строку,
 * представляющую активные биты в читаемом формате. Используется для
 * вывода или логирования состояния битовой маски.
 *
 * @param bmp Указатель на структуру `BITMAP_T`, содержащую битовую маску.
 * @param str Указатель на буфер, в который будет записана строка.
 * @param maxlen Максимальная длина буфера `str`, включая завершающий нулевой символ.
 *
 * @return Длина записанной строки (без учёта завершающего нулевого символа)
 *         или отрицательное значение в случае ошибки.
 */
int mask_to_string(BITMAP_T *bmp, uint8_t *str, uint32_t maxlen)
{
    if (!bmp || !bmp->arr || (maxlen < 5) || !str)
    {
        STP_LOG_ERR("Invalid inputs");
        return -1;
    }

    *str = '\0';

    if (is_mask_clear(bmp))
    {
        STP_LOG_DEBUG("BMP is Clear");
        return -1;
    }

    int32_t bmp_id = 0;
    uint32_t len = 0;
    do
    {
        if (len >= maxlen)
        {
            return -1;
        }

        bmp_id = bmp_get_next_set_bit(bmp, bmp_id);
        if (bmp_id != -1)
            len += snprintf((char *)str + len, maxlen - len, "%d ", bmp_id);
    } while (bmp_id != -1);

    return len;
}

/**
 * @brief Проверяет условие и завершает выполнение программы в случае его несоответствия.
 *
 * Функция выполняет проверку указанного условия. Если условие не выполняется
 * (значение `status` равно 0), то функция записывает сообщение об ошибке и завершает
 * выполнение программы. Используется для отладки и проверки корректности выполнения программы.
 *
 * @param status Условие для проверки:
 *               - Если `status` равно 0, программа завершает выполнение.
 *               - Если `status` не равно 0, выполнение продолжается.
 *
 * @return void
 */
void sys_assert(int status)
{
    assert(status);
}
