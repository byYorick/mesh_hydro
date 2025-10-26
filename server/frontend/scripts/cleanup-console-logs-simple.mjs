#!/usr/bin/env node

import fs from 'fs'
import path from 'path'
import { fileURLToPath } from 'url'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

let totalRemoved = 0
let filesProcessed = 0

function removeConsoleLogs (content, filePath) {
  const lines = content.split('\n')
  const newLines = []
  let removed = 0

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i]

    // Пропускаем строки с console.log в комментариях или строках
    if (line.includes('console.log') &&
        !line.trim().startsWith('//') &&
        !line.trim().startsWith('*') &&
        !line.includes('"console.log"') &&
        !line.includes("'console.log'") &&
        !line.includes('`console.log`')) {

      // Проверяем, не является ли это частью многострочного выражения
      const trimmed = line.trim()
      if (trimmed.startsWith('console.log(') && trimmed.endsWith(');')) {
        // Однострочный console.log - удаляем полностью
        removed++
        continue
      } else if (trimmed.startsWith('console.log(')) {
        // Многострочный console.log - нужно проверить следующие строки
        let j = i
        let bracketCount = 0
        let foundEnd = false

        while (j < lines.length && !foundEnd) {
          const currentLine = lines[j]
          for (const char of currentLine) {
            if (char === '(') bracketCount++
            if (char === ')') bracketCount--
          }
          if (bracketCount === 0 && currentLine.includes(';')) {
            foundEnd = true
            break
          }
          j++
        }

        if (foundEnd) {
          // Удаляем весь блок console.log
          removed++
          i = j // Пропускаем все строки до конца console.log
          continue
        }
      }
    }

    newLines.push(line)
  }

  if (removed > 0) {
    console.log(`  📝 ${filePath}: удалено ${removed} console.log`)
    totalRemoved += removed
  }

  return newLines.join('\n')
}

function processFile (filePath) {
  try {
    const content = fs.readFileSync(filePath, 'utf8')
    const newContent = removeConsoleLogs(content, filePath)

    if (content !== newContent) {
      fs.writeFileSync(filePath, newContent, 'utf8')
      filesProcessed++
    }
  } catch (error) {
    console.error(`❌ Ошибка при обработке ${filePath}:`, error.message)
  }
}

function getAllFiles (dir, extensions = ['.vue', '.js', '.ts']) {
  const files = []

  function traverse (currentDir) {
    const items = fs.readdirSync(currentDir)

    for (const item of items) {
      const fullPath = path.join(currentDir, item)
      const stat = fs.statSync(fullPath)

      if (stat.isDirectory()) {
        // Пропускаем node_modules, .git, dist и другие служебные папки
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
  console.log('🧹 Начинаю очистку console.log...\n')

  const srcDir = path.join(__dirname, '..', 'src')
  const files = getAllFiles(srcDir)

  // Исключаем файлы, которые НЕ нужно обрабатывать
  const excludeFiles = [
    'src/utils/logger.ts'
  ]

  const filesToProcess = files.filter(file => {
    const relativePath = path.relative(srcDir, file)
    return !excludeFiles.some(exclude => relativePath.includes(exclude))
  })

  console.log(`📁 Найдено файлов для обработки: ${filesToProcess.length}`)

  filesToProcess.forEach(file => {
    processFile(file)
  })

  console.log('\n✅ Очистка завершена!')
  console.log(`📊 Обработано файлов: ${filesProcessed}`)
  console.log(`🗑️  Удалено console.log: ${totalRemoved}`)
}

main().catch(console.error)


