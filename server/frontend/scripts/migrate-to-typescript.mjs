#!/usr/bin/env node

import fs from 'fs'
import path from 'path'
import { fileURLToPath } from 'url'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

// Файлы для миграции (в порядке приоритета)
const filesToMigrate = [
  'src/stores/app.js',
  'src/stores/nodes.js',
  'src/stores/events.js',
  'src/stores/telemetry.js',
  'src/stores/errors.js',
  'src/services/api.js',
  'src/services/echo.js',
  'src/services/nodes-api.js',
  'src/composables/useNodeStatus.js',
  'src/composables/useOfflineMode.js',
  'src/composables/useDebounce.js',
  'src/composables/useVirtualList.js',
  'src/composables/useResponsive.js',
  'src/composables/usePullToRefresh.js',
  'src/router/index.js',
  'src/plugins/vuetify.js',
  'src/utils/time.js'
]

let migratedCount = 0
let errorCount = 0

function migrateFile (filePath) {
  try {
    const fullPath = path.join(__dirname, '..', filePath)

    if (!fs.existsSync(fullPath)) {
      console.log(`⚠️  Файл не найден: ${filePath}`)
      return
    }

    const content = fs.readFileSync(fullPath, 'utf8')
    const newPath = filePath.replace('.js', '.ts')
    const newFullPath = path.join(__dirname, '..', newPath)

    // Базовые преобразования
    let newContent = content

    // 1. Добавляем типы для Vue 3 Composition API
    if (filePath.includes('stores/') || filePath.includes('composables/')) {
      // Добавляем импорт типов если их нет
      if (!newContent.includes('import type')) {
        newContent = `import type { Ref, ComputedRef } from 'vue'\n${  newContent}`
      }
    }

    // 2. Добавляем типы для Pinia stores
    if (filePath.includes('stores/')) {
      if (!newContent.includes('import type') && !newContent.includes('import { defineStore')) {
        newContent = `import type { Ref, ComputedRef } from 'vue'\n${  newContent}`
      }
    }

    // 3. Добавляем типы для API
    if (filePath.includes('services/')) {
      if (!newContent.includes('import type')) {
        newContent = `import type { AxiosResponse, AxiosError } from 'axios'\n${  newContent}`
      }
    }

    // 4. Заменяем export default на типизированные экспорты
    newContent = newContent.replace(
      /export default\s+function\s+(\w+)/g,
      'export function $1'
    )

    // 5. Добавляем типы для параметров функций
    newContent = newContent.replace(
      /function\s+(\w+)\s*\(\s*([^)]*)\s*\)/g,
      (match, funcName, params) => {
        if (params.trim()) {
          return `function ${funcName}(${params}): any`
        }
        return `function ${funcName}(): any`
      }
    )

    // 6. Добавляем типы для переменных
    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*ref\s*\(/g,
      'const $1 = ref<any>('
    )

    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*computed\s*\(/g,
      'const $1 = computed<any>('
    )

    // 7. Добавляем типы для объектов
    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*\{/g,
      'const $1: any = {'
    )

    // 8. Добавляем типы для массивов
    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*\[/g,
      'const $1: any[] = ['
    )

    // Записываем новый файл
    fs.writeFileSync(newFullPath, newContent, 'utf8')

    // Удаляем старый файл
    fs.unlinkSync(fullPath)

    console.log(`✅ Мигрирован: ${filePath} → ${newPath}`)
    migratedCount++

  } catch (error) {
    console.error(`❌ Ошибка при миграции ${filePath}:`, error.message)
    errorCount++
  }
}

async function main () {
  console.log('🔄 Начинаю миграцию JS → TypeScript...\n')

  for (const file of filesToMigrate) {
    migrateFile(file)
  }

  console.log('\n✅ Миграция завершена!')
  console.log(`📊 Успешно мигрировано: ${migratedCount}`)
  console.log(`❌ Ошибок: ${errorCount}`)

  if (errorCount === 0) {
    console.log('\n🎉 Все файлы успешно мигрированы в TypeScript!')
  }
}

main().catch(console.error)


