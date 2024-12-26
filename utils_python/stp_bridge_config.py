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


#  typedetruct STP_BRIDGE_CONFIG_MSG
# {
        # uint8_t opcode; /**< Операция: включение/отключение STP. */ // enable/disable
        # uint8_t stp_mode;                                           /**< Режим работы STP. 1-RSTP*/
        # int rootguard_timeout;                                      /**< Тайм-аут защиты корневого моста (в секундах). */
        # uint8_t base_mac_addr[6];                                   /**< Базовый MAC-адрес моста. */
    
# } __attribute__((packed)) STP_BRIDGE_CONFIG_MSG;PORT_ATTR



# Константы для сообщения
STP_MSG_TYPE = 2  # Тип сообщения, соответствующий конфигурации порта
LEN_MSG=52          #msg len for processing
OPCODE = 1          #enable uint8_t 
MODE = 1          # Режим активации (например, tagged или untagged)
TIMEOUT = 60
ENABLED = 1
M1 = 0x98 
M2 = 0x19
M3 = 0x2C
M4 = 0x21
M5 = 0x2C
M6 = 0x20
PORT_NAME = "AAA" 

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
        message_format = "<i I B B i B B B B B B"
        message = struct.pack(message_format, STP_MSG_TYPE, LEN_MSG, OPCODE, MODE, TIMEOUT, M1, M2, M3, M4, M5, M6)

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
