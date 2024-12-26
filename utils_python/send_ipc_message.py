import socket
import struct

# Константы
SOCKET_PATH = "/var/run/stpipc.sock"  # Путь к сокету IPC-сервера

# Пример типов команд (должны соответствовать вашим определениям в коде)
ENABLE_STP = 1
DISABLE_STP = 2


# Структура IPC-сообщения
# Пример структуры: тип команды (1 байт), VLAN ID (2 байта)
def create_ipc_message(command, vlan_id):
    """
    Создаёт сообщение IPC для отправки.

    :param command: Тип команды (например, ENABLE_STP или DISABLE_STP).
    :param vlan_id: Идентификатор VLAN.
    :return: Бинарное сообщение для отправки.
    """
    return struct.pack("!BH", command, vlan_id)


def send_ipc_message(command, vlan_id):
    """
    Отправляет IPC-сообщение в сокет сервера.

    :param command: Тип команды (например, ENABLE_STP или DISABLE_STP).
    :param vlan_id: Идентификатор VLAN.
    """
    try:
        # Создание сокета
        client_socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)

        # Подключение к сокету сервера
        client_socket.connect(SOCKET_PATH)

        # Создание сообщения
        message = create_ipc_message(command, vlan_id)

        # Отправка сообщения
        client_socket.sendall(message)
        print(f"Сообщение отправлено: команда={command}, vlan_id={vlan_id}")

        # Получение ответа (опционально)
        response = client_socket.recv(1024)
        print(f"Ответ от сервера: {response.decode('utf-8')}")

    except FileNotFoundError:
        print(f"Сокет сервера не найден: {SOCKET_PATH}")
    except Exception as e:
        print(f"Ошибка при отправке сообщения: {e}")
    finally:
        client_socket.close()


# Пример использования
if __name__ == "__main__":
    # Отправка команды для включения STP на VLAN 100
    send_ipc_message(ENABLE_STP, 100)

    # Отправка команды для отключения STP на VLAN 200
    send_ipc_message(DISABLE_STP, 200)
