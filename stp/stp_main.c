/**
 * @file stp_main.c
 * @author your name (you@domain.com)
 * @brief Основная логика обработки пакетов STP, включая прием и отправку BPDU.
 * @version 0.1
 * @date 2024-11-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "stp_main.h"

/* Глобальная структура контекста STP */
STPD_CONTEXT stpd_context;

/**
 * @brief служит для инициализации механизмов межпроцессного взаимодействия (IPC, Inter-Process Communication). Это взаимодействие необходимо для обмена данными между демоном STP (stpd) и другими компонентами системы, такими как базы данных SONiC (CONFIG_DB, STATE_DB)
 * Основные задачи функции stpd_ipc_init:
Настройка сокетов IPC:

Создание и настройка сокетов для связи между демоном STP и другими процессами.
Определение адресов и портов для отправки и приема сообщений.
Регистрация в системе IPC:

Демон регистрирует себя как активный процесс, который готов принимать сообщения.
Устанавливается канал связи для обработки запросов.
Инициализация структуры данных IPC:

Настройка буферов и очередей сообщений, используемых для передачи данных.
Определение формата сообщений, включая идентификаторы запросов, ответы, и данные (например, информацию о состоянии портов или топологии).
Обработка ошибок:

Проверка корректности инициализации.
Обработка ошибок, связанных с невозможностью установить соединение или создать необходимые ресурсы.

задачи, решаемые через IPC в STP:
Обмен информацией о топологии:

Передача данных о состоянии портов и VLAN другим процессам.
Получение изменений конфигурации из CONFIG_DB.
Синхронизация состояния:

Информирование других сервисов о топологии, идентификаторах корневого моста и состоянии портов.
Обработка внешних команд:

Получение запросов на включение или отключение STP для определенных VLAN.
Применение изменений конфигурации.
 *
 * @return int Возвращает код успеха или ошибки (0 для успешной инициализации, отрицательное значение при ошибке).
 */
int stpd_ipc_init()
{
    struct sockaddr_un sa;
    int ret;
    struct event *ipc_event = 0;

    /* Удаление существующего сокета */
    unlink(STPD_SOCK_NAME);

    /* Создание UNIX-сокета для обмена сообщениями */
    g_stpd_ipc_handle = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (!g_stpd_ipc_handle)
    {
        STP_LOG_ERR("ipc socket error %s", strerror(errno));
        return -1;
    }

    /* Настройка структуры адреса сокета */
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, STPD_SOCK_NAME, sizeof(sa.sun_path) - 1);

    /* Привязка сокета к адресу */
    ret = bind(g_stpd_ipc_handle, (struct sockaddr *)&sa, sizeof(struct sockaddr_un));
    if (ret == -1)
    {
        STP_LOG_ERR("ipc bind error %s", strerror(errno));
        close(g_stpd_ipc_handle);
        return -1;
    }

    /* Создание события для обработки сообщений через IPC */
    ipc_event = stpmgr_libevent_create(g_stpd_evbase, g_stpd_ipc_handle,
                                       EV_READ | EV_PERSIST, stpmgr_recv_client_msg, (char *)"IPC", NULL);
    if (!ipc_event)
    {
        STP_LOG_ERR("ipc_event Create failed");
        return -1;
    }

    STP_LOG_DEBUG("ipc init done");

    return 0;
}

/**
 * @brief Инициализирует систему логирования. Устанавливает уровень логирования в зависимости от наличия файла /stpd_dbg_reload: если файл существует, уровень устанавливается на DEBUG, иначе — на INFO.
 *
 */
void stpd_log_init()
{
    /**
     * @brief Установка уровня логирования на основе наличия файла /stpd_dbg_reload
     *
     */
    STP_LOG_INIT(); //TODO! не работает - переделать
    if (fopen("/stpd_dbg_reload", "r"))
    {
        STP_LOG_SET_LEVEL(STP_LOG_LEVEL_DEBUG);
    }
    else
        STP_LOG_SET_LEVEL(STP_LOG_LEVEL_INFO);
}

int stpd_main()
{
    int rc = 0;
    struct timeval stp_100ms_tv = {0, STPD_100MS_TIMEOUT};
    struct timeval msec_50 = {0, 50 * 1000};
    struct event *evtimer_100ms = 0;
    struct event *evpkt = 0;
    struct event_config *cfg = 0;
    int8_t ret = 0;

    /* Игнорирование сигнала SIGPIPE */
    signal(SIGPIPE, SIG_IGN);

    /* Инициализация системы логирования */
    stpd_log_init();

    /* Очистка таблиц STP в APP_DB */
    stpsync_clear_appdb_stp_tables();

    /* Обнуление структуры контекста STP */
    memset(&stpd_context, 0, sizeof(STPD_CONTEXT));

    /* Установка расширенного режима */
    stpmgr_set_extend_mode(true);

    /* Создание конфигурации для libevent */
    cfg = event_config_new();
    if (!cfg)
    {
        STP_LOG_ERR("event_config_new Failed");
        return -1;
    }

    STP_LOG_INFO("LIBEVENT VER : 0x%x", event_get_version_number());

    /* Настройка максимального интервала диспетчеризации событий */
    event_config_set_max_dispatch_interval(cfg, &msec_50 /*max_interval*/, 5 /*max_callbacks*/, 1 /*min-prio*/);

    /* Создание нового event_base с заданной конфигурацией */
    g_stpd_evbase = event_base_new_with_config(cfg);
    if (g_stpd_evbase == NULL)
    {
        STP_LOG_ERR("eventbase create Failed ");
        return -1;
    }

    /* Инициализация приоритетов очередей событий */
    event_base_priority_init(g_stpd_evbase, STP_LIBEV_PRIO_QUEUES);

    /* Создание высокоприоритетного таймера с интервалом 100 мс */
    evtimer_100ms = stpmgr_libevent_create(g_stpd_evbase, -1, EV_PERSIST,
                                           stpmgr_100ms_timer, (char *)"100MS_TIMER", &stp_100ms_tv);
    if (!evtimer_100ms)
    {
        STP_LOG_ERR("evtimer_100ms Create failed");
        return -1;
    }

    /* Инициализация IPC для взаимодействия с менеджером STP */
    rc = stpd_ipc_init();
    if (rc < 0)
    {
        STP_LOG_ERR("ipc init failed");
        return -1;
    }

    /* Создание базы данных интерфейсов STP */
    g_stpd_intf_db = avl_create(&stp_intf_avl_compare, NULL, NULL);
    if (!g_stpd_intf_db)
    {
        STP_LOG_ERR("intf db create failed");
        return -1;
    }

    /* Инициализация Netlink для получения информации об интерфейсах */
    g_stpd_netlink_handle = stp_netlink_init(&stp_intf_netlink_cb);
    if (-1 == g_stpd_netlink_handle)
    {
        STP_LOG_ERR("netlink init failed");
        return -1;
    }

    /* Создание сокета для передачи пакетов */
    /* Open Socket for Packet Tx
     * We need this for sending packet over Port-channels.
     * To simplify the design all STP tx will use this socket.
     * For RX we have per phy-port socket.
     *
     */
    if (-1 == (g_stpd_pkt_handle = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))))
    {
        STP_LOG_ERR("Create g_stpd_pkt_tx_handle, errno : %s", strerror(errno));
        return -1;
    }

    STP_LOG_INFO("STP Daemon Started");

    event_base_dispatch(g_stpd_evbase);

    return 0;
}
