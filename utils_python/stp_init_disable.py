import socket
import struct

# Параметры IPC сокета
IPC_SOCKET_PATH = "/var/run/stpipc.sock"

# Константы для сообщения
STP_MSG_TYPE = 1  # идентификатор окончания инициализации системы int
MSG_LEN  = 3 #3 байта? uint
OPCODE = 0       # enable/disable uin8_t
MAX_STP_INSTANCES = 16  # количество инстансов для демона uint16_t

def send_stp_ipc_msg(socket_path, type_m,len_m, code, max_i):
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
        # Формат упаковки
        # 'I' - unsigned int (4 байта)
        # 'i' - signed int (4 байта)
        # 'B' - uint8_t (1 байт)
        # 'H' - uint16_t (2 байта)
        # '>' - big ind
        struct_format = "<i I B H"
        message = struct.pack(struct_format, type_m,len_m, code, max_i) 

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
    send_stp_ipc_msg(IPC_SOCKET_PATH, STP_MSG_TYPE, MSG_LEN, OPCODE, MAX_STP_INSTANCES)
