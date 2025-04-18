/**
 * @file stp_debug.c
 * @brief Реализация функций отладки и мониторинга работы STP (Spanning Tree Protocol).
 *
 * Этот файл содержит функции для записи журналов, диагностики и сбора статистики
 * работы протокола STP. Функционал включает:
 * - Вывод данных об интерфейсах.
 * - Вывод глобальной и портовой статистики STP.
 * - Управление настройками отладки.
 *
 * @details
 * Функции из этого файла используются для сбора информации о состоянии сети
 * и диагностики проблем, связанных с конфигурацией и работой STP.
 *
 * @author Broadcom
 * @license Apache License 2.0
 */

#include "stp_inc.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

FILE* dbgfp = NULL;

#define STP_DUMP_START dbgfp = fopen("/var/log/stpd_wbos_dmp.log", "w+")
#define STP_DUMP(fmt, ...)                  \
    do                                      \
    {                                       \
        fprintf(dbgfp, fmt, ##__VA_ARGS__); \
        fflush(dbgfp);                      \
    } while (0)
#define STP_DUMP_STOP fclose(dbgfp)

#define STP_TIMER_STRING(timer_ptr) \
    (is_timer_active(timer_ptr) ? "ACTIVE" : "INACTIVE")
#define L2_STATE_STRING(s, p) l2_port_state_to_string(s, p)

char* l2_port_state_string[] =
    {
        "DISABLED",
        "BLOCKING",
        "LISTENING",
        "LEARNING",
        "FORWARDING",
        "UNKNOWN"};

/**
 * @brief Выводит данные о заданном интерфейсе из базы данных Netlink.
 *
 * Функция формирует детальную информацию о сетевом интерфейсе, включая его
 * имя, индексы, состояние, скорость и другие параметры.
 *
 * @param node Указатель на структуру интерфейса, содержащую данные для вывода.
 *
 * @return void
 */
void stpdbg_dump_nl_db_node(INTERFACE_NODE* node)
{
    STP_DUMP("-------------------------\n");
    STP_DUMP("Name           : %s\n", node->ifname);
    STP_DUMP("Kernel ifindex : %u\n", node->kif_index);
    STP_DUMP("Local  ifindex : %u\n", node->port_id);
    STP_DUMP("OPER State     : %s\n", node->oper_state ? "UP" : "DOWN");
    STP_DUMP("SPEED          : %u\n", node->speed);
    // STP_DUMP("MAC            : "PRINT_MAC_FORMAT"\n", PRINT_MAC_VAL(node->mac));
    STP_DUMP("Master ifindex : %u\n", node->master_ifindex);
    STP_DUMP("Member count   : %u\n", node->member_port_count);
    STP_DUMP("Priority       : %u\n", node->priority);
    STP_DUMP("Path cost      : %u\n", node->path_cost);
    STP_DUMP("\n");
}

/**
 * @brief Выводит данные о конкретном интерфейсе по его имени.
 *
 * Функция ищет интерфейс в базе данных Netlink по имени и выводит
 * его параметры. Если интерфейс не найден, выводится сообщение об ошибке.
 *
 * @param name Имя интерфейса для поиска и вывода данных.
 *
 * @return void
 */
void stpdbg_dump_nl_db_intf(char* name)
{
    INTERFACE_NODE search_node;
    INTERFACE_NODE* node = 0;

    memset(&search_node, 0, sizeof(INTERFACE_NODE));
    memcpy(search_node.ifname, name, IFNAMSIZ);
    if (NULL != (node = avl_find(g_stpd_intf_db, &search_node)))
        stpdbg_dump_nl_db_node(node);
    else
        STP_DUMP("Interface : %s not Found\n", name);
}

/**
 * @brief Выводит всю базу данных Netlink, связанную с интерфейсами.
 *
 * Функция отображает данные обо всех интерфейсах, хранящихся в базе данных Netlink.
 * Это включает информацию об именах интерфейсов, их индексах, состояниях,
 * скоростях и других параметрах.
 *
 * @return void
 */
void stpdbg_dump_nl_db()
{
    INTERFACE_NODE* node = 0;
    struct avl_traverser trav;
    avl_t_init(&trav, g_stpd_intf_db);

    while (NULL != (node = avl_t_next(&trav)))
        stpdbg_dump_nl_db_node(node);
}

/**
 * @brief Выводит глобальную статистику работы STP.
 *
 * Функция собирает и выводит данные о количестве пакетов, ошибок, активных событий
 * и другую информацию, связанную с работой STP.
 *
 * @return void
 */
void stpdbg_dump_stp_stats()
{
    uint16_t i = 0;
    STP_DUMP("STP max port  : %u\n", g_max_stp_port);
    STP_DUMP("Total Sockets : %u\n", g_stpd_stats_libev_no_of_sockets);
    STP_DUMP("No of Active Q's in Libev : %d\n", event_base_get_npriorities(stp_intf_get_evbase()));
    STP_DUMP("event_count_active        : %d\n", event_base_get_num_events(stp_intf_get_evbase(), EVENT_BASE_COUNT_ACTIVE));
    STP_DUMP("virtual_event_count       : %d\n", event_base_get_num_events(stp_intf_get_evbase(), EVENT_BASE_COUNT_VIRTUAL));
    STP_DUMP("event_count               : %d\n", event_base_get_num_events(stp_intf_get_evbase(), EVENT_BASE_COUNT_ADDED));
    STP_DUMP("----Stats----\n");
    STP_DUMP("Timer   : %lu\n", g_stpd_stats_libev_timer);
    STP_DUMP("Pkt-rx  : %lu\n", g_stpd_stats_libev_pktrx);
    STP_DUMP("IPC     : %lu\n", g_stpd_stats_libev_ipc);
    STP_DUMP("Netlink : %lu\n", g_stpd_stats_libev_netlink);

    STP_DUMP("\n");
    STP_DUMP("-----------------------------------------\n");
    STP_DUMP(" Port |   Rx   |   Tx   | Rx-Err | Tx-Err \n");
    STP_DUMP("-----------------------------------------\n");
    for (i = 0; i < g_max_stp_port; i++)
    {
        if (STPD_GET_PKT_COUNT(i, pkt_rx) || STPD_GET_PKT_COUNT(i, pkt_tx) || STPD_GET_PKT_COUNT(i, pkt_rx_err_trunc) || STPD_GET_PKT_COUNT(i, pkt_rx_err) || STPD_GET_PKT_COUNT(i, pkt_tx_err))
        {
            STP_DUMP("%4u  | %6lu | %6lu | %6lu | %6lu \n", i, STPD_GET_PKT_COUNT(i, pkt_rx), STPD_GET_PKT_COUNT(i, pkt_tx), STPD_GET_PKT_COUNT(i, pkt_rx_err) + STPD_GET_PKT_COUNT(i, pkt_rx_err_trunc), STPD_GET_PKT_COUNT(i, pkt_tx_err));
        }
    }
}

/**
 * @brief Выводит глобальные данные структуры STP.
 *
 * Функция формирует вывод с подробной информацией о глобальных настройках
 * STP, включая маски портов, таймеры и параметры конфигурации.
 *
 * @return void
 */
void stpdm_global()
{
    UINT8 enable_string[500], enable_admin_string[500];
    UINT8 fastspan_string[500], fastspan_admin_string[500], fastuplink_admin_string[500];
    UINT8 protect_string[500], protect_do_disable_string[500], protect_disabled_string[500], root_protect_string[500];

    mask_to_string(g_stp_enable_mask, enable_string, 500);
    mask_to_string(g_stp_enable_config_mask, enable_admin_string, 500);
    mask_to_string(stp_global.protect_mask, protect_string, sizeof(protect_string));
    mask_to_string(stp_global.protect_do_disable_mask, protect_do_disable_string, sizeof(protect_do_disable_string));
    mask_to_string(stp_global.protect_disabled_mask, protect_disabled_string, sizeof(protect_disabled_string));
    mask_to_string(stp_global.root_protect_mask, root_protect_string, sizeof(root_protect_string));
    mask_to_string(g_fastspan_mask, fastspan_string, 500);
    mask_to_string(g_fastspan_config_mask, fastspan_admin_string, 500);
    mask_to_string(g_fastuplink_mask, fastuplink_admin_string, 500);

    STP_DUMP("STP GLOBAL DATA STRUCTURE\n");
    STP_DUMP("==============================\n\n\t");

    STP_DUMP(
        "sizeof(STP_GLOBAL)     = %lu bytes\n\t"
        "sizeof(STP_CLASS)      = %lu bytes\n\t"
        "sizeof(STP_PORT_CLASS) = %lu bytes\n\t"
        "max_instances          = %d\n\t"
        "active_instances       = %d\n\t"
        "tick_id                = %d\n\t"
        "fast_span              = %d\n\t"
        "class_array            = 0x%p\n\t"
        "config_bpdu            = 0x%p\n\t"
        "tcn_bpdu               = 0x%p\n\t"
        "pvst_config_bpdu       = 0x%p\n\t"
        "pvst_tcn_bpdu          = 0x%p\n\t"
        "enable_mask            = %s\n\t"
        "enable_admin_mask      = %s\n\t"
        "protect_mask           = %s\n\t"
        "protect_do_disable_mask= %s\n\t"
        "protect_disabled_mask  = %s\n\t"
        "root_protect_mask      = %s\n\t"
        "root_protect_timeout   = %u\n\t"
        "fastspan_mask          = %s\n\t"
        "fastspan_admin_mask    = %s\n\t"
        "fastuplink_admin_mask  = %s\n\t"
        "stp_drop_count     = %u\n\t"
        "tcn_drop_count     = %u\n\t"
        "max port           = %u\n",
        sizeof(STP_GLOBAL),
        sizeof(STP_CLASS),
        sizeof(STP_PORT_CLASS),
        stp_global.max_instances,
        stp_global.active_instances,
        stp_global.tick_id,
        stp_global.fast_span,
        stp_global.class_array,
        &stp_global.config_bpdu,
        &stp_global.tcn_bpdu,
        &stp_global.pvst_config_bpdu,
        &stp_global.pvst_tcn_bpdu,
        enable_string,
        enable_admin_string,
        protect_string,
        protect_do_disable_string,
        protect_disabled_string,
        root_protect_string,
        stp_global.root_protect_timeout,
        fastspan_string,
        fastspan_admin_string,
        fastuplink_admin_string,
        stp_global.stp_drop_count,
        stp_global.tcn_drop_count,
        g_max_stp_port);
}

/**
 * @brief Функция для подготовки буфера к отправке пакета с глобальным дампом текущего состояния stpd
 *
 * @return int32_t длинна сообщения, <0 -> error
 */
int stpdm_global_wbos(char* buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size < 1)
    {
        return -1;
    }

    char* current_pos = buffer;
    size_t remaining_size = buffer_size;
    int total_written = 0;

    uint32_t ret_size = 0;
    int status = 0;
    uint8_t temp_buf[500];
    do
    {
        // STP_DUMP("STP GLOBAL DATA STRUCTURE\n");
        status = snprintf(current_pos + ret_size, buffer_size - ret_size, "stpd status global state vector start>> ");
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        status = snprintf(current_pos + ret_size, buffer_size - ret_size, "sizeof(STP_GLOBAL[bytes])= %lu "
                                                                          "sizeof(STP_CLASS[bytes])= %lu "
                                                                          "sizeof(STP_PORT_CLASS[bytes])= %lu "
                                                                          "max_instances= %d "
                                                                          "active_instances= %d "
                                                                          "tick_id= %d "
                                                                          "fast_span= %d "
                                                                          "class_array= %p "
                                                                          "config_bpdu= %p "
                                                                          "tcn_bpdu= %p "
                                                                          "pvst_config_bpdu= %p "
                                                                          "pvst_tcn_bpdu= %p "
                                                                          "stp_drop_count= %u "
                                                                          "tcn_drop_count= %u "
                                                                          "max_port= %u "
                                                                          "root_protect_timeout= %u ",
                          sizeof(STP_GLOBAL),
                          sizeof(STP_CLASS),
                          sizeof(STP_PORT_CLASS),
                          stp_global.max_instances,
                          stp_global.active_instances,
                          stp_global.tick_id,
                          stp_global.fast_span,
                          stp_global.class_array,
                          &stp_global.config_bpdu,
                          &stp_global.tcn_bpdu,
                          &stp_global.pvst_config_bpdu,
                          &stp_global.pvst_tcn_bpdu,
                          stp_global.stp_drop_count,
                          stp_global.tcn_drop_count,
                          g_max_stp_port,
                          stp_global.root_protect_timeout);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(g_stp_enable_mask, temp_buf, sizeof(temp_buf));
        // status = mask_to_string(g_stp_enable_mask, current_pos + ret_size, buffer_size - ret_size);

        // TODO! REFACTOR
        if (total_written <= 0)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }
        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "enable_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(g_stp_enable_config_mask, temp_buf, sizeof(temp_buf));
        // status = mask_to_string(g_stp_enable_config_mask, current_pos + ret_size, buffer_size - ret_size);
        if (total_written)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "enable_admin_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(stp_global.protect_mask, temp_buf, sizeof(temp_buf));
        // status = mask_to_string(stp_global.protect_mask, current_pos + ret_size, buffer_size - ret_size);
        if (total_written)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "protect_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(stp_global.protect_do_disable_mask, temp_buf, sizeof(temp_buf));
        // status = mask_to_string(stp_global.protect_do_disable_mask, current_pos + ret_size, buffer_size - ret_size);
        if (total_written)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "protect_do_disable_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(stp_global.protect_disabled_mask, temp_buf, sizeof(temp_buf));
        if (total_written)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "protect_disabled_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(stp_global.root_protect_mask, temp_buf, sizeof(temp_buf));
        if (total_written)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "root_protect_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(g_fastspan_mask, temp_buf, sizeof(temp_buf));
        if (total_written)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "fastspan_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(g_fastspan_config_mask, temp_buf, sizeof(temp_buf));
        if (total_written)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "fastspan_admin_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        total_written = mask_to_string(g_fastuplink_mask, temp_buf, sizeof(temp_buf));
        if (total_written)
        {
            temp_buf[0] = '-', temp_buf[1] = '-', temp_buf[2] = '-', temp_buf[3] = '\0';
        }

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "fastuplink_admin_mask= %s ", temp_buf);
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        status = snprintf(current_pos + ret_size, MIN(buffer_size - ret_size, sizeof(temp_buf)), "\n\0"); // end of the message
        if (status)
        {
            ret_size += status;
        }
        else
            break;

        return ret_size;

    } while (false);

    return -2;
}

/**
 * @brief Выводит данные о заданной структуре класса STP.
 *
 * Функция собирает и выводит информацию о VLAN, состоянии, масках портов
 * и других параметрах для конкретного экземпляра STP.
 *
 * @param stp_class Указатель на структуру STP класса для вывода данных.
 *
 * @return void
 */
void stpdm_class(STP_CLASS* stp_class)
{
    UINT8 s1[256], s2[256], s3[256];

    STP_DUMP("STP CLASS STRUCTURE - INDEX %ld\n"
             "====================================\n\t",
             (stp_class - stp_global.class_array));

    memset(s1, 0, 256);
    memset(s2, 0, 256);
    memset(s3, 0, 256);

    mask_to_string(stp_class->enable_mask, s1, 256);
    mask_to_string(stp_class->control_mask, s2, 256);
    mask_to_string(stp_class->untag_mask, s3, 256);

    STP_DUMP("vlan_id               = %d\n\t"
             "state                 = %d\n\t"
             "enable_mask           = %s\n\t"
             "control_mask          = %s\n\t"
             "untag_mask            = %s\n\t"
             "hello_timer           = %s %d\n\t"
             "tcn_timer             = %s %d\n\t"
             "topology_change_timer = %s %d\n\t",
             stp_class->vlan_id,
             stp_class->state,
             s1,
             s2,
             s3,
             STP_TIMER_STRING(&stp_class->hello_timer),
             stp_class->hello_timer.value,
             STP_TIMER_STRING(&stp_class->tcn_timer),
             stp_class->tcn_timer.value,
             STP_TIMER_STRING(&stp_class->topology_change_timer),
             stp_class->topology_change_timer.value);

    stputil_bridge_to_string(&stp_class->bridge_info.root_id, s1, 256);
    stputil_bridge_to_string(&stp_class->bridge_info.bridge_id, s2, 256);

    STP_DUMP("bridge_info\n\t\t"
             "root_id %s, root_path_cost %d\n\t\t"
             "max_age %d, hello_time %d, forward_delay %d\n\t\t"
             "bridge_id %s, root_port %d\n\t\t"
             "bridge_max_age %d, bridge_hello_time %d\n\t\t"
             "bridge_forward_delay %d, hold_time %d\n\t\t",
             s1,
             stp_class->bridge_info.root_path_cost,
             stp_class->bridge_info.max_age,
             stp_class->bridge_info.hello_time,
             stp_class->bridge_info.forward_delay,
             s2,
             stp_class->bridge_info.root_port,
             stp_class->bridge_info.bridge_max_age,
             stp_class->bridge_info.bridge_hello_time,
             stp_class->bridge_info.bridge_forward_delay,
             stp_class->bridge_info.hold_time);

    STP_DUMP("topology_change_detected %d, topology_change %d\n\t\t"
             "topology_change_time %d, topology_change_count %d\n\t\t"
             "topology_change_tick %d\n",
             stp_class->bridge_info.topology_change_detected,
             stp_class->bridge_info.topology_change,
             stp_class->bridge_info.topology_change_time,
             stp_class->bridge_info.topology_change_count,
             (stp_class->bridge_info.topology_change_tick ? (sys_get_seconds() - stp_class->bridge_info.topology_change_tick) : 0));

    STP_DUMP("\n");
}

/**
 * @brief Выводит информацию о состоянии порта для заданного экземпляра STP.
 *
 * Функция отображает данные о состоянии порта, такие как состояние STP,
 * стоимость пути, настройки таймеров и флаги изменений.
 *
 * @param stp_class Указатель на экземпляр STP класса (VLAN).
 * @param port_id Идентификатор порта, данные о котором будут выведены.
 *
 * @return void
 */
void stpdm_port_class(STP_CLASS* stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS* stp_port;
    UINT8 s1[50], s2[50];

    stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
    STP_DUMP("PORT CLASS - VLAN %u PORT %u(%s)\n", stp_class->vlan_id, port_number, stp_intf_get_port_name(port_number));
    STP_DUMP("==================================\n");

    STP_DUMP("port_id                     = %d %d\n"
             "state                       = %s\n",
             stp_port->port_id.priority,
             stp_port->port_id.number,
             L2_STATE_STRING(stp_port->state, stp_port->port_id.number));

    stputil_bridge_to_string(&stp_port->designated_root, s1, 50);
    stputil_bridge_to_string(&stp_port->designated_bridge, s2, 50);

    STP_DUMP("path_cost                   = %d\n"
             "designated_root             = 0x%s\n"
             "designated_cost             = %d\n"
             "designated_bridge           = 0x%s\n"
             "designated_port             = Pri-%d, Num-%d\n"
             "topology_change_acknowledge = %d\n"
             "config_pending              = %d\n"
             "change_detection_enabled    = %d\n"
             "self_loop                   = %d\n"
             "auto_config                 = %d\n"
             "message_age_timer           = %s %d\n"
             "forward_delay_timer         = %s %d\n"
             "hold timer                  = %s %d\n"
             "root_protect_timer          = %s %u\n"
             "forward_transitions         = %d\n"
             "rx_config_bpdu              = %d\n"
             "tx_config_bpdu              = %d\n"
             "rx_tcn_bpdu                 = %d\n"
             "tx_tcn_bpdu                 = %d\n",
             stp_port->path_cost,
             s1,
             stp_port->designated_cost,
             s2,
             stp_port->designated_port.priority,
             stp_port->designated_port.number,
             stp_port->topology_change_acknowledge,
             stp_port->config_pending,
             stp_port->change_detection_enabled,
             stp_port->self_loop,
             stp_port->auto_config,
             STP_TIMER_STRING(&stp_port->message_age_timer),
             stp_port->message_age_timer.value,
             STP_TIMER_STRING(&stp_port->forward_delay_timer),
             stp_port->forward_delay_timer.value,
             STP_TIMER_STRING(&stp_port->hold_timer),
             stp_port->hold_timer.value,
             STP_TIMER_STRING(&stp_port->root_protect_timer),
             stp_port->root_protect_timer.value,
             stp_port->forward_transitions,
             stp_port->rx_config_bpdu,
             stp_port->tx_config_bpdu,
             stp_port->rx_tcn_bpdu,
             stp_port->tx_tcn_bpdu);
}

char* l2_port_state_to_string(uint8_t state, uint32_t port)
{
    if (state >= L2_MAX_PORT_STATE)
        return ("BROKEN");
    if (state == DISABLED && port != BAD_PORT_ID)
    {
        if (IS_MEMBER(stp_global.protect_disabled_mask, port))
            return ("BPDU-DIS");
    }
    return (l2_port_state_string[state]);
}

/**
 * @brief Выводит текущие параметры отладки STP.
 *
 * Функция отображает текущие настройки отладки, такие как уровни
 * логирования, состояние отладки для портов и VLAN.
 *
 * @return void
 */
void stp_debug_show()
{
    char buffer[500];

    STP_DUMP("\n"
             "STP Debug Parameters\n"
             "--------------------\n"
             "STP debugging is : %s\n"
             "  Verbose        : %s\n"
             "  Event          : %s\n"
             "  BPDU-RX        : %s\n"
             "  BPDU-TX        : %s\n",
             debugGlobal.stp.enabled ? "ON" : "OFF",
             debugGlobal.stp.verbose ? "ON" : "OFF",
             debugGlobal.stp.event ? "ON" : "OFF",
             debugGlobal.stp.bpdu_rx ? "ON" : "OFF",
             debugGlobal.stp.bpdu_tx ? "ON" : "OFF");

    STP_DUMP("Ports: ");
    if (debugGlobal.stp.all_ports)
        STP_DUMP("All\n");
    else
    {
        mask_to_string(debugGlobal.stp.port_mask, buffer, sizeof(buffer));
        STP_DUMP("%s\n", buffer);
    }

    STP_DUMP("VLANs: ");
    if (debugGlobal.stp.all_vlans)
    {
        STP_DUMP("All\n");
    }
    else
    {
        mask_to_string(debugGlobal.stp.vlan_mask, buffer, sizeof(buffer));
        STP_DUMP("%s\n", buffer);
    }

    STP_DUMP("\n");
}

/**
 * @brief Включает или отключает глобальную отладку для указанного порта.
 *
 * Функция настраивает режим глобальной отладки для указанного порта.
 * Позволяет включить или отключить вывод отладочной информации для
 * порта на уровне всей сети STP.
 *
 * @param port_id Идентификатор порта, для которого настраивается режим отладки.
 * @param flag Значение:
 *             - `1` для включения отладки.
 *             - `0` для отключения отладки.
 *
 * @return void
 */
void stp_debug_global_enable_port(uint32_t port_id, uint8_t flag)
{
    if (flag)
    {
        if (port_id == BAD_PORT_ID)
        {
            debugGlobal.stp.all_ports = 1;
            bmp_reset_all(debugGlobal.stp.port_mask);
        }
        else
        {
            debugGlobal.stp.all_ports = 0;
            bmp_set(debugGlobal.stp.port_mask, port_id);
        }
    }
    else
    {
        debugGlobal.stp.all_ports = 0;
        if (port_id == BAD_PORT_ID)
            bmp_reset_all(debugGlobal.stp.port_mask);
        else
            bmp_reset(debugGlobal.stp.port_mask, port_id);
    }
}

/**
 * @brief Включает или отключает глобальную отладку для указанного VLAN.
 *
 * Функция настраивает режим глобальной отладки для указанного VLAN.
 * Позволяет включить или отключить вывод отладочной информации для
 * VLAN на уровне всей сети STP.
 *
 * @param vlan_id Идентификатор VLAN, для которого настраивается режим отладки.
 * @param flag Значение:
 *             - `1` для включения отладки.
 *             - `0` для отключения отладки.
 *
 * @return void
 */
void stp_debug_global_enable_vlan(uint16_t vlan_id, uint8_t flag)
{
    if (flag)
    {
        if (vlan_id)
        {
            debugGlobal.stp.all_vlans = 0;
            bmp_set(debugGlobal.stp.vlan_mask, vlan_id);
        }
        else
        {
            debugGlobal.stp.all_vlans = 1;
            bmp_reset_all(debugGlobal.stp.vlan_mask);
        }
    }
    else
    {
        debugGlobal.stp.all_vlans = 0;
        if (vlan_id)
            bmp_reset(debugGlobal.stp.vlan_mask, vlan_id);
        else
            bmp_reset_all(debugGlobal.stp.vlan_mask);
    }
}

/**
 * @brief Обрабатывает управляющее сообщение, связанное с отладкой STP.
 *
 * Функция анализирует полученное управляющее сообщение и выполняет
 * соответствующее действие, связанное с отладкой, мониторингом или изменением
 * параметров работы STP. Используется для обработки внешних команд.
 *
 * @param msg Указатель на структуру сообщения, содержащую данные управления.
 *
 * @return void
 */
void stpdbg_process_ctl_msg(void* msg)
{
    STP_CTL_MSG* pmsg = (STP_CTL_MSG*)msg;
    int detail = 0;
    STP_CLASS* stp_class;
    int i, vlan_id;
    uint32_t port_id;

    if (!pmsg)
    {
        STP_LOG_INFO("pmsg null");
        return;
    }

    STP_LOG_INFO("cmd: %d", pmsg->cmd_type);

    STP_DUMP_START;
    switch (pmsg->cmd_type)
    {
    case STP_CTL_DUMP_ALL:
    {
        /* Dump Global */
        STP_DUMP("GLOBAL:\n");
        stpdm_global();

        /* Dump STP class and port class */
        STP_DUMP("\nSTP CLASS:\n");
        for (i = 0; i < g_stp_instances; i++)
        {
            stp_class = GET_STP_CLASS(i);
            if (stp_class->state != STP_CLASS_FREE)
            {
                stpdm_class(stp_class);

                port_id = port_mask_get_first_port(stp_class->control_mask);
                while (port_id != BAD_PORT_ID)
                {
                    stpdm_port_class(stp_class, port_id);
                    port_id = port_mask_get_next_port(stp_class->control_mask, port_id);
                }
            }
        }

        /* Dump intf DB */
        STP_DUMP("\nNL_DB:\n");
        stpdbg_dump_nl_db();

        /* Dump Lib event stats */
        STP_DUMP("\nLSTATS:\n");
        stpdbg_dump_stp_stats();
        break;
    }
    case STP_CTL_DUMP_GLOBAL:
    {
        stpdm_global();
        break;
    }
    case STP_CTL_DUMP_VLAN:
    {
        vlan_id = pmsg->vlan_id;

        for (i = 0; i < g_stp_instances; i++)
        {
            stp_class = GET_STP_CLASS(i);
            if (stp_class->state != STP_CLASS_FREE && stp_class->vlan_id == vlan_id)
            {
                stpdm_class(stp_class);

                port_id = port_mask_get_first_port(stp_class->control_mask);
                while (port_id != BAD_PORT_ID)
                {
                    stpdm_port_class(stp_class, port_id);
                    port_id = port_mask_get_next_port(stp_class->control_mask, port_id);
                }
            }
        }

        break;
    }
    case STP_CTL_DUMP_INTF:
    {
        vlan_id = pmsg->vlan_id;
        port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);

        for (i = 0; i < g_stp_instances; i++)
        {
            stp_class = GET_STP_CLASS(i);
            if (stp_class->state != STP_CLASS_FREE && stp_class->vlan_id == vlan_id)
                stpdm_port_class(stp_class, port_id);
        }

        break;
    }
    case STP_CTL_DUMP_NL_DB:
    {
        stpdbg_dump_nl_db();
        break;
    }
    case STP_CTL_DUMP_NL_DB_INTF:
    {
        stpdbg_dump_nl_db_intf(pmsg->intf_name);
        break;
    }
    case STP_CTL_SET_LOG_LVL:
    {
        STP_LOG_SET_LEVEL(pmsg->level);
        STP_DUMP("log level set to %d\n", pmsg->level);
        break;
    }
    case STP_CTL_SET_DBG:
    {
        if (pmsg->dbg.flags & STPCTL_DBG_SET_ENABLED)
        {
            debugGlobal.stp.enabled = pmsg->dbg.enabled;

            if (!debugGlobal.stp.enabled)
            {
                /* Reset */
                debugGlobal.stp.verbose = 0;
                debugGlobal.stp.event = 0;
                debugGlobal.stp.bpdu_rx = 0;
                debugGlobal.stp.bpdu_tx = 0;
                debugGlobal.stp.all_ports = 1;
                debugGlobal.stp.all_vlans = 1;

                bmp_reset_all(debugGlobal.stp.vlan_mask);
                bmp_reset_all(debugGlobal.stp.port_mask);

                STP_LOG_SET_LEVEL(STP_LOG_LEVEL_INFO);
            }
        }
        else if (pmsg->dbg.flags & STPCTL_DBG_SET_VERBOSE)
        {
            debugGlobal.stp.verbose = pmsg->dbg.verbose;
            if (debugGlobal.stp.verbose)
                STP_LOG_SET_LEVEL(STP_LOG_LEVEL_DEBUG);
            else
                STP_LOG_SET_LEVEL(STP_LOG_LEVEL_INFO);
        }
        else if (pmsg->dbg.flags & STPCTL_DBG_SET_BPDU_RX ||
                 pmsg->dbg.flags & STPCTL_DBG_SET_BPDU_TX)
        {
            if (pmsg->dbg.flags & STPCTL_DBG_SET_BPDU_RX)
                debugGlobal.stp.bpdu_rx = pmsg->dbg.bpdu_rx;
            if (pmsg->dbg.flags & STPCTL_DBG_SET_BPDU_TX)
                debugGlobal.stp.bpdu_tx = pmsg->dbg.bpdu_tx;
        }
        else if (pmsg->dbg.flags & STPCTL_DBG_SET_EVENT)
            debugGlobal.stp.event = pmsg->dbg.event;
        else if (pmsg->dbg.flags & STPCTL_DBG_SET_PORT)
        {
            port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
            stp_debug_global_enable_port(port_id, pmsg->dbg.port);
        }
        else if (pmsg->dbg.flags & STPCTL_DBG_SET_VLAN)
            stp_debug_global_enable_vlan(pmsg->vlan_id, pmsg->dbg.vlan);
        else if (pmsg->dbg.flags & STPCTL_DBG_SHOW)
            stp_debug_show();
        break;
    }
    case STP_CTL_DUMP_LIBEV_STATS:
    {
        stpdbg_dump_stp_stats();
        break;
    }
    case STP_CTL_CLEAR_ALL:
    {
        stpmgr_clear_statistics(VLAN_ID_INVALID, BAD_PORT_ID);
        STP_DUMP("All stats cleared\n");
        break;
    }
    case STP_CTL_CLEAR_VLAN:
    {
        stpmgr_clear_statistics(pmsg->vlan_id, BAD_PORT_ID);
        STP_DUMP("Stats cleared for VLAN %d\n", pmsg->vlan_id);
        break;
    }
    case STP_CTL_CLEAR_INTF:
    {
        port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
        stpmgr_clear_statistics(VLAN_ID_INVALID, port_id);
        STP_DUMP("Stats cleared for %s\n", pmsg->intf_name);
        break;
    }
    case STP_CTL_CLEAR_VLAN_INTF:
    {
        port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
        stpmgr_clear_statistics(pmsg->vlan_id, port_id);
        STP_DUMP("Stats cleared for VLAN %d %s\n", pmsg->vlan_id, pmsg->intf_name);
        break;
    }
    default:
        STP_DUMP("invalid cmd: %d\n", pmsg->cmd_type);
        break;
    }
    STP_DUMP_STOP;
}
