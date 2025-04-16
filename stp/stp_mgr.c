/**
 * @file stp_mgr.c
 * @brief Реализация менеджера управления протоколом STP (Spanning Tree Protocol).
 *
 * Этот файл содержит функции для управления состояниями портов, синхронизации данных,
 * обработки сообщений и управления топологией в рамках работы STP.
 *
 * @details
 * Реализованы следующие основные функции:
 * - Управление портами (добавление, удаление, изменение состояния).
 * - Обработка сообщений STP и синхронизация с другими модулями.
 * - Управление изменениями топологии сети.
 *
 * @author
 * Broadcom, 2019. Лицензия Apache License 2.0.
 *
 * @see stp_mgr.h
 */

#include "stp_inc.h"
#include "stp_main.h"

MAC_ADDRESS g_stp_base_mac_addr;

char msgtype_str[][64] = {
    "STP_INVALID_MSG",
    "STP_INIT_READY",
    "STP_BRIDGE_CONFIG",
    "STP_VLAN_CONFIG",
    "STP_VLAN_PORT_CONFIG",
    "STP_PORT_CONFIG",
    "STP_VLAN_MEM_CONFIG",
    "STP_STPCTL_MSG",
    "STP_MAX_MSG"};

void stpmgr_libevent_destroy(struct event* ev)
{
    g_stpd_stats_libev_no_of_sockets--;
    event_del(ev);
}

struct event* stpmgr_libevent_create(struct event_base* base,
                                     evutil_socket_t sock,
                                     short flags,
                                     void* cb_fn,
                                     void* arg,
                                     const struct timeval* tv)
{
    struct event* ev = 0;
    int prio;

    g_stpd_stats_libev_no_of_sockets++;

    if (-1 == sock) // 100ms timer
        prio = STP_LIBEV_HIGH_PRI_Q;
    else
    {
        prio = STP_LIBEV_LOW_PRI_Q;
        evutil_make_socket_nonblocking(sock);
    }

    ev = event_new(base, sock, flags, cb_fn, arg);
    if (ev)
    {
        if (-1 == event_priority_set(ev, prio))
        {
            STP_LOG_ERR("event_priority_set failed");
            return NULL;
        }

        if (-1 != event_add(ev, tv))
        {
            STP_LOG_DEBUG("Event Added : ev-%p, arg : %s", ev, (char*)arg);
            STP_LOG_DEBUG("base : %p, sock : %d, flags : %x, cb_fn : %p", base, sock, flags, cb_fn);
            if (tv)
                STP_LOG_DEBUG("tv.sec : %u, tv.usec : %u", tv->tv_sec, tv->tv_usec);

            return ev;
        }
    }
    return NULL;
}

struct event* stpmgr_libevent_create_periodic_sender(struct event_base* base,
                                                     evutil_socket_t sock,
                                                     short flags,
                                                     void* cb_fn,
                                                     void* arg,
                                                     const struct timeval* tv)
{
    struct event* ev = 0;
    int prio;

    prio = STP_LIBEV_LOW_PRI_Q;

    ev = event_new(base, sock, flags, cb_fn, arg);
    if (ev)
    {
        if (-1 == event_priority_set(ev, prio))
        {
            STP_LOG_ERR("event_priority_set failed");
            return NULL;
        }

        if (-1 != event_add(ev, tv))
        {
            STP_LOG_DEBUG("Event Added for periodic sender : ev-%p, arg", ev);
            STP_LOG_DEBUG("base : %p, sock : %d, flags : %x, cb_fn : %p", base, sock, flags, cb_fn);
            if (tv)
                STP_LOG_DEBUG("tv.sec : %u, tv.usec : %u", tv->tv_sec, tv->tv_usec);

            return ev;
        }
    }
    return NULL;
}

/**
 * @brief Инициализирует менеджер STP.
 *
 * Устанавливает начальные параметры менеджера STP, включая создание внутренних
 * структур данных и инициализацию зависимостей.
 *
 * @return 0 в случае успешной инициализации, отрицательное значение в случае ошибки.
 */
void stpmgr_init(UINT16 max_stp_instances)
{
    if (max_stp_instances == 0)
        sys_assert(0);

    if (stpdata_init_global_structures(max_stp_instances) == false)
    {
        STP_LOG_CRITICAL("error - STP global structures initialization failed");
        sys_assert(0);
    }

// TODO! написать сообщение об успешом ините
#ifdef STPD_WBOS_DEBUG
    const char test_messages[] = {
        "stpd info init ok"};

        stpd_context.send_resp_ipc_packet(&stpd_context, test_messages, sizeof(test_messages));
#endif // STPD_WBOS_DEBUG

    STP_LOG_INFO("init done, max stp instances %d", max_stp_instances);
}

/**
 * @brief Инициализирует экземпляр STP для указанного VLAN.
 *
 * Функция выполняет настройку и инициализацию параметров структуры `STP_CLASS`
 * для заданного VLAN. Устанавливает значения по умолчанию и готовит экземпляр
 * STP к работе в указанной VLAN.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param vlan_id Идентификатор VLAN, связанного с данным экземпляром STP.
 *
 * @return void
 */
void stpmgr_initialize_stp_class(STP_CLASS* stp_class, VLAN_ID vlan_id)
{
    STP_INDEX stp_index;

    stp_index = GET_STP_INDEX(stp_class);

    stp_class->vlan_id = vlan_id;

    stputil_set_bridge_priority(&stp_class->bridge_info.bridge_id, STP_DFLT_PRIORITY, vlan_id);
    NET_TO_HOST_MAC(&stp_class->bridge_info.bridge_id.address, &g_stp_base_mac_addr);

    stp_class->bridge_info.bridge_max_age = STP_DFLT_MAX_AGE;
    stp_class->bridge_info.bridge_hello_time = STP_DFLT_HELLO_TIME;
    stp_class->bridge_info.bridge_forward_delay = STP_DFLT_FORWARD_DELAY;
    stp_class->bridge_info.hold_time = STP_DFLT_HOLD_TIME;

    stp_class->bridge_info.root_id = stp_class->bridge_info.bridge_id;
    stp_class->bridge_info.root_path_cost = 0;
    stp_class->bridge_info.root_port = STP_INVALID_PORT;

    stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
    stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
    stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;
    SET_ALL_BITS(stp_class->bridge_info.modified_fields);
    SET_ALL_BITS(stp_class->modified_fields);
}

/**
 * @brief Инициализирует порт управления для указанного экземпляра STP.
 *
 * Функция выполняет настройку и инициализацию параметров управления для
 * конкретного порта в заданном экземпляре STP. Устанавливает значения по умолчанию
 * и готовит порт к работе в рамках протокола STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, который нужно инициализировать.
 *
 * @return void
 */
void stpmgr_initialize_control_port(STP_CLASS* stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS* stp_port_class;

    stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    memset(stp_port_class, 0, sizeof(STP_PORT_CLASS));

    // initialize non-zero values
    stp_port_class->port_id.number = port_number;
    stp_port_class->port_id.priority = stp_intf_get_port_priority(port_number);
    stp_port_class->path_cost = stp_intf_get_path_cost(port_number);
    stp_port_class->change_detection_enabled = true;
    stp_port_class->auto_config = true;
}

/**
 * @brief Активирует указанный экземпляр STP.
 *
 * Функция переводит экземпляр STP в активное состояние, что позволяет ему
 * начать обработку BPDU и управлять портами в рамках протокола STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 *
 * @return void
 */
void stpmgr_activate_stp_class(STP_CLASS* stp_class)
{
    stp_class->state = STP_CLASS_ACTIVE;

    stp_class->bridge_info.topology_change_detected = false;
    stp_class->bridge_info.topology_change = false;

    stptimer_stop(&stp_class->tcn_timer);
    stptimer_stop(&stp_class->topology_change_timer);

    port_state_selection(stp_class);
    config_bpdu_generation(stp_class);
    stptimer_start(&stp_class->hello_timer, 0);
}

/**
 * @brief Деактивирует указанный экземпляр STP.
 *
 * Функция переводит экземпляр STP в неактивное состояние, останавливая обработку
 * BPDU и отключая управление портами в рамках протокола STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 *
 * @return void
 */
void stpmgr_deactivate_stp_class(STP_CLASS* stp_class)
{
    if (stp_class->state == STP_CLASS_CONFIG)
        return;

    stp_class->state = STP_CLASS_CONFIG;

    stptimer_stop(&stp_class->tcn_timer);
    stptimer_stop(&stp_class->topology_change_timer);
    stptimer_stop(&stp_class->hello_timer);

    if (stp_class->bridge_info.topology_change)
    {
        stp_class->bridge_info.topology_change = false;
        stputil_set_vlan_topo_change(stp_class);
    }

    // reset root specific information
    stp_class->bridge_info.root_id = stp_class->bridge_info.bridge_id;
    stp_class->bridge_info.root_path_cost = 0;
    stp_class->bridge_info.root_port = STP_INVALID_PORT;

    stpmgr_set_bridge_params(stp_class);
}

/* 8.8.1 */
/**
 * @brief Инициализирует указанный порт для экземпляра STP.
 *
 * Функция выполняет настройку и инициализацию параметров управления для
 * конкретного порта в заданном экземпляре STP. Устанавливает значения по умолчанию
 * и подготавливает порт к работе в рамках протокола STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, который нужно инициализировать.
 *
 * @return void
 */
void stpmgr_initialize_port(STP_CLASS* stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS* stp_port_class;

    stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    STP_LOG_DEBUG("vlan %d port %d", stp_class->vlan_id, port_number);

    become_designated_port(stp_class, port_number);

    stp_port_class->state = BLOCKING;
    stputil_set_port_state(stp_class, stp_port_class);

    stp_port_class->topology_change_acknowledge = false;
    stp_port_class->config_pending = false;
    stp_port_class->change_detection_enabled = true;
    stp_port_class->self_loop = false;

    stptimer_stop(&stp_port_class->message_age_timer);
    stptimer_stop(&stp_port_class->forward_delay_timer);
    stptimer_stop(&stp_port_class->hold_timer);
}

/* 8.8.2 */
/**
 * @brief Включает указанный порт для экземпляра STP.
 *
 * Функция активирует порт, позволяя ему участвовать в протоколе STP.
 * Устанавливает порт в активное состояние, запускает необходимые таймеры
 * и начинает обработку BPDU для указанного порта.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, который нужно включить.
 *
 * @return void
 */
void stpmgr_enable_port(STP_CLASS* stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS* stp_port_class;
    bool result = false;

    if (is_member(stp_class->enable_mask, port_number))
        return;

    set_mask_bit(stp_class->enable_mask, port_number);

    stpmgr_initialize_port(stp_class, port_number);

    port_state_selection(stp_class);
}

/* 8.8.3 */
/**
 * @brief Отключает указанный порт для экземпляра STP.
 *
 * Функция переводит порт в неактивное состояние, останавливает связанные таймеры,
 * запрещает обработку BPDU и удаляет порт из активных участников STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, который нужно отключить.
 *
 * @return void
 */
void stpmgr_disable_port(STP_CLASS* stp_class, PORT_ID port_number)
{
    bool root;
    STP_PORT_CLASS* stp_port_class;

    if (!is_member(stp_class->enable_mask, port_number))
        return;

    stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

    /* this can happen if a module has been deleted, stp can remove the
     * data structure before vlan has a chance to cleanup. adding a check
     * to only this routine.
     */
    if (stp_port_class == NULL)
        return;

    root = root_bridge(stp_class);
    become_designated_port(stp_class, port_number);

    stp_port_class->state = DISABLED;

    /* do not send a message to VPORT manager to set the state to
     * disabled - this call is being initiated from there
     */

    stp_port_class->topology_change_acknowledge = false;
    stp_port_class->config_pending = false;
    stp_port_class->change_detection_enabled = true;
    stp_port_class->self_loop = false;

    stptimer_stop(&stp_port_class->message_age_timer);
    stptimer_stop(&stp_port_class->forward_delay_timer);

    if (stp_port_class->root_protect_timer.active == true)
    {
        stp_port_class->root_protect_timer.active = false;
        stptimer_stop(&stp_port_class->root_protect_timer);
    }

    clear_mask_bit(stp_class->enable_mask, port_number);
    configuration_update(stp_class);
    port_state_selection(stp_class);

    if (root_bridge(stp_class) && !root)
    {
        stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
        stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
        stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;

        topology_change_detection(stp_class);
        stptimer_stop(&stp_class->tcn_timer);
        config_bpdu_generation(stp_class);
        stptimer_start(&stp_class->hello_timer, 0);

        stplog_topo_change(stp_class, port_number, STP_DISABLE_PORT);
        stplog_new_root(stp_class, STP_DISABLE_PORT);
    }
}

/* 8.8.4 */
/**
 * @brief Устанавливает приоритет моста для указанного экземпляра STP.
 *
 * Функция обновляет приоритет моста (Bridge Priority) в структуре `BRIDGE_IDENTIFIER`
 * для указанного экземпляра STP. Приоритет используется для выбора корневого моста
 * в протоколе STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param bridge_id Указатель на структуру `BRIDGE_IDENTIFIER`, содержащую новый приоритет моста.
 *
 * @return void
 */
void stpmgr_set_bridge_priority(STP_CLASS* stp_class, BRIDGE_IDENTIFIER* bridge_id)
{
    bool root;
    PORT_ID port_number;
    STP_PORT_CLASS* stp_port_class;

    root = root_bridge(stp_class);

    port_number = port_mask_get_first_port(stp_class->enable_mask);
    while (port_number != BAD_PORT_ID)
    {
        stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
        if (designated_port(stp_class, port_number))
        {
            stp_port_class->designated_bridge = *bridge_id;
            SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_BRIDGE_BIT);
        }

        port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
    }

    stp_class->bridge_info.bridge_id = *bridge_id;

    configuration_update(stp_class);
    port_state_selection(stp_class);

    if (root_bridge(stp_class))
    {
        if (!root)
        {
            stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
            stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
            stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;

            topology_change_detection(stp_class);
            stptimer_stop(&stp_class->tcn_timer);
            config_bpdu_generation(stp_class);
            stptimer_start(&stp_class->hello_timer, 0);

            stplog_new_root(stp_class, STP_CHANGE_PRIORITY);
        }
    }
    else
    {
        if (root)
        {
            stplog_root_change(stp_class, STP_CHANGE_PRIORITY);
        }
    }
}

/* 8.8.5 */
/**
 * @brief Устанавливает приоритет порта для указанного экземпляра STP.
 *
 * Функция обновляет приоритет указанного порта в экземпляре STP. Приоритет порта
 * используется для выбора портов с наивысшим приоритетом при определении
 * топологии STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, приоритет которого нужно установить.
 * @param priority Новое значение приоритета порта (0-65535, меньшее значение имеет более высокий приоритет).
 *
 * @return void
 */
void stpmgr_set_port_priority(STP_CLASS* stp_class, PORT_ID port_number, UINT16 priority)
{
    STP_PORT_CLASS* stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

    if (designated_port(stp_class, port_number))
    {
        stp_port_class->designated_port.priority = priority >> 4;
    }

    stp_port_class->port_id.priority = priority >> 4;
    SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_PORT_PRIORITY_BIT);

    if (stputil_compare_bridge_id(&stp_class->bridge_info.bridge_id, &stp_port_class->designated_bridge) == EQUAL_TO &&
        stputil_compare_port_id(&stp_port_class->port_id, &stp_port_class->designated_port) == LESS_THAN)
    {
        become_designated_port(stp_class, port_number);
        port_state_selection(stp_class);

        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_PORT_BIT);
    }
}

/* 8.8.6 */
/**
 * @brief Устанавливает стоимость пути (Path Cost) для указанного порта в экземпляре STP.
 *
 * Функция обновляет стоимость пути для заданного порта, используемую в алгоритме STP
 * для выбора оптимального пути к корневому мосту. При необходимости стоимость
 * может быть установлена автоматически или вручную.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, для которого устанавливается стоимость пути.
 * @param auto_config Флаг автоматической настройки:
 *                    - `true` — стоимость пути рассчитывается автоматически.
 *                    - `false` — стоимость пути задаётся вручную.
 * @param path_cost Стоимость пути, если `auto_config` равен `false`.
 *
 * @return void
 */
void stpmgr_set_path_cost(STP_CLASS* stp_class, PORT_ID port_number, bool auto_config, UINT32 path_cost)
{
    STP_PORT_CLASS* stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

    stp_port_class->path_cost = path_cost;
    stp_port_class->auto_config = auto_config;

    configuration_update(stp_class);
    port_state_selection(stp_class);
}

/* 8.8.7 */
/**
 * @brief Включает обнаружение изменений топологии для указанного порта.
 *
 * Функция активирует механизм обнаружения изменений топологии (Topology Change Detection)
 * для заданного порта. Это позволяет обнаруживать события, такие как подключение
 * или отключение устройств, и инициировать соответствующие действия в рамках STP.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, для которого включается обнаружение изменений.
 *
 * @return void
 */
void stpmgr_enable_change_detection(STP_CLASS* stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS* stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    stp_port_class->change_detection_enabled = true;
}

/* 8.8.8 */
/**
 * @brief Отключает обнаружение изменений топологии для указанного порта.
 *
 * Функция деактивирует механизм обнаружения изменений топологии (Topology Change Detection)
 * для заданного порта. Это предотвращает реагирование STP на изменения состояния порта,
 * такие как подключение или отключение устройств.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, для которого отключается обнаружение изменений.
 *
 * @return void
 */
void stpmgr_disable_change_detection(STP_CLASS* stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS* stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    stp_port_class->change_detection_enabled = false;
}

/* FUNCTION
 *		stpmgr_set_bridge_params()
 *
 * SYNOPSIS
 *		used when user configures an stp bridge parameter to propagate the
 *		values if this is the root bridge.
 */
/**
 * @brief Устанавливает параметры моста для указанного экземпляра STP.
 *
 * Функция обновляет параметры моста, такие как идентификатор моста, возраст сообщений,
 * приветственный интервал и задержку пересылки, в структуре `STP_CLASS`.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 *
 * @return void
 */
void stpmgr_set_bridge_params(STP_CLASS* stp_class)
{
    if (root_bridge(stp_class))
    {
        stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
        stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
        stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT);
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT);
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT);
    }
}

/* FUNCTION
 *		stpmgr_config_bridge_priority()
 *
 * SYNOPSIS
 *		sets the bridge priority.
 */
/**
 * @brief Конфигурирует приоритет моста для указанного экземпляра STP.
 *
 * Функция обновляет приоритет моста (Bridge Priority) для экземпляра STP, заданного индексом.
 * Это значение используется для выбора корневого моста в процессе работы протокола STP.
 *
 * @param stp_index Индекс экземпляра STP, для которого необходимо установить приоритет.
 * @param priority Новое значение приоритета моста (меньшее значение имеет более высокий приоритет).
 *
 * @return `true` в случае успешной конфигурации, `false` в случае ошибки.
 */
bool stpmgr_config_bridge_priority(STP_INDEX stp_index, UINT16 priority)
{
    STP_CLASS* stp_class;
    BRIDGE_IDENTIFIER bridge_id;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);
    bridge_id = stp_class->bridge_info.bridge_id;

    if (stputil_get_bridge_priority(&bridge_id) != priority)
    {
        stputil_set_bridge_priority(&bridge_id, priority, stp_class->vlan_id);

        if (stp_class->state == STP_CLASS_ACTIVE)
        {
            stpmgr_set_bridge_priority(stp_class, &bridge_id);
            /* Sync to APP DB */
            SET_ALL_BITS(stp_class->bridge_info.modified_fields);
            SET_ALL_BITS(stp_class->modified_fields);
        }
        else
        {
            stp_class->bridge_info.bridge_id = bridge_id;
            stp_class->bridge_info.root_id = bridge_id;
            SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT);
            SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT);
        }
    }

    return true;
}

/* FUNCTION
 *		stpmgr_config_bridge_max_age()
 *
 * SYNOPSIS
 *		sets the bridge max-age.
 */
/**
 * @brief Конфигурирует максимальный возраст сообщения (Max Age) для указанного экземпляра STP.
 *
 * Функция устанавливает значение максимального возраста сообщения для указанного экземпляра STP.
 * Этот параметр определяет, как долго мост будет считать BPDU актуальным.
 *
 * @param stp_index Индекс экземпляра STP, для которого устанавливается максимальный возраст сообщения.
 * @param max_age Новое значение максимального возраста сообщения (в секундах).
 *
 * @return `true` в случае успешной конфигурации, `false` в случае ошибки.
 */
bool stpmgr_config_bridge_max_age(STP_INDEX stp_index, UINT16 max_age)
{
    STP_CLASS* stp_class;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);

    if (max_age && stp_class->bridge_info.bridge_max_age != max_age)
    {
        stp_class->bridge_info.bridge_max_age = (UINT8)max_age;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_MAX_AGE_BIT);
        stpmgr_set_bridge_params(stp_class);
    }
    return true;
}

/* FUNCTION
 *		stpmgr_config_bridge_hello_time()
 *
 * SYNOPSIS
 *		sets the bridge hello-time.
 */
/**
 * @brief Конфигурирует время приветствия (Hello Time) для указанного экземпляра STP.
 *
 * Функция устанавливает значение времени приветствия для экземпляра STP. Этот параметр
 * определяет интервал времени, через который мост отправляет BPDU, если он является корневым.
 *
 * @param stp_index Индекс экземпляра STP, для которого устанавливается время приветствия.
 * @param hello_time Новое значение времени приветствия (в секундах). Обычно от 1 до 10 секунд.
 *
 * @return `true` в случае успешной конфигурации, `false` в случае ошибки.
 */
bool stpmgr_config_bridge_hello_time(STP_INDEX stp_index, UINT16 hello_time)
{
    STP_CLASS* stp_class;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);

    if (hello_time && stp_class->bridge_info.bridge_hello_time != hello_time)
    {
        stp_class->bridge_info.bridge_hello_time = (UINT8)hello_time;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_HELLO_TIME_BIT);
        stpmgr_set_bridge_params(stp_class);
    }
    return true;
}

/* FUNCTION
 *		stpmgr_config_bridge_forward_delay()
 *
 * SYNOPSIS
 *		sets the bridge forward-delay.
 */
/**
 * @brief Конфигурирует задержку пересылки (Forward Delay) для указанного экземпляра STP.
 *
 * Функция устанавливает значение задержки пересылки для экземпляра STP. Этот параметр
 * определяет время, в течение которого порт остаётся в состояниях Listening и Learning
 * перед переходом в Forwarding.
 *
 * @param stp_index Индекс экземпляра STP, для которого устанавливается задержка пересылки.
 * @param fwd_delay Новое значение задержки пересылки (в секундах). Обычно от 4 до 30 секунд.
 *
 * @return `true` в случае успешной конфигурации, `false` в случае ошибки.
 */
bool stpmgr_config_bridge_forward_delay(STP_INDEX stp_index, UINT16 fwd_delay)
{
    STP_CLASS* stp_class;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);

    if (fwd_delay && stp_class->bridge_info.bridge_forward_delay != fwd_delay)
    {
        stp_class->bridge_info.bridge_forward_delay = (UINT8)fwd_delay;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_FWD_DELAY_BIT);
        stpmgr_set_bridge_params(stp_class);
    }

    return true;
}

/* FUNCTION
 *		stpmgr_config_port_priority()
 *
 * SYNOPSIS
 *		sets the port priority.
 */
/**
 * @brief Конфигурирует приоритет порта для указанного экземпляра STP.
 *
 * Функция устанавливает значение приоритета порта (Port Priority) для указанного экземпляра STP.
 * Приоритет используется для определения роли порта в процессе выбора топологии STP.
 *
 * @param stp_index Индекс экземпляра STP, для которого задаётся приоритет порта.
 * @param port_number Идентификатор порта, для которого устанавливается приоритет.
 * @param priority Новое значение приоритета порта (0-65535, меньшее значение имеет более высокий приоритет).
 * @param is_global Флаг, определяющий, применяется ли конфигурация ко всем экземплярам:
 *                  - `true` — установить приоритет глобально для всех экземпляров.
 *                  - `false` — применить только к указанному экземпляру STP.
 *
 * @return `true` в случае успешной конфигурации, `false` в случае ошибки.
 */
bool stpmgr_config_port_priority(STP_INDEX stp_index, PORT_ID port_number, UINT16 priority, bool is_global)
{
    STP_CLASS* stp_class;
    STP_PORT_CLASS* stp_port;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);
    if (!is_member(stp_class->control_mask, port_number))
        return false;

    stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
    if (is_global)
    {
        /* If per vlan attributes are set, ignore global attributes */
        if (IS_STP_PER_VLAN_FLAG_SET(stp_port, STP_CLASS_PORT_PRI_FLAG))
            return true;
    }
    else
    {
        if (priority == stp_intf_get_port_priority(port_number))
            CLR_STP_PER_VLAN_FLAG(stp_port, STP_CLASS_PORT_PRI_FLAG);
        else
            SET_STP_PER_VLAN_FLAG(stp_port, STP_CLASS_PORT_PRI_FLAG);
    }

    if (stp_class->state == STP_CLASS_ACTIVE)
    {
        stpmgr_set_port_priority(stp_class, port_number, priority);
    }
    else
    {
        stp_port->port_id.priority = priority >> 4;
    }
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PORT_PRIORITY_BIT);
    return true;
}

/* FUNCTION
 *		stpmgr_config_port_path_cost()
 *
 * SYNOPSIS
 *		sets the ports path cost.
 */

/**
 * @brief Конфигурирует стоимость пути (Path Cost) для указанного порта STP.
 *
 * Функция устанавливает стоимость пути (Path Cost) для порта, используемую в алгоритме STP
 * для выбора оптимального пути. Стоимость может быть задана вручную или автоматически рассчитана.
 *
 * @param stp_index Индекс экземпляра STP, для которого задаётся стоимость пути (игнорируется, если `is_global` равен `true`).
 * @param port_number Идентификатор порта, для которого устанавливается стоимость пути.
 * @param auto_config Флаг автоматической настройки:
 *                    - `true` — стоимость пути рассчитывается автоматически.
 *                    - `false` — стоимость пути задаётся вручную.
 * @param path_cost Стоимость пути, если `auto_config` равен `false`.
 * @param is_global Флаг глобальной конфигурации:
 *                  - `true` — применить конфигурацию ко всем экземплярам STP.
 *                  - `false` — применить только к указанному экземпляру STP.
 *
 * @return `true` в случае успешной конфигурации, `false` в случае ошибки.
 */
bool stpmgr_config_port_path_cost(STP_INDEX stp_index, PORT_ID port_number, bool auto_config,
                                  UINT32 path_cost, bool is_global)
{
    STP_CLASS* stp_class;
    STP_PORT_CLASS* stp_port;
    UINT32 def_path_cost;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);
    if (!is_member(stp_class->control_mask, port_number))
        return false;

    stp_port = GET_STP_PORT_CLASS(stp_class, port_number);

    def_path_cost = stp_intf_get_path_cost(port_number);
    if (is_global)
    {
        /* If per vlan attributes are set, ignore global attributes */
        if (IS_STP_PER_VLAN_FLAG_SET(stp_port, STP_CLASS_PATH_COST_FLAG))
            return true;
    }
    else
    {
        if (path_cost == def_path_cost)
            CLR_STP_PER_VLAN_FLAG(stp_port, STP_CLASS_PATH_COST_FLAG);
        else
            SET_STP_PER_VLAN_FLAG(stp_port, STP_CLASS_PATH_COST_FLAG);
    }

    if (auto_config)
    {
        path_cost = def_path_cost;
    }

    if (stp_class->state == STP_CLASS_ACTIVE)
    {
        stpmgr_set_path_cost(stp_class, port_number, auto_config, path_cost);
    }
    else
    {
        stp_port->path_cost = path_cost;
        stp_port->auto_config = auto_config;
    }
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PATH_COST_BIT);

    return true;
}

/*****************************************************************************/
/* stpmgr_clear_port_statistics: clears the bpdu statistics associated with  */
/* input port.                                                               */
/*****************************************************************************/
/**
 * @brief Очищает статистику порта для указанного экземпляра STP.
 *
 * Функция сбрасывает все собранные статистические данные для указанного порта в рамках
 * заданного экземпляра STP. Это может включать такие показатели, как количество
 * полученных и отправленных BPDU, изменения состояния порта и другие метрики.
 *
 * @param stp_class Указатель на структуру `STP_CLASS`, представляющую экземпляр STP.
 * @param port_number Идентификатор порта, статистика которого очищается.
 *
 * @return void
 */
static void stpmgr_clear_port_statistics(STP_CLASS* stp_class, PORT_ID port_number)
{
    STP_PORT_CLASS* stp_port = NULL;

    if (port_number == BAD_PORT_ID)
    {
        /* Clear stats for all interface */
        port_number = port_mask_get_first_port(stp_class->control_mask);
        while (port_number != BAD_PORT_ID)
        {
            stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
            if (stp_port != NULL)
            {
                stp_port->rx_config_bpdu =
                    stp_port->rx_tcn_bpdu =
                        stp_port->tx_config_bpdu =
                            stp_port->tx_tcn_bpdu = 0;
            }
            SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_CLEAR_STATS_BIT);
            stputil_sync_port_counters(stp_class, stp_port);
            port_number = port_mask_get_next_port(stp_class->control_mask, port_number);
        }
    }
    else
    {
        stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
        if (stp_port != NULL)
        {
            stp_port->rx_config_bpdu =
                stp_port->rx_tcn_bpdu =
                    stp_port->tx_config_bpdu =
                        stp_port->tx_tcn_bpdu = 0;
            SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_CLEAR_STATS_BIT);
            stputil_sync_port_counters(stp_class, stp_port);
        }
    }
}

/*****************************************************************************/
/* stpmgr_clear_statistics: clears the bpdu statistics associated with all   */
/* the ports in input mask.                                                  */
/*****************************************************************************/
/**
 * @brief Очищает статистику для указанного VLAN и/или порта.
 *
 * Функция сбрасывает статистику, связанную с указанным VLAN и портом. Если порт
 * не указан (значение `port_number` равно `BAD_PORT_ID`), статистика очищается
 * для всех портов внутри указанного VLAN.
 *
 * @param vlan_id Идентификатор VLAN, статистика которого очищается.
 * @param port_number Идентификатор порта, для которого очищается статистика.
 *                    Если `BAD_PORT_ID`, статистика очищается для всех портов VLAN.
 *
 * @return void
 */
void stpmgr_clear_statistics(VLAN_ID vlan_id, PORT_ID port_number)
{
    STP_INDEX stp_index;
    STP_CLASS* stp_class;

    if (vlan_id == VLAN_ID_INVALID)
    {
        for (stp_index = 0; stp_index < g_stp_instances; stp_index++)
        {
            stp_class = GET_STP_CLASS(stp_index);
            if (stp_class->state == STP_CLASS_FREE)
                continue;
            stpmgr_clear_port_statistics(stp_class, port_number);
        }
    }
    else
    {
        if (stputil_get_index_from_vlan(vlan_id, &stp_index) == true)
        {
            stp_class = GET_STP_CLASS(stp_index);
            if (stp_class->state != STP_CLASS_FREE)
                stpmgr_clear_port_statistics(stp_class, port_number);
        }
    }
}

/* FUNCTION
 *		stpmgr_release_index()
 *
 * SYNOPSIS
 *		releases the stp index
 */
/**
 * @brief Освобождает указанный индекс STP экземпляра.
 *
 * Функция удаляет привязку индекса STP экземпляра и освобождает связанные ресурсы,
 * делая индекс доступным для повторного использования.
 *
 * @param stp_index Индекс STP экземпляра, который необходимо освободить.
 *
 * @return `true`, если индекс успешно освобождён, `false` в случае ошибки.
 */
bool stpmgr_release_index(STP_INDEX stp_index)
{
    STP_CLASS* stp_class;
    PORT_ID port_number;

    if (stp_index == STP_INDEX_INVALID)
        return false;

    stp_class = GET_STP_CLASS(stp_index);
    if (stp_class->state == STP_CLASS_FREE)
        return true; // already released

#if 0
	// unset sstp for control vlan
	if (stp_class->vlan_id == CONTROL_VLAN)
	{
		g_sstp_enabled = false;
	}
#endif

    clear_mask(stp_class->enable_mask);
    stpmgr_deactivate_stp_class(stp_class);

    port_number = port_mask_get_first_port(stp_class->control_mask);
    while (port_number != BAD_PORT_ID)
    {
        stpmgr_delete_control_port(stp_index, port_number, true);
        port_number = port_mask_get_next_port(stp_class->control_mask, port_number);
    }

    stpsync_del_vlan_from_instance(stp_class->vlan_id, stp_index);
    stpsync_del_stp_class(stp_class->vlan_id);

    stpdata_class_free(stp_index);

    return true;
}

/* FUNCTION
 *		stpmgr_add_control_port()
 *
 * SYNOPSIS
 *		adds the port mask to the list of stp ports controlled by the
 *		stp instance
 */
/**
 * @brief Добавляет порт управления (control port) к указанному экземпляру STP.
 *
 * Функция добавляет порт управления к экземпляру STP с заданным индексом. Порт
 * управления используется для обработки событий STP, таких как отправка или
 * получение BPDU, а также управления состояниями порта.
 *
 * @param stp_index Индекс экземпляра STP, к которому добавляется порт управления.
 * @param port_number Идентификатор порта, который будет добавлен как порт управления.
 * @param mode Режим работы порта управления (определяет поведение порта в рамках STP).
 *
 * @return `true` в случае успешного добавления порта, `false` в случае ошибки.
 */
bool stpmgr_add_control_port(STP_INDEX stp_index, PORT_ID port_number, uint8_t mode)
{
    STP_CLASS* stp_class;
    STP_PORT_CLASS* stp_port_class;

    STP_LOG_DEBUG("add_control_port inst %d port %d", stp_index, port_number);
    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);
    if (stp_class->state == STP_CLASS_FREE)
    {
        return false;
    }

    // filter out ports that are already part of the control mask
    if (is_member(stp_class->control_mask, port_number))
        return true;

    set_mask_bit(stp_class->control_mask, port_number);

    if (mode == 0) // UnTagged mode //если сюда попали с мод0 то будет циско бпду
        set_mask_bit(stp_class->untag_mask, port_number);

    stpmgr_initialize_control_port(stp_class, port_number);

    stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    if (stp_intf_is_port_up(port_number))
    {
        stpmgr_add_enable_port(stp_index, port_number);
    }
    else
    {
        stputil_set_port_state(stp_class, stp_port_class);
    }

    if (stp_port_class)
    {
        SET_ALL_BITS(stp_port_class->modified_fields);
    }

    return true;
}

/* FUNCTION
 *		stpmgr_delete_control_port()
 *
 * SYNOPSIS
 *		removes the port mask from the list of stp ports controlled by the
 *		stp instance
 */
/**
 * @brief Удаляет порт управления (control port) из указанного экземпляра STP.
 *
 * Функция удаляет порт управления из экземпляра STP с заданным индексом. При необходимости
 * может также удалить порт из STP таблицы. Это освобождает ресурсы, связанные с портом,
 * и отключает его участие в обработке протокола STP.
 *
 * @param stp_index Индекс экземпляра STP, из которого удаляется порт управления.
 * @param port_number Идентификатор порта, который будет удалён.
 * @param del_stp_port Указывает, нужно ли удалить порт из STP таблицы:
 *                     - `true` — порт будет удалён из STP таблицы.
 *                     - `false` — порт будет исключён только из управления.
 *
 * @return `true` в случае успешного удаления порта, `false` в случае ошибки.
 */
bool stpmgr_delete_control_port(STP_INDEX stp_index, PORT_ID port_number, bool del_stp_port)
{
    STP_CLASS* stp_class;
    STP_PORT_CLASS* stp_port;
    char* if_name;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);
    if (stp_class->state == STP_CLASS_FREE)
    {
        return false;
    }

    if (!is_member(stp_class->control_mask, port_number))
    {
        return false;
    }

    stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
    /* Reset state to forwarding before deletion */
    stp_port->state = FORWARDING;
    stputil_set_kernel_bridge_port_state(stp_class, stp_port);
    if (!del_stp_port)
        stpsync_update_port_state(GET_STP_PORT_IFNAME(stp_port), GET_STP_INDEX(stp_class), stp_port->state);

    stpmgr_delete_enable_port(stp_index, port_number);

    if_name = stp_intf_get_port_name(port_number);
    if (if_name)
    {
        /* When STP is disabled on port, we still need the STP port to be active in Orchagent and SAI
         * so that port state is in forwarding, deletion will result in removal in SAI which in turn will
         * set the STP state to disabled
         * */
        if (del_stp_port)
            stpsync_del_port_state(if_name, stp_index);
        stpsync_del_port_class(if_name, stp_class->vlan_id);
    }

    clear_mask_bit(stp_class->control_mask, port_number);
    clear_mask_bit(stp_class->untag_mask, port_number);

    return true;
}

/* FUNCTION
 *		stpmgr_add_enable_port()
 *
 * SYNOPSIS
 *		adds the port to the list of enabled stp ports
 */
/**
 * @brief Добавляет и активирует порт для указанного экземпляра STP.
 *
 * Функция добавляет порт в список активных портов для экземпляра STP, заданного индексом.
 * Этот порт начинает участвовать в обработке STP, включая отправку и получение BPDU,
 * управление состоянием и участие в алгоритме выбора корневого моста.
 *
 * @param stp_index Индекс экземпляра STP, к которому добавляется порт.
 * @param port_number Идентификатор порта, который добавляется и активируется.
 *
 * @return `true` в случае успешного добавления порта, `false` в случае ошибки.
 */
bool stpmgr_add_enable_port(STP_INDEX stp_index, PORT_ID port_number)
{
    STP_CLASS* stp_class;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);
    if (is_member(stp_class->enable_mask, port_number))
        return true;

    // check if the port is a member of the control mask before enabling
    if (!is_member(stp_class->control_mask, port_number))
    {
        STP_LOG_ERR("port %d not part of control mask (stp_index %u)",
                    port_number, stp_index);
        return false;
    }

    if (stp_class->state == STP_CLASS_CONFIG)
    {
        stpmgr_activate_stp_class(stp_class);
    }

    stpmgr_enable_port(stp_class, port_number);

    return true;
}

/* FUNCTION
 *		stpmgr_delete_enable_port()
 *
 * SYNOPSIS
 *		removes the port from the list of enabled stp ports
 */
/**
 * @brief Удаляет и деактивирует порт из указанного экземпляра STP.
 *
 * Функция удаляет порт из списка активных портов для экземпляра STP, заданного индексом.
 * Этот порт прекращает участие в обработке STP, включая отправку и получение BPDU,
 * управление состоянием и участие в алгоритме выбора корневого моста.
 *
 * @param stp_index Индекс экземпляра STP, из которого удаляется порт.
 * @param port_number Идентификатор порта, который будет удалён и деактивирован.
 *
 * @return `true` в случае успешного удаления порта, `false` в случае ошибки.
 */
bool stpmgr_delete_enable_port(STP_INDEX stp_index, PORT_ID port_number)
{
    STP_CLASS* stp_class;

    if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
        return false;
    }

    stp_class = GET_STP_CLASS(stp_index);
    if (!is_member(stp_class->enable_mask, port_number))
        return true;

    stpmgr_disable_port(stp_class, port_number);
    if (is_mask_clear(stp_class->enable_mask))
    {
        stpmgr_deactivate_stp_class(stp_class);
    }
    return true;
}

/* FUNCTION
 *		stpmgr_update_stats()
 *
 * SYNOPSIS
 *		updates stp port statistics
 */
/**
 * @brief Обновляет статистику для указанного порта в заданном экземпляре STP.
 *
 * Функция анализирует содержимое буфера BPDU и обновляет статистические данные
 * для указанного порта в заданном экземпляре STP. Также учитывает, является ли
 * обработка BPDU специфичной для протокола PVST.
 *
 * @param stp_index Индекс экземпляра STP, статистика которого обновляется.
 * @param port_number Идентификатор порта, для которого обновляется статистика.
 * @param buffer Указатель на буфер с данными BPDU.
 * @param pvst Флаг, указывающий, обрабатывается ли PVST BPDU:
 *             - `true` — BPDU относится к протоколу PVST.
 *             - `false` — стандартный BPDU.
 *
 * @return void
 */
static void stpmgr_update_stats(STP_INDEX stp_index, PORT_ID port_number, void* buffer, bool pvst)
{
    STP_CLASS* stp_class;
    STP_PORT_CLASS* stp_port;
    STP_CONFIG_BPDU* bpdu;
    UINT8* typestring = NULL;

    stp_class = GET_STP_CLASS(stp_index);
    stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
    bpdu = (STP_CONFIG_BPDU*)buffer;

    switch (bpdu->type)
    {
    case RSTP_BPDU_TYPE:
    case CONFIG_BPDU_TYPE:
        stp_port->rx_config_bpdu++;
        break;

    case TCN_BPDU_TYPE:
        stp_port->rx_tcn_bpdu++;
        break;

    default:
        stp_port->rx_drop_bpdu++;
        STP_LOG_ERR("error - stpmgr_update_stats() - unknown bpdu type %u", bpdu->type);
        return;
    }
}

/* FUNCTION
 *		stpmgr_process_pvst_bpdu()
 *
 * SYNOPSIS
 *		validates the pvst bpdu and passes to the stp bpdu handler process
 */
/**
 * @brief Обрабатывает PVST BPDU для указанного порта в экземпляре STP.
 *
 * Функция анализирует содержимое буфера PVST BPDU, обновляет соответствующую
 * статистику и выполняет необходимые действия для обработки BPDU в рамках
 * протокола PVST.
 *
 * @param stp_index Индекс экземпляра STP, к которому относится BPDU.
 * @param port_number Идентификатор порта, с которого получен BPDU.
 * @param buffer Указатель на буфер с данными PVST BPDU.
 *
 * @return void
 */
void stpmgr_process_pvst_bpdu(STP_INDEX stp_index, PORT_ID port_number, void* buffer)
{
    STP_CONFIG_BPDU* bpdu;
    STP_CLASS* stp_class;

    stp_class = GET_STP_CLASS(stp_index);
    if (!is_member(stp_class->enable_mask, port_number))
    {
        if (STP_DEBUG_BPDU_RX(stp_class->vlan_id, port_number))
        {
            STP_PKTLOG("Dropping PVST BPDU, Port:%d not in Vlan:%d enable mask", port_number, stp_class->vlan_id);
        }
        stp_class->rx_drop_bpdu++;
        return;
    }

    // hack the pointer to make it appear as a config bpdu so that the
    // rest of the routines can be called without any problems.
    bpdu = (STP_CONFIG_BPDU*)(((UINT8*)buffer) + 5);

    stputil_decode_bpdu(bpdu);
    stpmgr_update_stats(stp_index, port_number, bpdu, true /* pvst */);
    stputil_process_bpdu(stp_index, port_number, (void*)bpdu);
}

/* FUNCTION
 *		stpmgr_process_stp_bpdu()
 *
 * SYNOPSIS
 *		passes it to the protocol bpdu handler based on the type of
 *		bpdu received. note that the bpdu should have been validated
 *		before this function is called.
 */
/**
 * @brief Обрабатывает стандартный STP BPDU для указанного порта в экземпляре STP.
 *
 * Функция анализирует содержимое буфера STP BPDU, обновляет соответствующую
 * статистику и выполняет необходимые действия для обработки BPDU в рамках
 * стандартного протокола STP.
 *
 * @param stp_index Индекс экземпляра STP, к которому относится BPDU.
 * @param port_number Идентификатор порта, с которого получен BPDU.
 * @param buffer Указатель на буфер с данными STP BPDU.
 *
 * @return void
 */
void stpmgr_process_stp_bpdu(STP_INDEX stp_index, PORT_ID port_number, void* buffer)
{
    STP_CONFIG_BPDU* bpdu = (STP_CONFIG_BPDU*)buffer;
    STP_CLASS* stp_class = GET_STP_CLASS(stp_index);

    if (!is_member(stp_class->enable_mask, port_number))
    {
        if (STP_DEBUG_BPDU_RX(stp_class->vlan_id, port_number))
        {
            STP_PKTLOG("Dropping BPDU, Port:%d not in Vlan:%d enable mask", port_number, stp_class->vlan_id);
        }
        return;
    }

    stputil_decode_bpdu(bpdu);
    stpmgr_update_stats(stp_index, port_number, bpdu, false /* pvst */);
    stputil_process_bpdu(stp_index, port_number, buffer);
}

/* FUNCTION
 *		stpmgr_config_fastuplink()
 *
 * SYNOPSIS
 *		enables/disables fast uplink for all the ports in the port mask
 */
/**
 * @brief Конфигурирует функцию Fast Uplink для указанного порта.
 *
 * Функция включает или отключает функцию Fast Uplink на указанном порту. Эта функция
 * позволяет быстро переключаться на резервный линк при изменении топологии сети,
 * минимизируя время простоя.
 *
 * @param port_number Уникальный идентификатор порта, который необходимо настроить.
 * @param enable Логический флаг для включения/отключения функции:
 *               - `true` — включить Fast Uplink.
 *               - `false` — отключить Fast Uplink.
 *
 * @return void
 */
void stpmgr_config_fastuplink(PORT_ID port_number, bool enable)
{
    if (enable)
    {
        if (STP_IS_FASTUPLINK_CONFIGURED(port_number))
            return;

        set_mask_bit(g_fastuplink_mask, port_number);
    }
    else
    {
        if (!STP_IS_FASTUPLINK_CONFIGURED(port_number))
            return;

        clear_mask_bit(g_fastuplink_mask, port_number);
    }
}

/* FUNCTION
 *		stpmgr_protect_process()
 *
 * SYNOPSIS
 *		If "spanning protect" is configured, take the proper action.
 *
 * RETURN VALUE
 *		true if it is "spanning protect"; false otherwise.
 */

/**
 * @brief Обрабатывает защиту порта от нежелательных изменений топологии.
 *
 * Функция проверяет наличие условий, которые могут вызвать нарушение работы
 * протокола STP на указанном порту и VLAN, и применяет соответствующие меры
 * защиты, если это необходимо.
 *
 * @param rx_port Идентификатор порта, на котором был обнаружен потенциальный конфликт.
 * @param vlan_id Идентификатор VLAN, в контексте которого выполняется проверка.
 *
 * @return `true`, если защита успешно применена, `false` в случае отсутствия нарушений или ошибки.
 */
static bool stpmgr_protect_process(PORT_ID rx_port, uint16_t vlan_id)
{

    if (!STP_IS_PROTECT_CONFIGURED(rx_port) && !STP_IS_PROTECT_DO_DISABLE_CONFIGURED(rx_port))
        return (false);

    if (STP_IS_PROTECT_DO_DISABLE_CONFIGURED(rx_port))
    {
        // If already disabled
        if (STP_IS_PROTECT_DO_DISABLED(rx_port))
            return (true);

        // Update protect_disabled_mask
        set_mask_bit(stp_global.protect_disabled_mask, rx_port);

        // log message
        STP_SYSLOG("STP: BPDU(%u) received, interface %s disabled due to BPDU guard trigger", vlan_id, stp_intf_get_port_name(rx_port));

        // Disable rx_port
        stpsync_update_bpdu_guard_shutdown(stp_intf_get_port_name(rx_port), true);
        stpsync_update_port_admin_state(stp_intf_get_port_name(rx_port), false, STP_IS_ETH_PORT_ID(rx_port));
    }

    return (true);
}

/* FUNCTION
 *		stpmgr_config_fastspan()
 *
 * SYNOPSIS
 *		enables/disables stp port fast.
 *
 * NOTES
 *
 */
/**
 * @brief Конфигурирует функцию Fast Span для указанного порта.
 *
 * Функция включает или отключает функцию Fast Span для указанного порта. Эта функция
 * позволяет ускорить процесс согласования состояния порта в протоколе STP, минимизируя
 * задержки при изменении топологии сети.
 *
 * @param port_id Уникальный идентификатор порта, который необходимо настроить.
 * @param enable Логический флаг для включения/отключения функции:
 *               - `true` — включить Fast Span.
 *               - `false` — отключить Fast Span.
 *
 * @return `true`, если конфигурация была успешно применена, `false` в случае ошибки.
 */
static bool stpmgr_config_fastspan(PORT_ID port_id, bool enable)
{
    bool ret = true;

    if (enable)
    {
        if (is_member(g_fastspan_config_mask, port_id))
            return ret;
        set_mask_bit(g_fastspan_config_mask, port_id);
        set_mask_bit(g_fastspan_mask, port_id);
        stpsync_update_port_fast(stp_intf_get_port_name(port_id), true);
    }
    else
    {
        if (!is_member(g_fastspan_config_mask, port_id))
            return ret;
        clear_mask_bit(g_fastspan_config_mask, port_id);
        clear_mask_bit(g_fastspan_mask, port_id);
        stpsync_update_port_fast(stp_intf_get_port_name(port_id), false);
    }
    return ret;
}

/* FUNCTION
 *		stpmgr_config_protect()
 *
 * SYNOPSIS
 *		enables/disables stp bpdu protection on the input mask.
 *
 * NOTES
 *		The protection mask is (protect_mask || protect_do_disable_mask).
 */

/**
 * @brief Конфигурирует защиту порта (Protection) для указанного порта.
 *
 * Функция включает или отключает защиту порта, такую как BPDU Guard или Root Guard.
 * При необходимости также может отключить порт при обнаружении нарушения.
 *
 * @param port_id Уникальный идентификатор порта, который необходимо настроить.
 * @param enable Логический флаг для включения/отключения защиты:
 *               - `true` — включить защиту.
 *               - `false` — отключить защиту.
 * @param do_disable Логический флаг, указывающий, следует ли отключить порт при обнаружении нарушения:
 *                   - `true` — порт будет отключён в случае нарушения.
 *                   - `false` — порт останется активным, но нарушение будет зафиксировано.
 *
 * @return `true`, если конфигурация защиты была успешно применена, `false` в случае ошибки.
 */
static bool stpmgr_config_protect(PORT_ID port_id, bool enable, bool do_disable)
{
    bool ret = true;

    if (enable)
    {
        if (do_disable)
            set_mask_bit(stp_global.protect_do_disable_mask, port_id);
        else
            clear_mask_bit(stp_global.protect_do_disable_mask, port_id);

        set_mask_bit(stp_global.protect_mask, port_id);
    }
    else
    {
        clear_mask_bit(stp_global.protect_do_disable_mask, port_id);

        if (STP_IS_PROTECT_DO_DISABLED(port_id))
        {
            clear_mask_bit(stp_global.protect_disabled_mask, port_id);
            stpsync_update_bpdu_guard_shutdown(stp_intf_get_port_name(port_id), false);
        }

        clear_mask_bit(stp_global.protect_mask, port_id);
    }

    return ret;
}

/*****************************************************************************/
/* stpmgr_config_root_protect: enables/disables root-guard feature on the    */
/* input mask.                                                               */
/*****************************************************************************/
/**
 * @brief Конфигурирует функцию Root Protect для указанного порта.
 *
 * Функция включает или отключает функцию Root Protect, которая предотвращает
 * назначение порта корневым портом, даже если он получает BPDU с более высоким
 * приоритетом. Эта функция используется для защиты текущего корневого моста
 * от нежелательных изменений топологии.
 *
 * @param port_id Уникальный идентификатор порта, который необходимо настроить.
 * @param enable Логический флаг для включения/отключения функции:
 *               - `true` — включить Root Protect.
 *               - `false` — отключить Root Protect.
 *
 * @return `true`, если конфигурация Root Protect была успешно применена,
 *         `false` в случае ошибки.
 */
static bool stpmgr_config_root_protect(PORT_ID port_id, bool enable)
{
    if (enable)
        set_mask_bit(stp_global.root_protect_mask, port_id);
    else
        clear_mask_bit(stp_global.root_protect_mask, port_id);

    return true;
}

/*****************************************************************************/
/* stpmgr_config_root_protect_timeout: configures the timeout in seconds for */
/* which the violated stp port is kept in blocking state.                    */
/*****************************************************************************/
/**
 * @brief Устанавливает таймаут для функции Root Protect.
 *
 * Функция задаёт таймаут, в течение которого порт остаётся защищённым
 * от нежелательных изменений топологии (например, назначение порта корневым).
 * Таймаут определяет максимальное время действия функции Root Protect.
 *
 * @param timeout Значение таймаута в секундах.
 *
 * @return `true`, если значение таймаута было успешно установлено,
 *         `false` в случае ошибки.
 */
static bool stpmgr_config_root_protect_timeout(UINT timeout)
{
    // sanity check (should never happen)
    if (timeout < STP_MIN_ROOT_PROTECT_TIMEOUT ||
        timeout > STP_MAX_ROOT_PROTECT_TIMEOUT)
    {
        STP_LOG_ERR("input timeout %u not in range", timeout);
        return false;
    }

    stp_global.root_protect_timeout = timeout;
    return true;
}

/* FUNCTION
 *		stpmgr_set_extend_mode()
 *
 * SYNOPSIS
 *		sets the bridge to operate in extend mode (non-legacy mode). in this
 *		mode, bridge instances will operate in the 802.1d-2004 mode rather than
 *		802.1d-1998 mode.
 */
/**
 * @brief Включает или отключает расширенный режим (Extend Mode) для STP.
 *
 * Функция активирует или деактивирует расширенный режим работы STP. Расширенный режим
 * позволяет использовать дополнительные возможности и параметры STP, такие как
 * поддержка нестандартных конфигураций или улучшенных алгоритмов.
 *
 * @param enable Логический флаг для включения/отключения расширенного режима:
 *               - `true` — включить расширенный режим.
 *               - `false` — отключить расширенный режим.
 *
 * @return void
 */
void stpmgr_set_extend_mode(bool enable)
{
    if (enable == g_stpd_extend_mode)
        return;

    g_stpd_extend_mode = enable;
}

/* FUNCTION
 *		stpmgr_port_event()
 *
 * SYNOPSIS
 *		port event handler. propagates port events across all stp and
 *		rstp_instances.
 */
/**
 * @brief Обрабатывает событие изменения состояния порта.
 *
 * Функция вызывается при изменении состояния указанного порта (включение или выключение).
 * В зависимости от текущего состояния порта и входных параметров, обновляет
 * состояние порта в протоколе STP и выполняет связанные операции, такие как
 * активация или деактивация таймеров, обновление топологии и т. д.
 *
 * @param port_number Уникальный идентификатор порта, для которого произошло событие.
 * @param up Логический флаг, указывающий новое состояние порта:
 *           - `true` — порт стал активным (поднят).
 *           - `false` — порт стал неактивным (опущен).
 *
 * @return void
 */
void stpmgr_port_event(PORT_ID port_number, bool up)
{
    STP_INDEX index;
    STP_CLASS* stp_class;
    STP_PORT_CLASS* stp_port;
    UINT32 path_cost;
    bool (*func)(STP_INDEX, PORT_ID);

    STP_LOG_INFO("%d interface event: %s", port_number, (up ? "UP" : "DOWN"));
    // reset auto-config variables.
    if (!up)
    {
        if (!is_member(g_fastspan_mask, port_number) &&
            is_member(g_fastspan_config_mask, port_number))
        {
            stputil_update_mask(g_fastspan_mask, port_number, true);
            stpsync_update_port_fast(stp_intf_get_port_name(port_number), true);
        }
    }

    if (up)
    {
        if (STP_IS_PROTECT_DO_DISABLED(port_number))
        {
            clear_mask_bit(stp_global.protect_disabled_mask, port_number);
            stpsync_update_bpdu_guard_shutdown(stp_intf_get_port_name(port_number), false);
        }
    }

    if (g_stp_active_instances == 0)
        return;

    func = (up) ? stpmgr_add_enable_port : stpmgr_delete_enable_port;
    path_cost = stputil_get_default_path_cost(port_number, g_stpd_extend_mode);
    for (index = 0; index < g_stp_instances; index++)
    {
        stp_class = GET_STP_CLASS(index);
        if ((stp_class->state == STP_CLASS_FREE) ||
            (!is_member(stp_class->control_mask, port_number)))
        {
            continue;
        }

        // to accomodate auto-negotiated port speed, reset path-cost
        stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
        if (stp_port->auto_config)
        {
            stp_port->path_cost = path_cost;
        }
        (*func)(index, port_number);
        SET_ALL_BITS(stp_port->modified_fields);
    }
}

/**
 * @brief Обрабатывает полученный STP BPDU на указанном порту и VLAN.
 *
 * Функция вызывается при получении STP BPDU. Она анализирует содержимое BPDU,
 * обновляет статистику и вызывает обработку в рамках текущего состояния протокола STP
 * для указанного VLAN и порта.
 *
 * @param vlan_id Идентификатор VLAN, в котором получен BPDU.
 * @param port_id Идентификатор порта, на котором получен BPDU.
 * @param pkt Указатель на буфер с данными BPDU.
 *
 * @return void
 */
void stpmgr_rx_stp_bpdu(uint16_t vlan_id, uint32_t port_id, char* pkt)
{
    STP_INDEX stp_index = STP_INDEX_INVALID;
    STP_CONFIG_BPDU* bpdu = NULL;
    bool flag = true;

    // check for stp protect configuration.
    if (stpmgr_protect_process(port_id, vlan_id))
    {
        return;
    }

    bpdu = (STP_CONFIG_BPDU*)pkt;

    // validate bpdu
    if (!stputil_validate_bpdu(bpdu))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
        {
            STP_PKTLOG("Invalid STP BPDU received on Vlan:%d Port:%d - dropping",
                       vlan_id, port_id);
        }
        stp_global.stp_drop_count++;
        return;
    }

    // untagged ieee 802.1d bpdus processing
    if (stputil_is_port_untag(vlan_id, port_id))
    {
        vlan_id = 1;

        // 3 - if STP untagged BPDU is received and STP or RSTP is enabled on native vlan,
        // STP BPDU gets processed by STP or RSTP respectively,else will be processed by MSTP
        if (stputil_is_protocol_enabled(L2_PVSTP) && (bpdu->protocol_version_id == STP_VERSION_ID))
        {
            flag = stputil_get_index_from_vlan(vlan_id, &stp_index);
        }
    }
    else // Tagged BPDU Processing
    {
        if (stputil_is_protocol_enabled(L2_PVSTP))
            flag = stputil_get_index_from_vlan(vlan_id, &stp_index);
    }

    // rstp/stp processing
    if (!flag)
    {
        if (bpdu->protocol_version_id == STP_VERSION_ID)
        {
            if (bpdu->type == TCN_BPDU_TYPE)
                stp_global.tcn_drop_count++;
            else if (bpdu->type == CONFIG_BPDU_TYPE)
                stp_global.stp_drop_count++;
        }

        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
            STP_PKTLOG("dropping bpdu received on vlan %u, port %d", vlan_id, port_id);

        return;
    }

    if (stp_index != STP_INDEX_INVALID)
    {
        // ieee 802.1d 9.3.4 validation of bpdus.
        if (bpdu->type != TCN_BPDU_TYPE &&
            ntohs(bpdu->message_age) >= ntohs(bpdu->max_age))
        {
            STP_LOG_INFO("Invalid BPDU (message age %u exceeds max age %u)",
                         ntohs(bpdu->message_age), ntohs(bpdu->max_age));
        }
        else
        {
            stpmgr_process_stp_bpdu(stp_index, port_id, bpdu);
        }
    }
    else
    {
        if (bpdu->protocol_version_id == STP_VERSION_ID)
        {
            if (bpdu->type == TCN_BPDU_TYPE)
                stp_global.tcn_drop_count++;
            else if (bpdu->type == CONFIG_BPDU_TYPE)
                stp_global.stp_drop_count++;
        }

        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
            STP_PKTLOG("dropping bpdu - stp not configured Vlan:%d Port:%d", vlan_id, port_id);
    }
}

/**
 * @brief Обрабатывает полученный PVST BPDU на указанном порту и VLAN.
 *
 * Функция вызывается при получении BPDU протокола PVST (Per-VLAN Spanning Tree).
 * Она анализирует содержимое BPDU, обновляет соответствующую статистику и выполняет
 * обработку на основе состояния протокола PVST для указанного VLAN и порта.
 *
 * @param vlan_id Идентификатор VLAN, в котором был получен BPDU.
 * @param port_id Идентификатор порта, на котором был получен BPDU.
 * @param pkt Указатель на буфер с данными BPDU.
 *
 * @return void
 */
void stpmgr_rx_pvst_bpdu(uint16_t vlan_id, uint32_t port_id, void* pkt)
{
    STP_INDEX stp_index = STP_INDEX_INVALID;
    PVST_CONFIG_BPDU* bpdu = NULL;

    // check for stp protect configuration.
    if (stpmgr_protect_process(port_id, vlan_id))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
        {
            STP_PKTLOG("Dropping pvst bpdu on port:%d with stp protect enabled for Vlan:%d",
                       port_id, vlan_id);
        }
        stp_global.pvst_drop_count++;
        return;
    }

    // validate pvst bpdu
    bpdu = (PVST_CONFIG_BPDU*)(((UINT8*)pkt));
    if (!stputil_validate_pvst_bpdu(bpdu))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
        {
            STP_PKTLOG("Invalid PVST BPDU received Vlan:%d Port:%d - dropping",
                       vlan_id, port_id);
        }
        stp_global.pvst_drop_count++;
        return;
    }

    // drop pvst-bpdus associated with vlan 1. wait for untagged ieee bpdu
    if ((vlan_id == 1) && stputil_is_port_untag(vlan_id, port_id))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
        {
            STP_PKTLOG("Dropping PVST BPDU for VLAN:%d Port:%d", vlan_id, port_id);
        }
        stp_global.pvst_drop_count++;
        return;
    }

    stputil_get_index_from_vlan(vlan_id, &stp_index);

    if (stp_index != STP_INDEX_INVALID)
    {
        // ieee 802.1d 9.3.4 validation of bpdus.
        if (bpdu->type != TCN_BPDU_TYPE &&
            ntohs(bpdu->message_age) >= ntohs(bpdu->max_age))
        {
            STP_LOG_INFO("Invalid BPDU (message age %u exceeds max age %u) vlan %u port %u",
                         ntohs(bpdu->message_age), ntohs(bpdu->max_age), vlan_id, port_id);
            stp_global.pvst_drop_count++;
        }
        else
        {
            stpmgr_process_pvst_bpdu(stp_index, port_id, bpdu);
        }
    }
    else
    {
        stp_global.pvst_drop_count++;
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
            STP_PKTLOG("dropping bpdu - stp/rstp not configured vlan %u port %u", vlan_id, port_id);
    }
}

/**
 * @brief Обрабатывает полученный BPDU (STP или PVST) на указанном порту и VLAN.
 *
 * Функция вызывается при получении BPDU и определяет, к какому протоколу
 * относится BPDU (STP или PVST), а затем вызывает соответствующую функцию
 * для обработки. Также обновляет статистику и выполняет действия, связанные с
 * изменением состояния топологии.
 *
 * @param vlan_id Идентификатор VLAN, в котором был получен BPDU.
 * @param port_id Идентификатор порта, на котором был получен BPDU.
 * @param pkt Указатель на буфер с данными BPDU.
 *
 * @return void
 */
void stpmgr_process_rx_bpdu(uint16_t vlan_id, uint32_t port_id, unsigned char* pkt)
{
    // sanity checks
    if (!IS_VALID_VLAN(vlan_id))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
            STP_PKTLOG("Rx: INVALID VLAN-%u on Port-%u", vlan_id, port_id);
        return;
    }

    // check DA mac
    // PVST := 01 00 0c cc cc cd
    // STP  := 01 80 c2 00 00 00
    if (pkt[1] == 128) // pkt[1] == 0x80
        stpmgr_rx_stp_bpdu(vlan_id, port_id, pkt);
    else
        stpmgr_rx_pvst_bpdu(vlan_id, port_id, pkt);
}

/**
 * @brief Обработчик 100-миллисекундного таймера для STP.
 *
 * Эта функция вызывается каждые 100 миллисекунд для выполнения
 * периодических задач в контексте протокола STP. Основные задачи включают
 * обновление таймеров, управление состояниями портов, обработку изменений
 * топологии и обновление статистики.
 *
 * @param fd Сокетный дескриптор (не используется в данном обработчике).
 * @param what Тип события (не используется в данном обработчике).
 * @param arg Указатель на аргумент, переданный таймеру (обычно структура данных STP).
 *
 * @return void
 */
void stpmgr_100ms_timer(evutil_socket_t fd, short what, void* arg)
{
    const char* data = (char*)arg;
    g_stpd_stats_libev_timer++;
    stptimer_tick();
}

/**
 * @brief Обрабатывает сообщение конфигурации моста (Bridge Config Message).
 *
 * Эта функция отвечает за обработку входящего сообщения конфигурации моста.
 * Сообщение содержит параметры конфигурации, такие как приоритет моста,
 * максимальный возраст, время задержки и другие настройки STP. Функция
 * обновляет соответствующие параметры в контексте STP и выполняет
 * необходимые действия для применения новой конфигурации.
 *
 * @param msg Указатель на сообщение конфигурации моста.
 *
 * @return void
 */
static void stpmgr_process_bridge_config_msg(void* msg)
{
    STP_BRIDGE_CONFIG_MSG* pmsg = (STP_BRIDGE_CONFIG_MSG*)msg;
    int i;
    STP_CLASS* stp_class;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }

    STP_LOG_INFO("opcode : %d, stp_mode:%d, rg_timeout:%d, mac : %x%x:%x%x:%x%x",
                 pmsg->opcode, pmsg->stp_mode, pmsg->rootguard_timeout, pmsg->base_mac_addr[0],
                 pmsg->base_mac_addr[1], pmsg->base_mac_addr[2], pmsg->base_mac_addr[3],
                 pmsg->base_mac_addr[4], pmsg->base_mac_addr[5]);

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        stp_global.enable = true;
        stp_global.proto_mode = pmsg->stp_mode;

        if (pmsg->stp_mode==L2_NONE)
        {
            stp_global.config_bpdu.protocol_version_id = RSTP_BPDU_TYPE; //этот флаг заставляет отказаться от отправки pvst bpdu и отправлять ieee RSTP bpdu
        }
        

        stpmgr_config_root_protect_timeout(pmsg->rootguard_timeout);

        memcpy((char*)&g_stp_base_mac_addr._ulong, pmsg->base_mac_addr, sizeof(g_stp_base_mac_addr._ulong));
        memcpy((char*)&g_stp_base_mac_addr._ushort,
               (char*)(pmsg->base_mac_addr + 4),
               sizeof(g_stp_base_mac_addr._ushort));
    }
    else if (pmsg->opcode == STP_DEL_COMMAND)
    {
        stp_global.enable = false;
        // Release all index

        for (i = 0; i < g_stp_instances; i++)
        {
            stp_class = GET_STP_CLASS(i);
            if (stp_class->state == STP_CLASS_FREE)
                continue;

            stpmgr_release_index(i);
        }

        clear_mask(g_stp_enable_mask);
        stp_intf_reset_port_params();
    }
}

/**
 * @brief Включает поддержку протокола STP для указанного VLAN.
 *
 * Эта функция обрабатывает сообщение конфигурации VLAN, включающее параметры
 * для активации STP. Она проверяет корректность параметров, создаёт или
 * обновляет экземпляр STP для данного VLAN и включает его работу.
 *
 * @param pmsg Указатель на структуру сообщения конфигурации VLAN.
 *             Структура содержит информацию о VLAN, такую как идентификатор VLAN
 *             и параметры конфигурации STP.
 *
 * @return `true`, если STP был успешно активирован для VLAN,
 *         `false`, если произошла ошибка.
 */
static bool stpmgr_vlan_stp_enable(STP_VLAN_CONFIG_MSG* pmsg)
{
    PORT_ATTR* attr;
    int port_count;
    uint32_t port_id;

    static STP_VLAN_CONFIG_MSG* msg_body;
    msg_body=pmsg;

    // STP_LOG_DEBUG("newInst:%d inst_id:%d", pmsg->newInstance, pmsg->inst_id);
    STP_LOG_DEBUG("newInst:%d inst_id:%d", msg_body->newInstance, msg_body->inst_id);

    if (pmsg->newInstance)
    {
        stpdata_init_class(pmsg->inst_id, pmsg->vlan_id);

        stpsync_add_vlan_to_instance(pmsg->vlan_id, pmsg->inst_id);

        attr = pmsg->port_list;
        for (port_count = 0; port_count < pmsg->count; port_count++)
        {
            STP_LOG_INFO("Intf:%s Enab:%d Mode:%d", attr[port_count].intf_name, attr[port_count].enabled, attr[port_count].mode);
            port_id = stp_intf_get_port_id_by_name(attr[port_count].intf_name);

            if (port_id == BAD_PORT_ID)
                continue;

            if (attr[port_count].enabled)
            {
                stpmgr_add_control_port(pmsg->inst_id, port_id, attr[port_count].mode); // Sets control_mask
            }
            else
            {
                /* STP not enabled on this interface. Make it FORWARDING */
                stpsync_update_port_state(attr[port_count].intf_name, pmsg->inst_id, FORWARDING);
            }
        }
    }

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        stpmgr_config_bridge_forward_delay(pmsg->inst_id, pmsg->forward_delay);
        stpmgr_config_bridge_hello_time(pmsg->inst_id, pmsg->hello_time);
        stpmgr_config_bridge_max_age(pmsg->inst_id, pmsg->max_age);
        stpmgr_config_bridge_priority(pmsg->inst_id, pmsg->priority);
    }
    return true;
}

/**
 * @brief Отключает поддержку протокола STP для указанного VLAN.
 *
 * Эта функция обрабатывает сообщение конфигурации VLAN, отключающее STP для данного VLAN.
 * Она удаляет или деактивирует экземпляр STP, связанный с VLAN, и выполняет
 * соответствующую очистку ресурсов.
 *
 * @param pmsg Указатель на структуру сообщения конфигурации VLAN.
 *             Структура содержит информацию о VLAN, такую как идентификатор VLAN
 *             и параметры конфигурации STP.
 *
 * @return `true`, если STP был успешно отключён для VLAN,
 *         `false`, если произошла ошибка.
 */
static bool stpmgr_vlan_stp_disable(STP_VLAN_CONFIG_MSG* pmsg)
{
    stpmgr_release_index(pmsg->inst_id);
    return true;
}

/**
 * @brief Обрабатывает сообщение конфигурации VLAN.
 *
 * Эта функция обрабатывает входящее сообщение конфигурации VLAN, определяя,
 * требуется ли включить или отключить поддержку STP для указанного VLAN.
 * В зависимости от содержимого сообщения выполняется активация или
 * деактивация протокола STP для данного VLAN.
 *
 * @param msg Указатель на сообщение конфигурации VLAN. Ожидается, что
 *            сообщение приведено к структуре `STP_VLAN_CONFIG_MSG`,
 *            содержащей информацию о VLAN и требуемых действиях.
 *
 * @return void
 */
static void stpmgr_process_vlan_config_msg(void* msg)
{
    STP_VLAN_CONFIG_MSG* pmsg = (STP_VLAN_CONFIG_MSG*)msg;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }

    if (pmsg->inst_id > g_stp_instances)
    {
        STP_LOG_ERR("invalid inst_id:%d", pmsg->inst_id);
        return;
    }

    STP_LOG_INFO("op:%d, NewInst:%d, vlan:%d, Inst:%d fwd_del:%d, hello:%d, max_age:%d, pri:%d, count:%d",
                 pmsg->opcode, pmsg->newInstance, pmsg->vlan_id, pmsg->inst_id, pmsg->forward_delay,
                 pmsg->hello_time, pmsg->max_age, pmsg->priority, pmsg->count);

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        stpmgr_vlan_stp_enable(pmsg);
    }
    else if (pmsg->opcode == STP_DEL_COMMAND)
    {
        stpmgr_vlan_stp_disable(pmsg);
    }
    else
        STP_LOG_ERR("invalid opcode %d", pmsg->opcode);
}

/**
 * @brief Отправляет ответное сообщение через UNIX-сокет.
 *
 * Эта функция используется для отправки данных в ответ на запрос через
 * UNIX-сокет. Она принимает адрес клиента, указатель на сообщение и его длину,
 * отправляет сообщение и логирует результат.
 *
 * @param addr Указатель на структуру `sockaddr_un`, содержащую адрес клиента.
 * @param msg Указатель на сообщение, которое нужно отправить.
 * @param len Длина сообщения в байтах.
 *
 * @return void
 */
static void stpmgr_send_reply(struct sockaddr_un addr, void* msg, int len)
{
    int rc;

    STP_LOG_INFO("sending msg to %s", addr.sun_path);
    rc = sendto(g_stpd_ipc_handle, msg, len, 0, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == -1)
        STP_LOG_ERR("reply send error %s", strerror(errno));
    else
        STP_LOG_DEBUG("reply sent");
}

/**
 * @brief Обрабатывает сообщение конфигурации интерфейса VLAN.
 *
 * Эта функция используется для обработки сообщений, содержащих параметры конфигурации
 * интерфейсов VLAN. Она анализирует переданные данные, вносит изменения в текущую
 * конфигурацию и выполняет соответствующие действия для интерфейсов VLAN.
 *
 * @param msg Указатель на сообщение конфигурации интерфейса VLAN.
 *            Ожидается, что сообщение приведено к структуре, содержащей информацию
 *            о VLAN и соответствующем интерфейсе.
 *
 * @return void
 */
static void stpmgr_process_vlan_intf_config_msg(void* msg)
{
    STP_VLAN_PORT_CONFIG_MSG* pmsg = (STP_VLAN_PORT_CONFIG_MSG*)msg;
    uint32_t port_id;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }

    if (pmsg->inst_id > g_stp_instances)
    {
        STP_LOG_ERR("invalid inst_id:%d", pmsg->inst_id);
        return;
    }

    STP_LOG_INFO("op:%d, vlan_id:%d intf:%s, inst_id:%d, cost:%d, pri:%d",
                 pmsg->opcode, pmsg->vlan_id, pmsg->intf_name, pmsg->inst_id, pmsg->path_cost, pmsg->priority);

    port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
    if (port_id == BAD_PORT_ID)
        return;

    if (pmsg->priority != -1)
        stpmgr_config_port_priority(pmsg->inst_id, port_id, pmsg->priority, false);
    if (pmsg->path_cost)
        stpmgr_config_port_path_cost(pmsg->inst_id, port_id, false, pmsg->path_cost, false);
}

/**
 * @brief Обрабатывает сообщение конфигурации интерфейса.
 *
 * Эта функция используется для обработки сообщений, связанных с изменениями конфигурации
 * интерфейсов. Она анализирует переданные данные, выполняет соответствующие изменения
 * в конфигурации интерфейса и, при необходимости, обновляет состояние STP для данного интерфейса.
 *
 * @param msg Указатель на сообщение конфигурации интерфейса. Ожидается, что сообщение
 *            приведено к структуре, содержащей данные о конфигурации интерфейса.
 *
 * @return void
 */
static void stpmgr_process_intf_config_msg(void* msg)
{
    STP_PORT_CONFIG_MSG* pmsg = (STP_PORT_CONFIG_MSG*)msg;
    uint32_t port_id;
    int inst_count;
    VLAN_ATTR* attr;
    UINT32 path_cost;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }

    STP_LOG_INFO("op:%d, intf:%s, enable:%d, root_grd:%d, bpdu_grd:%d , do_dis:%d, cost:%d, pri:%d, portfast:%d, uplink_fast:%d, count:%d",
                 pmsg->opcode, pmsg->intf_name, pmsg->enabled, pmsg->root_guard, pmsg->bpdu_guard,
                 pmsg->bpdu_guard_do_disable, pmsg->path_cost, pmsg->priority,
                 pmsg->portfast, pmsg->uplink_fast, pmsg->count);

    port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
    if (port_id == BAD_PORT_ID)
    {
        if (!STP_IS_PO_PORT(pmsg->intf_name))
            return;

        port_id = stp_intf_handle_po_preconfig(pmsg->intf_name);
        if (port_id == BAD_PORT_ID)
            return;
    }

    stputil_set_global_enable_mask(port_id, pmsg->enabled);

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        if (pmsg->priority != -1)
            stp_intf_set_port_priority(port_id, pmsg->priority);
        if (pmsg->path_cost)
            stp_intf_set_path_cost(port_id, pmsg->path_cost);

        attr = pmsg->vlan_list;
        for (inst_count = 0; inst_count < pmsg->count; inst_count++)
        {
            if (attr[inst_count].inst_id > g_stp_instances)
            {
                STP_LOG_ERR("invalid instance id %d", attr[inst_count].inst_id);
                continue;
            }

            STP_LOG_DEBUG("%d", attr[inst_count].inst_id);

            if (pmsg->enabled)
            {
                stpmgr_add_control_port(attr[inst_count].inst_id, port_id, attr[inst_count].mode);

                if (pmsg->priority != -1)
                    stpmgr_config_port_priority(attr[inst_count].inst_id, port_id, pmsg->priority, true);
                if (pmsg->path_cost)
                    stpmgr_config_port_path_cost(attr[inst_count].inst_id, port_id, false, pmsg->path_cost, true);
            }
            else
            {
                stpmgr_delete_control_port(attr[inst_count].inst_id, port_id, false);
            }
        }

        if (pmsg->enabled)
        {
            stpmgr_config_root_protect(port_id, pmsg->root_guard);
            stpmgr_config_protect(port_id, pmsg->bpdu_guard, pmsg->bpdu_guard_do_disable);

            stpmgr_config_fastspan(port_id, pmsg->portfast);
            stpmgr_config_fastuplink(port_id, pmsg->uplink_fast);
        }
    }
    else
    {
        /* Interface is deleted either due to non-L2 and PO delete */
        stp_intf_set_port_priority(port_id, STP_DFLT_PORT_PRIORITY);
        path_cost = stputil_get_default_path_cost(port_id, g_stpd_extend_mode);
        stp_intf_set_path_cost(port_id, path_cost);
    }

    if (pmsg->opcode == STP_DEL_COMMAND || !pmsg->enabled)
    {
        stpmgr_config_root_protect(port_id, false);
        stpmgr_config_protect(port_id, false, false);

        stpmgr_config_fastspan(port_id, true);
        stpmgr_config_fastuplink(port_id, false);

        stpsync_del_stp_port(pmsg->intf_name);
    }
}

/**
 * @brief Обрабатывает сообщение конфигурации членства VLAN.
 *
 * Эта функция используется для обработки сообщений, которые управляют членством
 * портов в VLAN. Она анализирует переданные данные, выполняет добавление или
 * удаление порта из указанного VLAN и обновляет связанные параметры STP.
 *
 * @param msg Указатель на сообщение конфигурации членства VLAN. Ожидается, что
 *            сообщение приведено к структуре, содержащей идентификатор VLAN,
 *            порт и действие.
 *
 * @return void
 */
static void stpmgr_process_vlan_mem_config_msg(void* msg)
{
    STP_VLAN_MEM_CONFIG_MSG* pmsg = (STP_VLAN_MEM_CONFIG_MSG*)msg;
    uint32_t port_id;
    STP_CLASS* stp_class;
    STP_PORT_CLASS* stp_port_class;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }

    if (pmsg->inst_id > g_stp_instances)
    {
        STP_LOG_ERR("invalid inst_id:%d", pmsg->inst_id);
        return;
    }

    STP_LOG_INFO("op:%d, vlan:%d, inst_id:%d, intf:%s, mode:%d, cost:%d, pri:%d enabled:%d",
                 pmsg->opcode, pmsg->vlan_id, pmsg->inst_id, pmsg->intf_name, pmsg->mode,
                 pmsg->path_cost, pmsg->priority, pmsg->enabled);

    port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
    if (port_id == BAD_PORT_ID)
        return;

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        if (pmsg->enabled)
        {
            stpmgr_add_control_port(pmsg->inst_id, port_id, pmsg->mode);
        }
        else
        {
            /* STP not enabled on this interface. Make it FORWARDING */
            stpsync_update_port_state(pmsg->intf_name, pmsg->inst_id, FORWARDING);
        }

        if (pmsg->priority != -1)
            stpmgr_config_port_priority(pmsg->inst_id, port_id, pmsg->priority, true);
        if (pmsg->path_cost)
            stpmgr_config_port_path_cost(pmsg->inst_id, port_id, false, pmsg->path_cost, true);
    }
    else
    {
        /* This is a case where vlan is deleted from port, so we shouldn't add vid from linux bridge port this happens
         * as before deletion we set port to forwarding state (which adds vid to linux bridge port)
         * Setting kernel state to forward ensures we skip deleting the vid from the linux bridge port
         * */

        stp_class = GET_STP_CLASS(pmsg->inst_id);
        if (is_member(stp_class->control_mask, port_id))
        {
            stp_port_class = GET_STP_PORT_CLASS(stp_class, port_id);
            stp_port_class->kernel_state = STP_KERNEL_STATE_FORWARD;

            stpmgr_delete_control_port(pmsg->inst_id, port_id, true);
        }
        else
        {
            stpsync_del_port_state(pmsg->intf_name, pmsg->inst_id);
        }
    }
}

/**
 * @brief Обрабатывает входящее IPC-сообщение от клиента.
 *
 * Эта функция отвечает за обработку сообщений межпроцессного взаимодействия (IPC),
 * полученных от клиента. В зависимости от типа сообщения, она вызывает
 * соответствующие функции для выполнения запрошенного действия и отправляет
 * ответ клиенту.
 *
 * @param msg Указатель на структуру IPC-сообщения. Содержит информацию о типе
 *            команды и связанных данных.
 * @param len Длина сообщения в байтах.
 * @param client_addr Структура `sockaddr_un`, содержащая адрес клиента,
 *                    отправившего сообщение.
 *
 * @return void
 */
static void stpmgr_process_ipc_msg(STP_IPC_MSG* msg, int len, struct sockaddr_un client_addr)
{
    int ret;
    //! TODO переписать парсер IPC msg
    STP_LOG_INFO("rcvd %s msg type", msgtype_str[msg->msg_type]);

    /* Temp code until warm boot is handled */
    if (msg->msg_type != STP_INIT_READY && msg->msg_type != STP_STPCTL_MSG)
    {
        if (g_max_stp_port == 0)
        {
            STP_LOG_ERR("max port invalid ignore msg type %s", msgtype_str[msg->msg_type]);
            return;
        }
    }

    switch (msg->msg_type)
    {
    case STP_INIT_READY:
    {
        STP_INIT_READY_MSG* pmsg = (STP_INIT_READY_MSG*)msg->data;
        /* All ports are initialized in the system. Now build IF DB in STP */
        ret = stp_intf_event_mgr_init();
        if (ret == -1)
            return;

        /* Do other protocol related inits */
        stpmgr_init(pmsg->max_stp_instances);
        break;
    }
    case STP_BRIDGE_CONFIG:
    {
        stpmgr_process_bridge_config_msg(msg->data);
        break;
    }
    case STP_VLAN_CONFIG:
    {
        stpmgr_process_vlan_config_msg(msg->data);
        break;
    }
    case STP_VLAN_PORT_CONFIG:
    {
        stpmgr_process_vlan_intf_config_msg(msg->data);
        break;
    }
    case STP_PORT_CONFIG:
    {
        stpmgr_process_intf_config_msg(msg->data);
        break;
    }
    case STP_VLAN_MEM_CONFIG:
    {
        stpmgr_process_vlan_mem_config_msg(msg->data);
        break;
    }

    case STP_STPCTL_MSG:
    {
        STP_LOG_INFO("Server received from %s", client_addr.sun_path);
        stpdbg_process_ctl_msg(msg->data);
        // TODO! refactor to internal class sendf
        // stpmgr_send_reply(client_addr, (void*)msg, len);
        STP_LOG_ERR("SOMEHOW tried to send to UNIX socket %s", client_addr.sun_path);
        break;
    }

    default:
        break;
    }
}

/* Process all messages from clients (STPMGRd) */
/**
 * @brief Обрабатывает входящее сообщение от клиента через IPC-сокет.
 *
 * Эта функция вызывается, когда приходит сообщение от клиента через IPC-сокет.
 * Она считывает данные, определяет тип сообщения и вызывает соответствующую
 * функцию для обработки. После выполнения запроса отправляет ответ клиенту.
 *
 * @param fd Сокетный дескриптор, через который было получено сообщение.
 * @param what Тип события, связанный с сокетом (например, готовность к чтению).
 * @param arg Указатель на дополнительные данные, связанные с контекстом STP
 *            (обычно структура, управляющая IPC).
 *
 * @return void
 */
void stpmgr_recv_client_msg(evutil_socket_t fd, short what, void* arg)
{
    char buffer[4096];
    int len;
    struct sockaddr_un client_sock;

    g_stpd_stats_libev_ipc++;

    len = sizeof(struct sockaddr_un);
    len = recvfrom(fd, buffer, 4096, 0, (struct sockaddr*)&client_sock, &len);
    if (len == -1)
    {
        STP_LOG_ERR("recv  message error %s", strerror(errno));
    }
    else if (len < 10)
    {
        STP_LOG_ERR("message error, len too small= %d", len);
    }
    else if (!((buffer[0] == 'w') && (buffer[1] == 'b') && (buffer[2] == 'o') && (buffer[3] == 's') && (buffer[4] == 'b')))
    {
        STP_LOG_ERR("message error, magic is wrong bin header, message= %.*s", 5, buffer);
        // stpmgr_process_ipc_msg((STP_IPC_MSG*)(buffer), (len), client_sock);
    }
    else
    {
        STP_LOG_INFO("magic is ok, alpha message = %.*s", 5, buffer);
        stpmgr_process_ipc_msg((STP_IPC_MSG*)(buffer + 5), (len - 5), client_sock);
    }
}
