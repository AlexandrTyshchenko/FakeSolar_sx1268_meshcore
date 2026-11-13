# FakeSolar: що змінено і як зібрати з нуля

Самодостатня інструкція для прошивок FakeSolar: Faketec (ProMicro nRF52840) + E22-400M22S (SX1268, 433 МГц). Опис заліза і команд CLI — у `README.md` поруч.

## Звідки код

| | |
|---|---|
| Репозиторій FakeSolar | https://github.com/AlexandrTyshchenko/FakeSolar_sx1268_meshcore, гілка `main` (публічний) |
| Основа | гілка `dev` https://github.com/meshcore-dev/MeshCore, коміт `a4c66307` (2026-11-13). Вона вже містить новий GPS-код (#3753) з нашим захистом від спуфінгу (`GPS_GUARD`), автовизначенням швидкості GPS, показом `time wait` (#3801), лімітом пошуку до першого fix `gps firstfix` (#3823) і очікуванням 3D fix (#3821). Тривога `ALERT` у MeshCore лише в CLI: канал у телеметрії прибрано (#3813) |
| Патч | `fakesolar.patch` у цій папці: усі зміни FakeSolar відносно `a4c66307`, без готових прошивок |

Попередні версії FakeSolar лишаються в історії гілки `main`: на гілці `gps_refactor` (коміт `544514f5`, реліз `v1.18.0-fakesolar.1`) і з власним GPS-кодом на MeshCore 1.18.0 (коміт `f91d3a89`).

## Що змінено відносно основи

### Радіо (SX1268)

| Файл | Зміна |
|---|---|
| `variants/promicro/target.h` | при `USE_SX1268` підключається `CustomSX1268Wrapper` замість SX1262 |
| `src/helpers/radiolib/CustomSX1268.h` | TCXO лишається увімкненим у standby (`setStandbyXOSC`), тож старт Rx/Tx не чекає запуску TCXO; регістр 0x8B5 (біт 0, RX/AGC) при `SX126X_REGISTER_PATCH` |
| `src/helpers/radiolib/SX126xReset.h` | скидання AGC: якщо калібрування не завершилось, запис регістрів пропускається; після `calibrateImage` пауза 50 мс (як у Meshtastic) |
| `src/Dispatcher.cpp/.h` | періодичне скидання AGC (`agc.reset.interval` > 0) пропускається, якщо за останній інтервал прийнято пакет або в черзі є пакет на передачу; раз на `AGC_FORCED_RESET_MILLIS` (24 год) — примусово, якщо черга передачі порожня |
| `src/helpers/radiolib/RadioLibWrappers.cpp/.h` | рівень шуму не вимірюється перші 50 мс після старту прийому (`NOISE_FLOOR_SETTLE_MILLIS`) |

### Живлення і сенсори

| Файл | Зміна |
|---|---|
| `src/helpers/sensors/EnvironmentSensorManager.cpp/.h` | INA3221: пошук на всіх адресах 0x40–0x43, окремий шунт для кожного каналу (`TELEM_INA3221_SHUNT_CH1..3`), канал батареї (`TELEM_INA3221_BAT_CH`) з ознакою заряджання; у debug-збірці — лог знайдених I²C-адрес |
| `variants/promicro/PromicroBoard.h` | `ADC_MULTIPLIER` можна задати зі збірки; час вибірки АЦП `BATT_ADC_SAMPLE_TIME_US`; напруга батареї та заряджання з INA3221 |

Зміни INA3221 і лог I²C запропоновані в MeshCore: meshcore-dev/MeshCore#3731, #3732, #3727.

### GPS і час

Код GPS — з основи (MeshCore `dev`). FakeSolar лише вмикає його прапорцями (див. нижче) і задає інтервал за замовчуванням:

| Файл | Зміна |
|---|---|
| `src/helpers/CommonCLI.h`, `examples/companion_radio/NodePrefs.h` | `GPS_PERIOD_DEFAULT`: інтервал GPS для нових налаштувань (у FakeSolar 1800 с). Якщо на платі вже збережено інший інтервал, діє збережений; змінюється командою `gps period` |

### CLI

`src/helpers/CommonCLI.cpp`: команди `telemetry` (усі сенсори текстом) і `time force <epoch>` (час можна перевести назад, наприклад після спуфінгу). Ручне виставлення часу скидає прив'язку GPS-часу.

### Конфігурація збірки

`variants/fakesolar_sx1268/platformio.ini` (новий файл): секція `[Faketec_SX1268]` і середовища:

| Середовище | Роль |
|---|---|
| `Faketec_SX1268_repeater` | Repeater |
| `Faketec_SX1268_sensor` | Sensor |
| `Faketec_SX1268_companion_radio_ble` | Companion (BLE, PIN `123456`) |
| `Faketec_SX1268_repeater_debug` | Repeater з логами (`MESH_DEBUG`) |

Основні прапорці:

| Прапорець | Значення |
|---|---|
| `USE_SX1268` | SX1268 замість SX1262 |
| `LORA_FREQ` / `LORA_BW` / `LORA_SF` / `LORA_CR` | 433.650 / 62.5 / 8 / 8 (діють лише при першому запуску) |
| `LORA_TX_POWER` | 22 |
| `SX126X_CURRENT_LIMIT` | 140 |
| `SX126X_RX_BOOSTED_GAIN` | 1 |
| `SX126X_REGISTER_PATCH` | 1 |
| `ENV_INCLUDE_AHTX0`, `ENV_INCLUDE_BME280`, `ENV_INCLUDE_BMP280`, `ENV_INCLUDE_INA3221`, `ENV_INCLUDE_GPS` | сенсори |
| `TELEM_INA3221_ADDRESS` | 0x42 |
| `TELEM_INA3221_SHUNT_CH1/2/3` | 0.1 / 0.04 / 0.04 Ом |
| `TELEM_INA3221_BAT_CH` | 2 |
| `ADC_MULTIPLIER` | 1.5234 (дільник 1.1 МОм / 1.5 МОм) |
| `BATT_ADC_SAMPLE_TIME_US` | 40 |
| `GPS_BAUD_PROBE_LIST` | 9600, 115200, 38400 |
| `GPS_PERIOD_DEFAULT` | 1800 (GPS раз на 30 хв) |
| `GPS_GUARD`, `GPS_GUARD_LAT/LON_MIN/MAX` | захист від спуфінгу; регіон 44.0–52.5 пн. ш., 22.0–40.3 сх. д. |
| GPS | RX=3, TX=4, EN=5 |
| I²C | SDA=8, SCL=7 |
| Кнопка | пін 6 |

Repeater і sensor мають пароль адміністратора за замовчуванням `password`; його треба змінити після прошивки.

## Збірка з нуля (Windows)

Перевірено з Python 3.13.7, PlatformIO Core 6.2.0, Git для Windows (Git Bash).

### 1. Інструменти

1. Встановити Git: https://git-scm.com/download/win
2. Встановити Python 3: https://www.python.org/downloads/ (галочка «Add python.exe to PATH»).
3. Встановити PlatformIO Core:
   ```bash
   python -m pip install -U platformio
   ```
   Якщо `pio` не знаходиться, він лежить у `%USERPROFILE%\.platformio\penv\Scripts\pio.exe` (для варіанта з VS Code) або запускається як `python -m platformio`.

### 2. Код: варіант А, репозиторій FakeSolar

```bash
git clone https://github.com/AlexandrTyshchenko/FakeSolar_sx1268_meshcore.git MeshCore
cd MeshCore
```

### 2. Код: варіант Б, основа + патч

Якщо репозиторій FakeSolar недоступний:

```bash
git clone https://github.com/meshcore-dev/MeshCore.git
cd MeshCore
git checkout a4c66307
git apply --check /шлях/до/fakesolar.patch
git apply /шлях/до/fakesolar.patch
```

`--check` нічого не змінює, лише перевіряє, що патч лягає без помилок.

### 3. Збірка

```bash
pio run -e Faketec_SX1268_repeater -e Faketec_SX1268_sensor -e Faketec_SX1268_companion_radio_ble
```

Перший запуск сам завантажує платформу nRF52, тулчейн і бібліотеки (кілька хвилин). Кожна збірка займає близько хвилини. Наприкінці має бути `SUCCESS` для кожного середовища.

Результат лежить у `.pio/build/<середовище>/`:
- `firmware.zip` — для DFU по USB, веб-flasher або OTA;
- `firmware.hex` — для програматора.

### 4. UF2

PlatformIO не робить `.uf2` для цих середовищ, тому його треба зробити скриптом з репозиторію:

```bash
python bin/uf2conv/uf2conv.py .pio/build/Faketec_SX1268_repeater/firmware.hex -c -f 0xADA52840 -o Faketec_SX1268_repeater.uf2
```

Те саме для `Faketec_SX1268_sensor` і `Faketec_SX1268_companion_radio_ble`. `0xADA52840` — ідентифікатор сімейства nRF52840 для загрузчика Adafruit.

### 5. Прошивка

- **UF2:** двічі швидко натиснути reset, з'явиться USB-диск, скопіювати на нього `.uf2`. Плата перезавантажиться сама.
- **ZIP по OTA (repeater/sensor):** у CLI ноди виконати `start ota`, відповідь `OK - mac: ...`. Далі в nRF Device Firmware Update / nRF Connect на телефоні вибрати пристрій за MAC і `.zip`.

Налаштування (ім'я, пароль, ключі, радіо) після оновлення зберігаються. Інтервал GPS береться зі збережених налаштувань: на платі, де раніше був постійний режим, виконати `gps period 30m`.

### 6. Перевірка після прошивки

```
ver
get radio          433.65,62.5,8,8
stats-radio        рівень шуму, лічильники пакетів
telemetry          показники сенсорів
power              батарея / заряджання
gps                on, ..., periodic (1800s)
gps guard          стан захисту GPS
```

## Що перевірено

На платі FakeSolar (repeater, debug-збірка):
- GPS на 38400 знайдено автоматично;
- AHT20, BMP280, INA3221 знайдено; батарея з INA3221 CH2, ознака заряджання;
- `gps period 30m`: GPS вимикається після fix і чекає 30 хв;
- захист GPS на реальному спуфінгу (позиція за ~12 000 км): усі такі fix відкинуто, час не змінено;
- `gps fence` і `gps guard on/off` зберігаються після перезавантаження.

Перевірки вище зроблено на попередній основі (`gps_refactor`, `544514f5`). На основі `dev` `ef4477b6` (2026-11-11, repeater і repeater debug) повторно перевірено:
- радіо, сенсори, батарея з INA3221, ознака заряджання;
- автовизначення швидкості GPS, інтервальний режим 30 хв;
- реальний спуфінг (знову позиція в Перу, до 12 супутників): усі fix відкинуто (`rej region`), час не прийнято (`time rej nofix`);
- годинник, що відставав на 16 хв: GPS не виправив його сам (`time rej step 976s`, більше 10 хв), після `time force` час GPS збігається з годинником (`time ok +0s`), а між вмиканнями GPS показується `time wait`.
На основі `dev` `c4258a5b` (2026-11-11, repeater): радіо і сенсори; `get/set advert.loc` і стара `gps advert`; `telemetry` без каналу тривоги; спуфінг, що саме йшов, відкинуто (`rej region`, `time rej nofix`).
На основі `dev` `a4c66307` (2026-11-14, repeater): налаштування після оновлення збереглися (`gps period` 1800); `gps firstfix` показує й змінює ліміт, значення переживає перезавантаження; `gps diag` показує `fm:1` → `fm:3`; під час спуфінгу fix відкинуто (`rej region`), GPS лишається ввімкненим, поки діє ліміт першого fix. Шлях 2D fix на залізі не перевірено (2D fix тут не було).

## Що не перевірено

- Тривога `ALERT` на справжньому залізі (годинник виправлено вручну, тривоги не чекали).
- Sensor і companion на залізі (лише збірка).
- Збірка на чистій машині: бібліотеки PlatformIO бралися з уже завантаженого кешу. Реєстр PlatformIO інколи відповідає «Download limit exceeded»: тоді треба зачекати й повторити `pio run`.
