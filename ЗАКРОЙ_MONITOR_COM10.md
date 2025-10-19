# ⚠️ ЗАКРОЙ MONITOR ДЛЯ ПРОШИВКИ!

**Проблема:** COM10 занят - невозможно прошить

---

## 🚨 ОШИБКА

```
Serial port COM10
A fatal error occurred: Could not open COM10, the port is busy or doesn't exist.
```

### Причина:
**У тебя открыт `idf.py monitor` на порту COM10!**

---

## ✅ РЕШЕНИЕ

### 1. Закрой monitor:
В окне терминала где открыт monitor нажми:
```
Ctrl + ]
```

Или просто закрой окно терминала.

---

### 2. Прошей climate узел:
```batch
C:\Windows\system32\cmd.exe /c "cd /d C:\esp\hydro\mesh\mesh_hydro\node_climate && C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005 && idf.py -p COM10 flash"
```

---

### 3. После прошивки запусти monitor:
```batch
C:\Windows\system32\cmd.exe /c "cd /d C:\esp\hydro\mesh\mesh_hydro\node_climate && C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005 && idf.py -p COM10 monitor"
```

---

## 📋 ПОРЯДОК ДЕЙСТВИЙ

### Шаг 1: Закрой все monitor окна
- COM7 (root)
- COM10 (climate)
- COM9 (ph_ec)

### Шаг 2: Прошей climate (COM10)
```batch
cd node_climate
C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
idf.py -p COM10 flash
```

### Шаг 3: Подожди 10 секунд

### Шаг 4: Проверь MAC в БД
```bash
docker exec hydro_backend php artisan tinker --execute="echo App\Models\Node::where('node_id', 'climate_001')->first()->mac_address;"
```

**Должно быть:**
```
00:4B:12:37:D5:A4
```

---

**ЗАКРОЙ MONITOR И ПРОШЕЙ СНОВА!** 🔌

