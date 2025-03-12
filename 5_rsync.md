//команда для деплоя бинарников из репы виртуалки
rsync -avzhP  stp-maker:/home/pi/sonic-stp/ /home/admin/sonic-stp/
rsync -avzhP  stp-maker:/home/pi/sonic-stp/ /home/admin/sonic-stp/ --exclude '.git'  --dry-run
rsync -avzhP /home/admin/sonic-stp/  stp-maker:/home/pi/sonic-stp/  --exclude '.git' 
