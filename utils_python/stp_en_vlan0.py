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


# typedef struct STP_VLAN_CONFIG_MSG
# {
        # uint8_t opcode; /**< Операция: включение/отключение VLAN. */ // enable/disable
        # uint8_t newInstance;                                         /**< Новый экземпляр VLAN или существующий. */
        # int vlan_id;                                                 /**< Идентификатор VLAN. */
        # int inst_id;                                                 /**< Идентификатор экземпляра STP. */
        # int forward_delay;                                           /**< Задержка пересылки в секундах. */
        # int hello_time;                                              /**< Интервал Hello в секундах. */
        # int max_age;                                                 /**< Максимальный возраст сообщений в секундах. */
        # int priority;                                                /**< Приоритет моста для VLAN. */
        # int count;                                                   /**< Количество портов в списке. */
        # PORT_ATTR port_list[0];                                      /**< Список портов переменной длины. */
    
# } __attribute__((packed)) STP_VLAN_CONFIG_MSG;

# typedef struct PORT_ATTR
# {
        # char intf_name[IFNAMSIZ]; /**< Имя интерфейса. 16c */
        # int8_t mode;              /**< Режим работы порта. */
        # uint8_t enabled;          /**< Статус активности порта. */
    
# } PORT_ATTR;




# Константы для сообщения
STP_MSG_TYPE = 3  # Тип сообщения, соответствующий конфигурации порта
LEN_MSG=52          #msg len for processing
OPCODE = 1          #enable uint8_t 
NEW_INST = 1 
VLAN_ID = 1
INST_ID = 1
FORWARD_DELAY = 1
HELLO_TIME = 2
MAX_AGE =20
PRIORITY = 8
COUNT = 1
PORT_NAME = "Ethernet51"  # Имя порта
MODE = 2          # Режим активации (например, tagged или untagged)
ENABLED = 1

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
        message_format = "<i I B B i i i i i i i {}s B B".format(16)
        message = struct.pack(message_format, STP_MSG_TYPE, LEN_MSG, OPCODE, NEW_INST, VLAN_ID, INST_ID, FORWARD_DELAY, HELLO_TIME, MAX_AGE, PRIORITY, COUNT,  port_name.encode(),MODE, ENABLED)

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
