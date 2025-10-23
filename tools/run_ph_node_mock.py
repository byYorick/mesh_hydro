#!/usr/bin/env python3
"""
Запуск pH узла с моковыми данными для тестирования PID системы
"""

import subprocess
import sys
import os
import time

def run_command(cmd, cwd=None):
    """Выполняет команду и возвращает результат"""
    print(f"🔧 Выполняем: {cmd}")
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"✅ Успешно: {cmd}")
            if result.stdout:
                print(result.stdout)
        else:
            print(f"❌ Ошибка: {cmd}")
            print(f"STDOUT: {result.stdout}")
            print(f"STDERR: {result.stderr}")
        return result.returncode == 0
    except Exception as e:
        print(f"❌ Исключение: {e}")
        return False

def main():
    print("🚀 Запуск pH узла с моковыми данными")
    print("=" * 50)
    
    # Переходим в директорию проекта
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ph_node_dir = os.path.join(project_root, "node_ph")
    
    if not os.path.exists(ph_node_dir):
        print(f"❌ Директория {ph_node_dir} не найдена")
        return
    
    print(f"📁 Рабочая директория: {ph_node_dir}")
    
    # Проверяем наличие build.bat
    build_script = os.path.join(ph_node_dir, "build.bat")
    if not os.path.exists(build_script):
        print(f"❌ Скрипт сборки {build_script} не найден")
        return
    
    # Собираем проект
    print("\n🔨 Сборка pH узла...")
    if not run_command("build.bat", cwd=ph_node_dir):
        print("❌ Ошибка сборки")
        return
    
    print("\n✅ Сборка завершена успешно!")
    print("\n📋 Инструкции для тестирования:")
    print("1. Подключите ESP32-C3 к компьютеру")
    print("2. Убедитесь, что порт COM доступен")
    print("3. Запустите flash.bat для прошивки")
    print("4. Запустите test_pid_system.py для симуляции данных")
    
    # Предлагаем запустить flash
    response = input("\n❓ Запустить прошивку сейчас? (y/n): ")
    if response.lower() == 'y':
        flash_script = os.path.join(ph_node_dir, "flash.bat")
        if os.path.exists(flash_script):
            print("\n🔌 Прошивка ESP32...")
            run_command("flash.bat", cwd=ph_node_dir)
        else:
            print("❌ Скрипт прошивки не найден")

if __name__ == "__main__":
    main()
