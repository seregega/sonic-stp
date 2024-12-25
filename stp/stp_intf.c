/**
 * @file stp_intf.c
 * @brief Реализация функций управления интерфейсами в протоколе STP.
 *
 * Этот файл содержит функции для работы с интерфейсами STP, включая доступ к базе данных
 * интерфейсов, управление параметрами портов, обработку событий Netlink, а также
 * управление портовыми группами и агрегацией каналов.
 *
 * @details
 * Включает функциональность:
 * - Управление AVL-деревом интерфейсов.
 * - Работа с параметрами интерфейсов, такими как скорость, приоритет и стоимость пути.
 * - Интеграция с Netlink для получения данных об интерфейсах.
 * - Поддержка агрегированных каналов (LAG/PO).
 *
 * @author
 * Broadcom
 *
 * @license
 * Apache License 2.0
 */

#include "stp_inc.h"

/**
 * @brief Возвращает файловый дескриптор Netlink для работы с интерфейсами.
 *
 * Функция используется для получения текущего дескриптора Netlink, который
 * используется для взаимодействия с ядром Linux.
 *
 * @return int
 *         Файловый дескриптор Netlink.
 */
int stp_intf_get_netlink_fd()
{
    return stpd_context.netlink_fd;
}

/**
 * @brief Возвращает указатель на базу событий libevent.
 *
 * Функция используется для получения текущего объекта базы событий,
 * который управляет асинхронными событиями в libevent.
 *
 * @return struct event_base*
 *         Указатель на базу событий libevent.
 */
struct event_base *stp_intf_get_evbase()
{
    return stpd_context.evbase;
}

/**
 * @brief Получает имя интерфейса по идентификатору порта.
 *
 * Функция ищет в базе данных интерфейсов имя, связанное с заданным
 * идентификатором порта.
 *
 * @param port_id Идентификатор порта.
 *
 * @return char*
 *         Имя интерфейса или `NULL`, если интерфейс не найден.
 */
char *stp_intf_get_port_name(uint32_t port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while (NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->ifname;
    }
    return NULL;
}

/**
 * @brief Проверяет, активен ли указанный порт.
 *
 * Функция возвращает состояние операционной активности порта.
 *
 * @param port_id Идентификатор порта.
 *
 * @return bool
 *         - `true`, если порт активен.
 *         - `false`, если порт неактивен.
 */
bool stp_intf_is_port_up(int port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while (NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->oper_state ? true : false;
    }
    return false;
}

/**
 * @brief Получает скорость порта.
 *
 * Функция возвращает текущую скорость указанного порта в Мбит/с.
 *
 * @param port_id Идентификатор порта.
 *
 * @return uint32_t
 *         Скорость порта в Мбит/с или `0`, если порт не найден.
 */
uint32_t stp_intf_get_speed(int port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while (NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->speed;
    }
    return 0;
}

/**
 * @brief Получает узел интерфейса по идентификатору порта.
 *
 * Функция возвращает структуру узла интерфейса, связанную с указанным
 * идентификатором порта.
 *
 * @param port_id Идентификатор порта.
 *
 * @return INTERFACE_NODE*
 *         Указатель на узел интерфейса или `NULL`, если узел не найден.
 */
INTERFACE_NODE *stp_intf_get_node(uint32_t port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while (NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node;
    }

    return NULL;
}

/**
 * @brief Получает узел интерфейса по индексу ядра (kernel ifindex).
 *
 * Функция возвращает структуру узла интерфейса, связанную с заданным
 * индексом интерфейса ядра.
 *
 * @param kif_index Индекс интерфейса в ядре.
 *
 * @return INTERFACE_NODE*
 *         Указатель на узел интерфейса или `NULL`, если узел не найден.
 */
INTERFACE_NODE *stp_intf_get_node_by_kif_index(uint32_t kif_index)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while (NULL != (node = avl_t_next(&trav)))
    {
        if (node->kif_index == kif_index)
            return node;
    }

    return NULL;
}

/**
 * @brief Получает MAC-адрес для указанного порта.
 *
 * Функция извлекает MAC-адрес порта и сохраняет его в переданную структуру.
 * В текущей реализации для всех интерфейсов используется один и тот же MAC-адрес.
 *
 * @param port_id Идентификатор порта.
 * @param mac Указатель на структуру для сохранения MAC-адреса.
 *
 * @return bool
 *         - `true`, если MAC-адрес успешно получен.
 *         - `false`, если произошла ошибка.
 */
bool stp_intf_get_mac(int port_id, MAC_ADDRESS *mac)
{
    // SONIC has same MAC for all interface
    COPY_MAC(mac, &g_stp_base_mac_addr);
#if 0
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
        {
            memcpy(&mac->_ulong, &node->mac[0], sizeof(uint32_t));
            memcpy(&mac->_ushort, &node->mac[4], sizeof(uint16_t));
            return true;
        }
    }
    return false;
#endif
}

#if 0
/**
 * @brief Получает индекс интерфейса в ядре для указанного порта.
 *
 * Функция возвращает индекс интерфейса в ядре Linux (kernel ifindex),
 * связанный с данным идентификатором порта. Используется для взаимодействия
 * с ядром через Netlink.
 *
 * @param port_id Идентификатор порта, для которого требуется получить kernel ifindex.
 *
 * @return uint32_t
 *         - Индекс интерфейса в ядре (kernel ifindex), если порт найден.
 *         - `0`, если порт не найден или произошла ошибка.
 */
uint32_t stp_intf_get_kif_index(uint32_t port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->kif_index;
    }

    return BAD_PORT_ID;
}

/**
 * @brief Получает идентификатор порта по индексу интерфейса в ядре.
 *
 * Функция выполняет поиск в базе данных интерфейсов STP и возвращает
 * идентификатор порта, соответствующий указанному kernel ifindex (индексу интерфейса в ядре).
 *
 * @param kif_index Индекс интерфейса в ядре (kernel ifindex), для которого требуется найти порт.
 *
 * @return uint32_t
 *         - Идентификатор порта, если интерфейс найден.
 *         - `0`, если интерфейс не найден.
 */
uint32_t stp_intf_get_port_id_by_kif_index(uint32_t kif_index)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->kif_index == kif_index)
            return node->port_id;
    }

    return BAD_PORT_ID;
}

/**
 * @brief Получает индекс интерфейса в ядре по идентификатору порта.
 *
 * Функция выполняет поиск в базе данных интерфейсов STP и возвращает
 * kernel ifindex (индекс интерфейса в ядре), соответствующий указанному
 * идентификатору порта.
 *
 * @param port_id Идентификатор порта, для которого требуется найти kernel ifindex.
 *
 * @return uint32_t
 *         - Индекс интерфейса в ядре (kernel ifindex), если порт найден.
 *         - `0`, если порт не найден.
 */
uint32_t stp_intf_get_kif_index_by_port_id(uint32_t port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->kif_index;
    }

    return BAD_PORT_ID;
}
#endif

/**
 * @brief Получает идентификатор порта по имени интерфейса.
 *
 * Функция выполняет поиск в базе данных интерфейсов STP и возвращает
 * идентификатор порта, связанный с указанным именем интерфейса.
 *
 * @param ifname Имя интерфейса, для которого требуется найти идентификатор порта.
 *
 * @return uint32_t
 *         - Идентификатор порта, если интерфейс найден.
 *         - `0`, если интерфейс не найден.
 */
uint32_t stp_intf_get_port_id_by_name(char *ifname)
{
    INTERFACE_NODE *node = 0;

    node = stp_intf_get_node_by_name(ifname);
    if (node && (node->port_id != BAD_PORT_ID))
        return node->port_id;

    return BAD_PORT_ID;
}

/**
 * @brief Получает узел интерфейса по имени интерфейса.
 *
 * Функция выполняет поиск в базе данных интерфейсов STP и возвращает указатель
 * на структуру узла интерфейса, связанного с указанным именем интерфейса.
 *
 * @param ifname Имя интерфейса, для которого требуется найти узел интерфейса.
 *
 * @return INTERFACE_NODE*
 *         - Указатель на узел интерфейса, если интерфейс найден.
 *         - `NULL`, если интерфейс не найден.
 */
INTERFACE_NODE *stp_intf_get_node_by_name(char *ifname)
{
    INTERFACE_NODE search_node;

    if (!ifname)
        return NULL;

    memset(&search_node, 0, sizeof(INTERFACE_NODE));
    memcpy(search_node.ifname, ifname, IFNAMSIZ);

    return avl_find(g_stpd_intf_db, &search_node);
}

/**
 * @brief Сравнивает два узла AVL-дерева для базы данных интерфейсов STP.
 *
 * Функция используется как callback для операций вставки, поиска и удаления
 * в AVL-дереве интерфейсов. Выполняет сравнение двух узлов дерева
 * на основе пользовательских данных и параметров.
 *
 * @param user_p Указатель на пользовательские данные, которые необходимо сравнить.
 * @param data_p Указатель на данные узла AVL-дерева, с которыми происходит сравнение.
 * @param param Дополнительные параметры для настройки логики сравнения (может быть NULL).
 *
 * @return int
 *         - Отрицательное значение, если `user_p` меньше `data_p`.
 *         - `0`, если `user_p` эквивалентен `data_p`.
 *         - Положительное значение, если `user_p` больше `data_p`.
 */
int stp_intf_avl_compare(const void *user_p, const void *data_p, void *param)
{
    stp_if_avl_node_t *pa = (stp_if_avl_node_t *)user_p;
    stp_if_avl_node_t *pb = (stp_if_avl_node_t *)data_p;

    return (strncasecmp(pa->ifname, pb->ifname, IFNAMSIZ));
}

/**
 * @brief Удаляет указанный интерфейс из базы данных интерфейсов STP.
 *
 * Функция удаляет узел интерфейса из AVL-дерева базы данных интерфейсов STP.
 * Используется для удаления информации об интерфейсе при его деинициализации
 * или отключении.
 *
 * @param node Указатель на структуру узла интерфейса, который нужно удалить.
 *
 * @return void
 */
void stp_intf_del_from_intf_db(INTERFACE_NODE *node)
{
    STP_LOG_INFO("AVL Delete :  %s  kif : %d  port_id : %u", node->ifname, node->kif_index, node->port_id);

    if (STP_IS_ETH_PORT(node->ifname))
        stp_pkt_sock_close(node);

    avl_delete(g_stpd_intf_db, node);
    free(node);

    return;
}

/**
 * @brief Добавляет указанный интерфейс в базу данных интерфейсов STP.
 *
 * Функция добавляет узел интерфейса в AVL-дерево базы данных интерфейсов STP.
 * Используется для инициализации интерфейса при его создании или активации.
 *
 * @param node Указатель на структуру узла интерфейса, который нужно добавить.
 *
 * @return uint32_t
 *         - `1`, если узел успешно добавлен.
 *         - `0`, если узел уже существует или произошла ошибка.
 */
uint32_t stp_intf_add_to_intf_db(INTERFACE_NODE *node)
{
    void **ret_ptr = avl_probe(g_stpd_intf_db, node);
    if (*ret_ptr != node)
    {
        if (*ret_ptr == NULL)
            STP_LOG_CRITICAL("AVL-Insert Malloc Failure, Intf: %s kif: %d", node->ifname, node->kif_index);
        else
            STP_LOG_CRITICAL("DUPLICATE Entry found Intf: %s kif: %d", ((INTERFACE_NODE *)(*ret_ptr))->ifname, ((INTERFACE_NODE *)(*ret_ptr))->kif_index);
        // This should never happen.
        sys_assert(0);
    }
    else
    {
        STP_LOG_INFO("AVL Insert :  %s %d %u", node->ifname, node->kif_index, node->port_id);

        // create socket only for Ethernet ports
        if (STP_IS_ETH_PORT(node->ifname))
            stp_pkt_sock_create(node);

        return node->port_id;
    }
}

/**
 * @brief Получает имя интерфейса по индексу ядра (kernel ifindex).
 *
 * Функция использует ioctl-запрос для получения имени интерфейса,
 * соответствующего указанному индексу интерфейса в ядре.
 *
 * @param kif_index Индекс интерфейса в ядре (kernel ifindex).
 * @param if_name Указатель на буфер для сохранения имени интерфейса.
 *                Буфер должен быть достаточного размера, чтобы вместить имя интерфейса.
 *
 * @return bool
 *         - `true`, если имя интерфейса успешно получено.
 *         - `false`, если произошла ошибка (например, интерфейс не найден).
 */
bool stp_intf_ioctl_get_ifname(uint32_t kif_index, char *if_name)
{
    struct ifreq ifr;

    ifr.ifr_ifindex = kif_index;
    if (ioctl(g_stpd_ioctl_sock, SIOCGIFNAME, &ifr) < 0)
        return false;

    strncpy(if_name, ifr.ifr_name, IFNAMSIZ);
    return true;
}

/**
 * @brief Получает индекс интерфейса в ядре (kernel ifindex) по имени интерфейса.
 *
 * Функция использует ioctl-запрос для получения индекса интерфейса в ядре,
 * соответствующего указанному имени интерфейса.
 *
 * @param if_name Имя интерфейса, для которого требуется получить kernel ifindex.
 *
 * @return uint32_t
 *         - Индекс интерфейса в ядре (kernel ifindex), если операция выполнена успешно.
 *         - `0`, если произошла ошибка (например, интерфейс не найден).
 */
uint32_t stp_intf_ioctl_get_kif_index(char *if_name)
{
    struct ifreq ifr;
    size_t if_name_len = strlen(if_name);

    if (if_name_len < sizeof(ifr.ifr_name))
    {
        memcpy(ifr.ifr_name, if_name, if_name_len);
        ifr.ifr_name[if_name_len] = '\0';

        if (ioctl(g_stpd_ioctl_sock, SIOCGIFINDEX, &ifr) < 0)
            return -1;

        return ifr.ifr_ifindex;
    }
    return -1;
}

/**
 * @brief Создаёт узел интерфейса для базы данных STP.
 *
 * Функция создаёт новый узел интерфейса и инициализирует его с заданным
 * именем интерфейса и kernel ifindex. Используется при добавлении нового
 * интерфейса в базу данных STP.
 *
 * @param ifname Имя интерфейса, для которого создаётся узел.
 * @param kif_index Индекс интерфейса в ядре (kernel ifindex).
 *
 * @return INTERFACE_NODE*
 *         - Указатель на созданный узел интерфейса.
 *         - `NULL`, если произошла ошибка при создании узла.
 */
INTERFACE_NODE *stp_intf_create_intf_node(char *ifname, uint32_t kif_index)
{
    INTERFACE_NODE *node = NULL;

    node = calloc(1, sizeof(INTERFACE_NODE));
    if (!node)
    {
        STP_LOG_CRITICAL("Calloc Failed");
        return NULL;
    }

    /* Initialize to BAD port id */
    node->port_id = BAD_PORT_ID;

    /* Update interface name */
    if (ifname)
    {
        strncpy(node->ifname, ifname, IFNAMSIZ);
    }
    else
    {
        if (stp_intf_ioctl_get_ifname(kif_index, node->ifname))
            STP_LOG_INFO("Kernel ifindex %u name %s", kif_index, node->ifname);
        else
            STP_LOG_ERR("Kernel ifindex %u name fetch failed", kif_index);
    }

    /* Update kernel ifindex*/
    if (kif_index == BAD_PORT_ID)
    {
        /* Update kernel ifindex*/
        node->kif_index = stp_intf_ioctl_get_kif_index(ifname);
        if (node->kif_index == BAD_PORT_ID)
            STP_LOG_ERR("Kernel ifindex fetch for %s failed", ifname);
    }
    else
    {
        node->kif_index = kif_index;
    }

    node->priority = (STP_DFLT_PORT_PRIORITY >> 4);

    stp_intf_add_to_intf_db(node);
    return node;
}

/**
 * @brief Обрабатывает предварительную конфигурацию Port-Channel (PO).
 *
 * Функция используется для обработки предварительных настроек
 * агрегированных каналов (Port-Channel) перед их окончательной
 * конфигурацией в системе STP. Возвращает идентификатор порта (PORT_ID)
 * для созданного или найденного Port-Channel.
 *
 * @param ifname Имя интерфейса Port-Channel, который необходимо обработать.
 *
 * @return PORT_ID
 *         - Уникальный идентификатор Port-Channel.
 *         - `INVALID_PORT_ID`, если произошла ошибка.
 */
PORT_ID stp_intf_handle_po_preconfig(char *ifname)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node_by_name(ifname);
    if (!node)
    {
        node = stp_intf_create_intf_node(ifname, BAD_PORT_ID);
        if (!node)
            return BAD_PORT_ID;
    }

    /* Allocate port id for PO if not yet done */
    if (node->port_id == BAD_PORT_ID && g_stpd_port_init_done)
    {
        node->port_id = stp_intf_allocate_po_id();
        if (node->port_id == BAD_PORT_ID)
            sys_assert(0);
    }
    return node->port_id;
}

/**
 * @brief Добавляет член Port-Channel (PO) в указанный узел интерфейса.
 *
 * Функция добавляет физический интерфейс в состав агрегированного канала
 * (Port-Channel). Обновляет данные узла интерфейса и синхронизирует информацию
 * с базой данных STP.
 *
 * @param if_node Указатель на узел интерфейса, представляющий Port-Channel,
 *                в который добавляется член (участник).
 *
 * @return void
 */
void stp_intf_add_po_member(INTERFACE_NODE *if_node)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node_by_kif_index(if_node->master_ifindex);
    if (!node)
    {
        node = stp_intf_create_intf_node(NULL, if_node->master_ifindex);
        if (!node)
            return;
    }

    /* Increment member port count */
    node->member_port_count++;

    /* Populate PO speed from member port speed */
    if (node->speed == 0)
    {
        node->speed = if_node->speed;

        /* Calculate default Path cost */
        node->path_cost = stputil_get_path_cost(node->speed, g_stpd_extend_mode);
    }

    /* Allocate port id for PO if not yet done */
    if (node->port_id == BAD_PORT_ID && g_stpd_port_init_done)
    {
        node->port_id = stp_intf_allocate_po_id();
        if (node->port_id == BAD_PORT_ID)
            sys_assert(0);
    }

    STP_LOG_INFO("Add PO member kernel_if - %u member_if - %u kif_index - %u", if_node->master_ifindex, if_node->port_id, if_node->kif_index);
}

/**
 * @brief Удаляет член (участника) из указанного Port-Channel (PO).
 *
 * Функция удаляет физический интерфейс (член) из состава агрегированного канала
 * (Port-Channel). Обновляет данные узла интерфейса Port-Channel в базе данных STP.
 *
 * @param po_kif_index Индекс Port-Channel в ядре (kernel ifindex), из которого удаляется член.
 * @param member_port Идентификатор физического интерфейса (члена), который необходимо удалить.
 *
 * @return void
 */
void stp_intf_del_po_member(uint32_t po_kif_index, uint32_t member_port)
{
    INTERFACE_NODE *node = 0;
    STP_INDEX stp_index = 0;

    node = stp_intf_get_node_by_kif_index(po_kif_index);
    if (!node)
    {
        STP_LOG_ERR("PO not found in interface DB kernel_if - %u member_if - %u", po_kif_index, member_port);
        return;
    }

    if (node->member_port_count == 0)
    {
        STP_LOG_ERR("PO member count is 0 kernel_if - %u member_if - %u", po_kif_index, member_port);
        return;
    }

    node->member_port_count--;
    /* if this is last member port, delete PO node in avl tree*/
    if (!node->member_port_count)
    {
        stputil_set_global_enable_mask(node->port_id, false);
        for (stp_index = 0; stp_index < g_stp_instances; stp_index++)
            stpmgr_delete_control_port(stp_index, node->port_id, true);

        stp_intf_release_po_id(node->port_id);
        stp_intf_del_from_intf_db(node);
    }
    STP_LOG_INFO("Del PO member kernel_if - %u member_if - %u", po_kif_index, member_port);
}

/**
 * @brief Обновляет список членов Port-Channel (PO).
 *
 * Функция проверяет текущее состояние участников агрегированного канала
 * (Port-Channel) и обновляет список членов в базе данных STP.
 *
 * @param if_db Указатель на структуру данных Netlink, содержащую информацию об интерфейсе.
 * @param node Указатель на узел интерфейса Port-Channel, который необходимо обновить.
 *
 * @return bool
 *         - `true`, если список членов успешно обновлён.
 *         - `false`, если произошла ошибка.
 */
bool stp_intf_update_po_members(netlink_db_t *if_db, INTERFACE_NODE *node)
{
    /* Add member port to PO */
    if (!node->master_ifindex && if_db->master_ifindex)
    {
        node->master_ifindex = if_db->master_ifindex;
        stp_intf_add_po_member(node);
    }

    /* Delete member port from PO */
    if (node->master_ifindex && !if_db->master_ifindex)
    {
        stp_intf_del_po_member(node->master_ifindex, node->port_id);
        node->master_ifindex = 0;
    }
}

/**
 * @brief Обновляет базу данных интерфейсов STP.
 *
 * Функция добавляет или обновляет запись в базе данных интерфейсов STP
 * на основе данных, полученных из Netlink. Если запись уже существует,
 * она обновляется. Если запись отсутствует и флаг `is_add` установлен,
 * создаётся новая запись.
 *
 * @param if_db Указатель на структуру данных Netlink, содержащую информацию об интерфейсе.
 * @param is_add Флаг, указывающий на необходимость добавления записи:
 *               - `1` для добавления или обновления записи.
 *               - `0` для удаления записи.
 * @param init_in_prog Флаг, указывающий, выполняется ли инициализация базы данных:
 *                     - `true`, если процесс инициализации активен.
 *                     - `false`, если операция выполняется в рабочем режиме.
 * @param eth_if Флаг, указывающий, является ли интерфейс Ethernet:
 *               - `true` для Ethernet-интерфейсов.
 *               - `false` для других типов интерфейсов.
 *
 * @return INTERFACE_NODE*
 *         - Указатель на обновлённый или созданный узел интерфейса.
 *         - `NULL`, если операция завершилась с ошибкой.
 */
INTERFACE_NODE *stp_intf_update_intf_db(netlink_db_t *if_db, uint8_t is_add, bool init_in_prog, bool eth_if)
{
    INTERFACE_NODE *node = NULL;
    uint32_t port_id = 0;

    if (is_add)
    {
        node = stp_intf_get_node_by_name(if_db->ifname);
        if (!node)
        {
            node = stp_intf_create_intf_node(if_db->ifname, if_db->kif_index);
            if (!node)
                return NULL;

            /* Update port id */
            if (eth_if)
            {
                port_id = strtol(((char *)if_db->ifname + STP_ETH_NAME_PREFIX_LEN), NULL, 10);
                node->port_id = port_id;

                /* Derive Max Port */
                if (init_in_prog)
                {
                    if (port_id + (4 - (port_id % 4)) > g_max_stp_port)
                    {
                        g_max_stp_port = port_id + (4 - (port_id % 4));
                    }
                }
            }
            STP_LOG_INFO("Add Kernel ifindex %d name %s", if_db->kif_index, if_db->ifname);
        }

        /* Update the port speed */
        if (eth_if)
        {
            if (!node->speed)
            {
                node->speed = stpsync_get_port_speed(if_db->ifname);

                /* Calculate default Path cost */
                node->path_cost = stputil_get_path_cost(node->speed, g_stpd_extend_mode);
            }

            /* Handle PO member port */
            if (if_db->is_member || node->master_ifindex)
                stp_intf_update_po_members(if_db, node);
        }
        return node;
    }
    else
    {
        // Netlink Delete does not send name, hence traverse and get the node to delete
        node = stp_intf_get_node_by_kif_index(if_db->kif_index);
        if (!node)
        {
            STP_LOG_ERR("Delete FAILED, AVL Node not found, Kif: %d", if_db->kif_index);
            return NULL;
        }

        stp_intf_del_from_intf_db(node);
        STP_LOG_INFO("Del Kernel ifindex %x name %s", if_db->kif_index, if_db->ifname);
    }

    return NULL;
}

/**
 * @brief Callback-функция для обработки событий Netlink, связанных с интерфейсами STP.
 *
 * Функция вызывается при получении событий Netlink, таких как добавление, удаление
 * или обновление состояния интерфейсов. Она обновляет базу данных интерфейсов STP
 * и синхронизирует изменения с другими компонентами системы.
 *
 * @param if_db Указатель на структуру `netlink_db_t`, содержащую данные об интерфейсе,
 *              такие как имя, индекс ядра, состояние и скорость.
 * @param is_add Флаг, указывающий тип операции:
 *               - `1` для добавления или обновления интерфейса.
 *               - `0` для удаления интерфейса.
 * @param init_in_prog Флаг, указывающий, выполняется ли инициализация системы:
 *                     - `true`, если операция выполняется в процессе инициализации.
 *                     - `false`, если операция выполняется в рабочем режиме.
 *
 * @return void
 */
void stp_intf_netlink_cb(netlink_db_t *if_db, uint8_t is_add, bool init_in_prog)
{
    INTERFACE_NODE *node = NULL, *po_node = NULL;
    bool eth_if;
    g_stpd_stats_libev_netlink++;

    if (STP_IS_ETH_PORT(if_db->ifname))
        eth_if = true;
    else if (STP_IS_PO_PORT(if_db->ifname))
        eth_if = false;
    else
        return;

    node = stp_intf_update_intf_db(if_db, is_add, init_in_prog, eth_if);

    /* Handle oper data change */
    if (node)
    {
        /* Handle oper state change */
        if (if_db->oper_state != node->oper_state)
        {
            node->oper_state = if_db->oper_state;
            if (eth_if)
            {
                node->speed = stpsync_get_port_speed(if_db->ifname);
                /* Calculate default Path cost */
                node->path_cost = stputil_get_path_cost(node->speed, g_stpd_extend_mode);
                if (if_db->master_ifindex)
                {
                    po_node = stp_intf_get_node_by_kif_index(if_db->master_ifindex);

                    if (po_node && (po_node->member_port_count == 1 || !po_node->oper_state))
                    {
                        po_node->speed = node->speed;
                        /* Calculate default Path cost */
                        po_node->path_cost = node->path_cost;
                    }
                }
            }

            if (!init_in_prog && (if_db->master_ifindex == 0) && (node->port_id != BAD_PORT_ID))
            {
                stpmgr_port_event(node->port_id, if_db->oper_state);
            }
        }
    }
}

/**
 * @brief Инициализирует статистику портов в системе STP.
 *
 * Функция создаёт и инициализирует структуру данных для хранения
 * статистики всех портов. Это включает в себя статистику приёма,
 * передачи и ошибок BPDU.
 *
 * @return int
 *         - `0`, если инициализация успешна.
 *         - Отрицательное значение, если произошла ошибка.
 */
int stp_intf_init_port_stats()
{
    uint16_t i = 0;
    // Allocate stats array g_max_stp_port
    g_stpd_intf_stats = calloc(1, ((g_max_stp_port) * sizeof(STPD_INTF_STATS *)));
    if (!g_stpd_intf_stats)
    {
        STP_LOG_CRITICAL("Calloc Failed, g_stpd_intf_stats");
        sys_assert(0);
    }
    for (i = 0; i < g_max_stp_port; i++)
    {
        g_stpd_intf_stats[i] = calloc(1, sizeof(STPD_INTF_STATS));
        if (!g_stpd_intf_stats[i])
        {
            STP_LOG_CRITICAL("Calloc Failed, g_stpd_intf_stats[%d]", i);
            sys_assert(0);
        }
    }

    return 0;
}

/**
 * @brief Инициализирует пул идентификаторов Port-Channel (PO ID Pool).
 *
 * Функция создаёт и инициализирует пул идентификаторов для управления
 * агрегированными каналами (Port-Channel) в системе STP.
 *
 * @return int
 *         - `0`, если инициализация успешна.
 *         - Отрицательное значение, если произошла ошибка.
 */
int stp_intf_init_po_id_pool()
{
    int ret = 0;
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;

    // Allocate po-id-pool
    ret = bmp_alloc(&stpd_context.po_id_pool, STP_MAX_PO_ID);
    if (-1 == ret)
    {
        STP_LOG_ERR("bmp_alloc Failed");
        return -1;
    }

    // Allocate port-id for all PO's
    avl_t_init(&trav, g_stpd_intf_db);
    while (NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == BAD_PORT_ID && STP_IS_PO_PORT(node->ifname))
        {
            node->port_id = stp_intf_allocate_po_id();
            STP_LOG_INFO("Allocated PO port id %d name %s", node->port_id, node->ifname);
        }
    }
    return 0;
}

/**
 * @brief Инициализирует менеджер событий интерфейсов STP.
 *
 * Функция выполняет начальную настройку и инициализацию менеджера событий,
 * который обрабатывает события, связанные с интерфейсами (например, изменения состояния,
 * добавление или удаление интерфейсов) в контексте STP.
 *
 * @return int
 *         - `0`, если инициализация успешна.
 *         - Отрицательное значение, если произошла ошибка.
 */
int stp_intf_event_mgr_init(void)
{
    struct event *nl_event = 0;

    if ((g_stpd_ioctl_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        sys_assert(0);

    g_max_stp_port = 0;

    /* Open Netlink comminucation to populate Interface DB */
    g_stpd_netlink_handle = stp_netlink_init(&stp_intf_netlink_cb);
    if (-1 == g_stpd_netlink_handle)
    {
        STP_LOG_CRITICAL("netlink init failed");
        sys_assert(0);
    }

    if (stp_netlink_recv_all(g_stpd_netlink_handle) == -1)
    {
        STP_LOG_CRITICAL("error in intf db creation");
        sys_assert(0);
    }

    g_max_stp_port = g_max_stp_port * 2; // Phy Ports + LAG
    STP_LOG_INFO("intf db done. max port %d", g_max_stp_port);

    if (g_max_stp_port == 0)
        return -1;

    stp_intf_init_port_stats();

    if (-1 == stp_intf_init_po_id_pool())
    {
        STP_LOG_CRITICAL("error Allocating port-id for PO's");
        sys_assert(0);
    }

    g_stpd_port_init_done = 1;

    /* Add libevent to monitor interface events */
    nl_event = stpmgr_libevent_create(stp_intf_get_evbase(), stp_intf_get_netlink_fd(), EV_READ | EV_PERSIST, stp_netlink_events_cb, (char *)"NETLINK", NULL);
    if (!nl_event)
    {
        STP_LOG_ERR("Netlink Event create Failed");
        return -1;
    }
}

/**
 * @brief Устанавливает приоритет для указанного порта.
 *
 * Функция обновляет приоритет порта в базе данных STP. Приоритет используется
 * для определения роли порта в процессах выбора корневого моста и топологии.
 *
 * @param port_id Идентификатор порта, для которого устанавливается приоритет.
 * @param priority Значение приоритета порта (должно быть кратно 16 в соответствии со спецификацией STP).
 *
 * @return bool
 *         - `true`, если приоритет успешно установлен.
 *         - `false`, если произошла ошибка (например, порт не найден).
 */
bool stp_intf_set_port_priority(PORT_ID port_id, uint16_t priority)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        node->priority = priority >> 4;
        return true;
    }

    return false;
}

/**
 * @brief Получает текущий приоритет указанного порта.
 *
 * Функция возвращает значение приоритета порта из базы данных STP.
 * Приоритет порта используется для определения его роли в процессах выбора
 * корневого моста и управления топологией.
 *
 * @param port_id Идентификатор порта, для которого требуется получить приоритет.
 *
 * @return uint16_t
 *         - Значение приоритета порта, если порт найден.
 *         - `0`, если порт не найден или произошла ошибка.
 */
uint16_t stp_intf_get_port_priority(PORT_ID port_id)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
        return (node->priority);

    return (STP_DFLT_PORT_PRIORITY >> 4);
}

/**
 * @brief Устанавливает значение стоимости пути (Path Cost) для указанного порта.
 *
 * Функция обновляет значение стоимости пути порта в базе данных STP.
 * Стоимость пути используется в вычислениях протокола STP для выбора
 * корневого моста и определения ролей портов.
 *
 * @param port_id Идентификатор порта, для которого устанавливается стоимость пути.
 * @param path_cost Значение стоимости пути для порта.
 *
 * @return bool
 *         - `true`, если стоимость пути успешно установлена.
 *         - `false`, если произошла ошибка (например, порт не найден).
 */
bool stp_intf_set_path_cost(PORT_ID port_id, uint32_t path_cost)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        node->path_cost = path_cost;
        return true;
    }

    return false;
}

/**
 * @brief Получает текущее значение стоимости пути (Path Cost) для указанного порта.
 *
 * Функция возвращает значение стоимости пути порта из базы данных STP.
 * Стоимость пути используется в вычислениях протокола STP для выбора
 * корневого моста и определения ролей портов.
 *
 * @param port_id Идентификатор порта, для которого требуется получить стоимость пути.
 *
 * @return uint32_t
 *         - Значение стоимости пути порта, если порт найден.
 *         - `0`, если порт не найден или произошла ошибка.
 */
uint32_t stp_intf_get_path_cost(PORT_ID port_id)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
        return (node->path_cost);

    return 0;
}

/**
 * @brief Сбрасывает параметры всех портов STP к значениям по умолчанию.
 *
 * Функция выполняет сброс параметров всех портов, включая приоритет,
 * стоимость пути и состояние, к значениям по умолчанию. Используется
 * для инициализации или очистки конфигурации портов.
 *
 * @return void
 */
void stp_intf_reset_port_params()
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while (NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id != BAD_PORT_ID)
        {
            node->priority = STP_DFLT_PORT_PRIORITY >> 4;
            node->path_cost = stputil_get_path_cost(node->speed, g_stpd_extend_mode);
        }
    }
}
