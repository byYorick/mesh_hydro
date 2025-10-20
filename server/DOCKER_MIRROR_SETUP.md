# Настройка Docker зеркала для обхода проблем с Docker Hub

## Проблема
Docker Hub периодически недоступен (503 ошибки), что блокирует сборку контейнеров.

## Решение: Настройка зеркала

### Windows (Docker Desktop)
1. Скопируйте `docker-daemon.json` в `C:\ProgramData\Docker\config\daemon.json`
2. Перезапустите Docker Desktop
3. Проверьте: `docker info | findstr "Registry Mirrors"`

### Linux
```bash
sudo cp docker-daemon.json /etc/docker/daemon.json
sudo systemctl restart docker
docker info | grep "Registry Mirrors"
```

### Альтернативные зеркала
Если `mirror.gcr.io` не работает, попробуйте:
- `https://docker.mirrors.ustc.edu.cn`
- `https://hub-mirror.c.163.com`
- `https://mirror.baidubce.com`

## Проверка работы
```bash
docker pull postgres:15.4-alpine
docker pull eclipse-mosquitto:2.0.18
docker pull nginx:1.27-alpine
```

## Зафиксированные теги
- `postgres:15.4-alpine` (вместо `postgres:15-alpine`)
- `eclipse-mosquitto:2.0.18` (вместо `eclipse-mosquitto:2.0`)
- `nginx:1.27-alpine` (вместо `nginx:alpine`)

Это обеспечивает стабильность сборки и предсказуемые версии.
