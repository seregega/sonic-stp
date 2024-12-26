# **Spanning Tree Protocol (STP) Implementation for Network Devices**

## Описание проекта

Этот проект реализует поддержку Spanning Tree Protocol (STP), предназначенного для предотвращения появления петель в сетях Ethernet. Код написан на языке C и предоставляет гибкие возможности для управления состоянием портов, топологическими изменениями и взаимодействием с сетевыми протоколами.

---

## **Основные возможности**

- **Поддержка STP и PVSTP**:
  - Управление состояниями портов (Blocking, Listening, Learning, Forwarding).
  - Обработка BPDU (Bridge Protocol Data Units) сообщений.
  - Выбор корневого моста (Root Bridge) и назначенных портов.

- **Расширенные функции управления**:
  - Управление приоритетами мостов и портов.
  - Поддержка защиты портов: Root Guard и BPDU Guard.
  - Поддержка LAG (Link Aggregation Group).

- **Интеграция с ядром Linux**:
  - Использование Netlink API для взаимодействия с сетевыми интерфейсами.
  - Настройка состояния мостов и портов через ioctl.

- **Интеграция с библиотекой libevent**:
  - Асинхронное управление событиями и таймерами.

- **Расширенные возможности отладки**:
  - Управление режимами отладки через IPC сообщения.
  - Поддержка вывода детальной информации о топологии, VLAN, и портах.

---

## **Структура проекта**

| **Файл**          | **Описание**                                                                                  |
|--------------------|----------------------------------------------------------------------------------------------|
| `stp.c`           | Реализация основных функций STP: обработка BPDU, выбор корневого моста, управление портами.  |
| `stp_mgr.c`       | Менеджер управления конфигурацией STP: настройка VLAN, портов и политики.                     |
| `stp_timer.c`     | Управление таймерами STP, такими как Hello Timer, Forward Delay Timer.                        |
| `stp_netlink.c`   | Интеграция с Netlink API для управления состоянием интерфейсов.                              |
| `stp_pkt.c`       | Обработка входящих и исходящих BPDU сообщений.                                               |
| `stp_debug.c`     | Реализация функций отладки и логирования.                                                    |
| `stp_intf.c`      | Управление базой данных интерфейсов, поддержка LAG и физических портов.                       |

---

### Ключевые структуры
- **`struct avl_table`**: представляет AVL-дерево.
- **`struct avl_node`**: узел AVL-дерева, содержащий указатели на дочерние элементы и баланс-фактор.
- **`struct avl_traverser`**: объект для обхода дерева.

### Основные функции
- **Создание дерева**: `avl_create`
- **Поиск элемента**: `avl_find`
- **Вставка элемента**: `avl_probe`, `avl_replace`
- **Удаление элемента**: `avl_delete`
- **Обход дерева**: `avl_t_first`, `avl_t_next`


## Пример использования

```python
import socket
import struct

# Настройки
SOCKET_PATH = "/var/run/stp.sock"
INTERFACE_NAME = "Ethernet50"

# Сообщение IPC
msg_type = 4  # STP_VLAN_PORT_CONFIG
vlan_id = 1
opcode = 1  # Enable
data = struct.pack(f"=B{len(INTERFACE_NAME)}si", opcode, INTERFACE_NAME.encode(), vlan_id)
msg_len = len(data)
msg = struct.pack(f"=Ii{len(data)}s", msg_type, msg_len, data)

# Отправка
with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as sock:
    sock.connect(SOCKET_PATH)
    sock.send(msg)



## Автор
Картавцев Сергей sergega@me.com
Разработано в BULAT.

