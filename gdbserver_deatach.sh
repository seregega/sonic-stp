#!/bin/bash

# Запуск gdbserver в фоне с nohup и перенаправлением вывода
nohup sudo gdbserver --multi :1234 /home/admin/sonic-stp/stpd > /dev/null 2>&1 &

# Явное отсоединение процесса от терминала (disown)
#disown

# Всегда возвращаем 0
exit 0
