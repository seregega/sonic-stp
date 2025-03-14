#!/bin/bash

# Пути и настройки
BINARY="./stpd"
GDBINIT="./.gdbinit_auto"
PORT=1234

# Генерация временного .gdbinit
cat > "$GDBINIT" <<EOF
target extended-remote :$PORT
set confirm off
break main
continue
EOF

# Запуск GDB с настройками
gdb -x "$GDBINIT" "$BINARY"

# Удаление временного файла при завершении
rm -f "$GDBINIT"
