**Taski** — это легковесная утилита командной строки для управления локальным окружением разработки в Windows. Забудьте о десятках открытых терминалов и сложных батниках. Taski позволяет описать все необходимые фоновые сервисы (БД, очереди, веб-серверы) и задачи (миграции, билды, сидеры) в одном декларативном файле `environment.yml`.

Все, что нужно — запустить утилиту, и она поднимет сервисы в правильном порядке, отследит их состояние и предоставит удобное меню для выполнения скриптов.

## 🔥 Ключевые возможности

*   **Декларативное окружение**: Описывайте сервисы и задачи в простом YAML-файле.
*   **Умный запуск сервисов**: Автоматическое разрешение зависимостей (`dependsOn`). Сервисы запускаются в правильном порядке.
*   **Мониторинг состояния**: Утилита отслеживает, запущен ли процесс, и показывает его статус (`ps`). Поддержка мониторинга как по имени процесса, так и по аргументам командной строки.
*   **Гибкое управление жизненным циклом**: Поддержка нативных `stopCommand` для корректной остановки (например, `pg_ctl stop`) или автоматическая отправка `SIGTERM`.
*   **Удобная обертка для задач**: Запускайте shell-команды, объединяйте их в цепочки, управляйте зависимостями и скрывайте служебные задачи.
*   **Шаблонизация**: Используйте [Inja](https://github.com/pantor/inja) для шаблонов в конфигурации, что позволяет легко адаптировать окружение под разные машины.
*   **Поддержка `.env`**: Чувствительные данные и локальные пути можно вынести в `.env`-файл, который подхватывается автоматически.
*   **Интерактивное меню**: Просто запустите `cli menu`, чтобы получить удобный TUI для управления всем окружением.

---

## 📦 Установка

1. Скачайте последнюю версию из [Releases](https://github.com/ogetmanskii/taski/releases).
2. Поместите `cli.exe` в директорию, которая добавлена в системный `PATH`, или используйте полный путь к файлу.

## 🚀 Быстрый старт за 60 секунд

1.**Создайте директорию** для вашего проекта и перейдите в неё:

```bash
mkdir my-backend-project
cd my-backend-project
```

2. **Создайте файл** `.env` для ваших локальных путей и секретов:

```dotenv
# .env
PG_USER=myuser
PG_PASSWORD=s3cret
REDIS_HOME=C:\Tools\Redis
```

3. **Создайте файл** `environment.yml` с описанием сервисов и задач. Простейший пример для PostgreSQL:

```yaml
# environment.yml
services:
   postgres:
      name: "PostgreSQL 16"
      startCommand: pg_ctl start -D data -l logfile.txt
      stopCommand: pg_ctl stop -D data
      monitorProcess: postgres.exe
      detachAfterMessage: "database system is ready to accept connections"
    
tasks:
   create-db:
      name: "Create dev database"
      command: createdb -U ${PG_USER} myapp_dev
```

4. **Запустите окружение**:

```bash
cli -d my-backend-project
# или
cli menu
```

Готово! В открывшемся интерактивном меню вы сможете запустить PostgreSQL, а затем выполнить задачу по созданию базы данных.

## ⚙️ Справочник по конфигурации

### Секция `services:`

Определяет долгоиграющие фоновые процессы.

| Ключ | Тип | Обязательный | Описание |
| --- | --- | --- | --- |
| `name` | string | Нет | Человеко-читаемое имя для меню и логов. По умолчанию — ключ из YAML. |
| `utf8` | bool | Нет | Кодировать ли вывод в UTF-8. По умолчанию `true`. |
| `dependsOn` | list | Нет | Список сервисов, которые должны быть запущены перед этим. |
| `workingDirectory` | string | Нет | Рабочая директория для команды запуска. |
| `environment` | map | Нет | Дополнительные переменные окружения. |
| `startCommand` | string | **Да** | Команда для запуска сервиса. |
| `detachAfterSeconds` | int | Нет | Отсоединиться от процесса через N секунд. |
| `detachAfterMessage` | string | Нет | Отсоединиться, когда в выводе появится фраза. |
| `monitorProcess` | string/object | Нет | Процесс для мониторинга. Если не указан, сервис можно только запустить. |
| `stopCommand` | string | Нет | Команда для остановки. Если не указана, посылается `SIGTERM` всем найденным `monitorProcess`. |

### Секция `tasks:`

Определяет конечные скрипты для автоматизации.

| Ключ | Тип | Обязательный | Описание |
| --- | --- | --- | --- |
| `name` | string | Нет | Отображаемое имя. |
| `hidden` | bool | Нет | Скрывает задачу из интерактивного меню (полезно для `dependsOn`). |
| `dependsOn` | list | Нет | Задачи, которые нужно выполнить перед этой **один раз за сессию**. |
| `before` | list | Нет | Задачи, которые **всегда** выполняются перед этой. |
| `after` | list | Нет | Задачи, которые **всегда** выполняются после этой. |
| `command` | string/list | **Да** | Команда или список команд (объединяются через `&&`). |
| `workingDirectory` | string | Нет | Рабочая директория. |
| `exitCodes` | list | Нет | Список кодов возврата, которые считать успехом (например, `[0, 1]`). |
| `environment` | map | Нет | Переменные окружения. |
| `utf8` | bool | Нет | Управление кодировкой вывода. |

## 💻 Интерфейс командной строки

```text
cli.exe [OPTIONS] [SUBCOMMANDS]

OPTIONS:
  -h, --help              Показать эту справку
  -d, --dir TEXT:DIR      Установить рабочую директорию (по умолчанию текущая)
  -f, --file TEXT:FILE    Имя файла конфигурации (по умолчанию environment.yml)
  -e, --env TEXT:FILE     Имя dot env файла (по умолчанию .env)
  -v, --version           Показать версию утилиты

SUBCOMMANDS:
  list                    Вывести список всех сервисов и задач
  up                      Запустить все сервисы
  down                    Остановить все запущенные сервисы
  run <task_name>         Запустить конкретную задачу
  ps                      Показать статус всех сервисов
  menu                    Показать интерактивное меню управления
```

## Зачем этот проект?

В экосистеме Windows долгое время не хватало простого инструмента, похожего на docker-compose для нативных процессов или на Makefile/Taskfile с удобным управлением демонами. Taski закрывает эту нишу, позволяя разработчикам:

- Быстро разворачивать сложное окружение для проектов, завязанных на Windows-инструменты.

- Иметь единый способ запуска всех утилит в команде (`cli run build` вместо `build.bat`, `build.ps1`, `npm run build`).

- Не держать в голове (и в автозагрузке) кучу программ, а запускать их одним нажатием в консольном меню, только когда это нужно.

## Примеры использования

### Классический веб-бэкенд (Go + PostgreSQL + Redis)

Сценарий: Разработчик пишет бэкенд на Go, которому нужны PostgreSQL для основных данных и Redis для кеширования и очередей. Он устал каждый раз вручную запускать БД перед стартом API.

`.env`:
```dotenv
# Локальные пути к портативным версиям БД
PG_HOME=C:\DevTools\pgsql\16
REDIS_HOME=C:\DevTools\Redis

# Общие настройки
PG_PORT=5432
REDIS_PORT=6379
```

`environment.yml`:
```yaml
{% set pg_data = "C:\\DevData\\myapp_pg" %}
{% set redis_data = "C:\\DevData\\myapp_redis" %}

services:
   # 1. Сначала запускаем БД и Кеш
   postgres:
      name: "PostgreSQL 16 (Main DB)"
      workingDirectory: ${PG_HOME}/bin
      startCommand: pg_ctl.exe start -D ${pg_data} -l ${pg_data}/logfile.txt -o "-p ${PG_PORT}"
      stopCommand: pg_ctl.exe stop -D ${pg_data}
      monitorProcess: 
         executable: ${PG_HOME}/bin/postgres.exe
         args: "* -D ${pg_data} *"
      detachAfterMessage: "database system is ready to accept connections"
      environment:
         PGPORT: ${PG_PORT}

   redis:
      name: "Redis 7 (Cache & Queue)"
      workingDirectory: ${REDIS_HOME}
      startCommand: redis-server.exe --port ${REDIS_PORT}
      monitorProcess: redis-server.exe
      detachAfterSeconds: 3

   # 2. API-сервер зависит от готовности БД и Кеша
   api-server:
      name: "Go API Server (Air Live Reload)"
      dependsOn:
         - postgres
         - redis
   # Запускаем не "намертво", а через Air для hot-reload при изменениях кода
   workingDirectory: .\cmd\api
   startCommand: air.exe
   monitorProcess: api.exe
   detachAfterSeconds: 5
   environment:
      DATABASE_URL: "postgres://myuser:mypass@localhost:${PG_PORT}/myapp_dev?sslmode=disable"
      REDIS_URL: "redis://localhost:${REDIS_PORT}/0"

tasks:
   # Задачи для управления БД
   db-create:
      name: "DB: Create"
      command: createdb -U postgres -p ${PG_PORT} myapp_dev
      exitCodes: [0, 1] # Ошибка если БД уже есть - не страшно

   db-migrate:
      name: "DB: Run Migrations"
      dependsOn: [db-create] # Миграции имеет смысл выполнять только если база создается в этой сессии
      workingDirectory: .\migrations
      command: go run main.go up

   db-seed:
      name: "DB: Import Fixtures"
      workingDirectory: .\cmd\seeder
      command: go run main.go

   # Универсальная задача для запуска всего окружения "под ключ"
   dev-init:
      name: "Full Dev Init (DB + Seed)"
      dependsOn:
         - db-migrate
      before:
         - db-seed # Сиды всегда обновляем перед стартом, даже если миграции уже были
```

### Стек для фронтенда (Nginx + Node.js SSR)

Сценарий: Проект, где фронтенд раздаётся через Nginx с обратным проксированием на Node.js сервер, который занимается Server-Side Rendering (SSR). Разработчик хочет одной командой стартовать всю связку и видеть статус.

`.env`:
```dotenv
# Путь к portable-версии Nginx
NGINX_HOME=C:\Tools\nginx-1.25
NODE_HOME=C:\Program Files\nodejs
```

`environment.yml`:
```yaml
services:
   ssr-server:
   name: "Node.js SSR Server (Next.js/Nuxt)"
   workingDirectory: .
   startCommand: npm run dev
   monitorProcess: node.exe
   detachAfterMessage: "ready - started server on" # Сообщение от фреймворка
   environment:
      NODE_ENV: development
      PORT: 3000

   nginx:
      name: "Nginx Reverse Proxy"
      dependsOn:
         - ssr-server # Ждем, пока не стартанет SSR-сервер
      workingDirectory: ${NGINX_HOME}
      startCommand: nginx.exe -c conf/nginx.conf
      stopCommand: nginx.exe -s quit
      monitorProcess: nginx.exe
      detachAfterSeconds: 2

tasks:
   install-deps:
      name: "npm: Install Dependencies"
      workingDirectory: .
      command: npm install

   build:
      name: "npm: Build Project"
      dependsOn: [install-deps]
      workingDirectory: .
      command: npm run build

   # Задача для открытия браузера после старта всего
   open-browser:
      name: "Open App in Browser"
      command: start http://localhost
```

### Утилита для дата-инженера (Python ETL с MongoDB)

Сценарий: Аналитик или дата-инженер запускает у себя MongoDB и набор Python-скриптов, которые забирают данные из API, обрабатывают и складывают в базу. Он хочет управлять сложным порядком запуска задач.

`.env`:
```dotenv
# Данные для подключения к источникам (чтобы не светить в yml)
SOURCE_API_KEY=prod-key-12345
MONGO_HOME=C:\Tools\mongodb
PYTHON_EXE=C:\Users\Analyst\.virtualenvs\etl_pipeline\Scripts\python.exe
```

`environment.yml`:
```yaml
services:
   mongodb:
      name: "MongoDB 7"
      workingDirectory: ${MONGO_HOME}/bin
      startCommand: mongod.exe --dbpath C:\Data\mongo_dev --port 27017
      monitorProcess: mongod.exe
      detachAfterMessage: "Waiting for connections"

tasks:
   # Задачи разделены на этапы. Зависимости гарантируют порядок.
   01-pull-raw:
      name: "[1] Pull Raw Data"
      workingDirectory: .\etl
      command: ${PYTHON_EXE} pull_data.py --date yesterday
      environment:
         API_KEY: ${SOURCE_API_KEY}

   02-transform:
      name: "[2] Transform & Clean"
      dependsOn:
         - 01-pull-raw
      workingDirectory: .\etl
      command: ${PYTHON_EXE} transform.py

   03-load-to-mongo:
      name: "[3] Load to MongoDB"
      dependsOn:
         - 02-transform
      workingDirectory: .\etl
      command: ${PYTHON_EXE} load_to_mongo.py --uri mongodb://localhost:27017

   04-generate-report:
      name: "[4] Generate CSV Report"
      dependsOn:
         - 03-load-to-mongo
      workingDirectory: .\reports
      command: ${PYTHON_EXE} generate_report.py --output ./output/report.csv

   # Главная задача, которую запускает аналитик
   run-full-pipeline:
      name: "Run FULL ETL Pipeline"
      dependsOn:
         - 04-generate-report # Тянет за собой всё цепочку [1] -> [2] -> [3] -> [4]
      after:
         - open-report-dir # После завершения пайплайна открыть папку с отчетом

   open-report-dir:
      hidden: true
      command: explorer.exe .\reports\output
```
