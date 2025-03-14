#!/bin/bash

# Настройки
BINARY="/home/admin/sonic-stp/stpd"
PORT=1234
LOG_FILE="./gdbserver.log"

# Функция запуска gdbserver
start_gdbserver() {
    echo "Starting gdbserver for $BINARY on port $PORT..."
    killall -q gdbserver /home/admin/sonic-stp/stpd  # Убить предыдущие процессы
    gdbserver --multi :$PORT "$BINARY" &>> "$LOG_FILE" &
}

# Мониторинг изменений бинарника
start_gdbserver
while inotifywait -q -e modify,attrib "$BINARY"; do
    echo "Binary changed! Restarting gdbserver..."
    start_gdbserver
done
