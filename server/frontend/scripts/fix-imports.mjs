#!/usr/bin/env node

import fs from 'fs'
import path from 'path'
import { fileURLToPath } from 'url'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

let fixedCount = 0

function fixImportsInFile (filePath) {
  try {
    const content = fs.readFileSync(filePath, 'utf8')
    let newContent = content

    // Список файлов, которые были переименованы
    const renamedFiles = [
      'stores/app.js',
      'stores/nodes.js',
      'stores/events.js',
      'stores/telemetry.js',
      'stores/errors.js',
      'services/api.js',
      'services/echo.js',
      'services/nodes-api.js',
      'composables/useNodeStatus.js',
      'composables/useOfflineMode.js',
      'composables/useDebounce.js',
      'composables/useVirtualList.js',
      'composables/useResponsive.js',
      'composables/usePullToRefresh.js',
      'router/index.js',
      'plugins/vuetify.js',
      'utils/time.js'
    ]

    let hasChanges = false

    // Заменяем импорты .js на .ts
    for (const oldFile of renamedFiles) {
      const newFile = oldFile.replace('.js', '.ts')

      // Различные варианты импортов
      const patterns = [
        new RegExp(`from ['"]\\./${oldFile}['"]`, 'g'),
        new RegExp(`from ['"]@/${oldFile}['"]`, 'g'),
        new RegExp(`from ['"]\\.\\./${oldFile}['"]`, 'g'),
        new RegExp(`from ['"]\\.\\./\\.\\./${oldFile}['"]`, 'g'),
        new RegExp(`import ['"]\\./${oldFile}['"]`, 'g'),
        new RegExp(`import ['"]@/${oldFile}['"]`, 'g'),
        new RegExp(`import ['"]\\.\\./${oldFile}['"]`, 'g'),
        new RegExp(`import ['"]\\.\\./\\.\\./${oldFile}['"]`, 'g')
      ]

      for (const pattern of patterns) {
        const oldContent = newContent
        newContent = newContent.replace(pattern, (match) => {
          return match.replace(oldFile, newFile)
        })

        if (oldContent !== newContent) {
          hasChanges = true
        }
      }
    }

    if (hasChanges) {
      fs.writeFileSync(filePath, newContent, 'utf8')
      console.log(`✅ Исправлены импорты в: ${filePath}`)
      fixedCount++
    }

  } catch (error) {
    console.error(`❌ Ошибка при обработке ${filePath}:`, error.message)
  }
}

function getAllFiles (dir, extensions = ['.vue', '.ts', '.js']) {
  const files = []

  function traverse (currentDir) {
    const items = fs.readdirSync(currentDir)

    for (const item of items) {
      const fullPath = path.join(currentDir, item)
      const stat = fs.statSync(fullPath)

      if (stat.isDirectory()) {
        if (!['node_modules', '.git', 'dist', 'build', '__tests__'].includes(item)) {
          traverse(fullPath)
        }
      } else if (stat.isFile()) {
        const ext = path.extname(item)
        if (extensions.includes(ext)) {
          files.push(fullPath)
        }
      }
    }
  }

  traverse(dir)
  return files
}

async function main () {
  console.log('🔧 Исправляю импорты после миграции...\n')

  const srcDir = path.join(__dirname, '..', 'src')
  const files = getAllFiles(srcDir)

  files.forEach(file => {
    fixImportsInFile(file)
  })

  console.log('\n✅ Исправление импортов завершено!')
  console.log(`📊 Обработано файлов: ${fixedCount}`)
}

main().catch(console.error)


