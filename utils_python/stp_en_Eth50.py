import socket
import struct

# Параметры IPC сокета
IPC_SOCKET_PATH = "/var/run/stpipc.sock"

#typedef struct STP_IPC_MSG
#{
#    int msg_type;         /**< Тип сообщения. */
#    unsigned int msg_len; /**< Длина полезной нагрузки (в байтах). */
#    char data[0];         /**< Поле данных переменной длины. */
#} STP_IPC_MSG;

#typedef struct STP_PORT_CONFIG_MSG
#{
#    uint8_t opcode; /**< Операция: включение/отключение порта. */ // enable/disable
#    char intf_name[IFNAMSIZ];                                     /**< Имя интерфейса порта. */
#    uint8_t enabled;                                              /**< Статус порта (включён/выключен). */
#    uint8_t root_guard;                                           /**< Функция Root Guard (включена/выключена). */
#    uint8_t bpdu_guard;                                           /**< Функция BPDU Guard (включена/выключена). */
#    uint8_t bpdu_guard_do_disable;                                /**< Отключение порта при срабатывании BPDU Guard. */
#    uint8_t portfast;                                             /**< Функция PortFast (включена/выключена). */
#    uint8_t uplink_fast;                                          /**< Функция UplinkFast (включена/выключена). */
#    int path_cost;                                                /**< Стоимость пути для порта. */
#    int priority;                                                 /**< Приоритет порта. */
#    int count;                                                    /**< Количество VLAN, связанных с портом. */
#    VLAN_ATTR vlan_list[0];                                       /**< Список VLAN переменной длины. */
#} __attribute__((packed)) STP_PORT_CONFIG_MSG;

#typedef struct VLAN_ATTR
#{
#    int inst_id; /**< Идентификатор экземпляра STP. */
#    int vlan_id; /**< Идентификатор VLAN. */
#    int8_t mode; /**< Режим работы VLAN. */
#} VLAN_ATTR;

# Константы для сообщения
STP_MSG_TYPE = 5  # Тип сообщения, соответствующий конфигурации порта
LEN_MSG=52          #msg len for processing
OPCODE = 1          #enable uint8_t 
PORT_NAME = "Ethernet51"  # Имя порта
ENABLED = 1
ROOT_GUARD = 0
BPDU_GUARD = 0
BPDU_GUARD_DO_DIASABLE = 0
PORTFAST = 1
UPLINK_FAST =1
PATH_COST=800
COUNT = 1
INST_ID = 1         #int
PATH_COST = 800     #int
PRIORITY = 128    #int
INST_ID=1
VLAN_ID=1
MODE = 4          # Режим активации (например, tagged или untagged)

def send_stp_ipc_msg(socket_path, vlan_id, port_name, mode):
    """
    Отправляет сообщение на IPC сокет для активации STP на заданном порту.

    :param socket_path: Путь к сокету IPC.
    :param vlan_id: VLAN ID для активации STP.
    :param port_name: Имя порта (например, Ethernet50).
    :param mode: Режим активации (1 - tagged, 0 - untagged).
    """
    try:
        # Создаем UNIX сокет
        client_socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)

        # Структура сообщения
        # Например: тип сообщения (int), VLAN ID (int), длина имени порта (int), имя порта (string), режим (int)
        message_format = "<i I B {}s B B B B B B i i i i i B".format(16)
        message = struct.pack(message_format, STP_MSG_TYPE, LEN_MSG, OPCODE, port_name.encode(), ENABLED, ROOT_GUARD, BPDU_GUARD, BPDU_GUARD_DO_DIASABLE, PORTFAST,UPLINK_FAST, PATH_COST, PRIORITY, COUNT, INST_ID,VLAN_ID,MODE)

        # Отправляем сообщение
        client_socket.sendto(message, socket_path)
        print("Сообщение отправлено на IPC сокет:", message)

        # Закрываем сокет
        client_socket.close()
    except Exception as e:
        print("Ошибка при отправке IPC сообщения:", e)

if __name__ == "__main__":
    # Проверка, что скрипт запущен напрямую
    print("Запуск отправки IPC сообщения для активации STP...")
    send_stp_ipc_msg(IPC_SOCKET_PATH, ENABLED, PORT_NAME, MODE)
