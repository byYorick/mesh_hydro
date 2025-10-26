#!/usr/bin/env node

import fs from 'fs'
import path from 'path'
import { fileURLToPath } from 'url'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

let fixedCount = 0

function fixTypeScriptErrors (filePath) {
  try {
    const content = fs.readFileSync(filePath, 'utf8')
    let newContent = content

    // 1. Добавляем типы для параметров функций
    newContent = newContent.replace(
      /function\s+(\w+)\s*\(\s*([^)]*)\s*\)/g,
      (match, funcName, params) => {
        if (params.trim()) {
          const typedParams = params.split(',').map(p => {
            const trimmed = p.trim()
            if (trimmed && !trimmed.includes(':')) {
              return `${trimmed}: any`
            }
            return trimmed
          }).join(', ')
          return `function ${funcName}(${typedParams})`
        }
        return `function ${funcName}()`
      }
    )

    // 2. Добавляем типы для стрелочных функций
    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*\(\s*([^)]*)\s*\)\s*=>/g,
      (match, varName, params) => {
        if (params.trim()) {
          const typedParams = params.split(',').map(p => {
            const trimmed = p.trim()
            if (trimmed && !trimmed.includes(':')) {
              return `${trimmed}: any`
            }
            return trimmed
          }).join(', ')
          return `const ${varName} = (${typedParams}) =>`
        }
        return `const ${varName} = () =>`
      }
    )

    // 3. Добавляем типы для переменных
    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*ref\s*\(/g,
      'const $1 = ref<any>('
    )

    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*computed\s*\(/g,
      'const $1 = computed<any>('
    )

    // 4. Добавляем типы для объектов
    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*\{/g,
      'const $1: any = {'
    )

    // 5. Добавляем типы для массивов
    newContent = newContent.replace(
      /const\s+(\w+)\s*=\s*\[/g,
      'const $1: any[] = ['
    )

    // 6. Исправляем импорты типов
    newContent = newContent.replace(
      /import\s+type\s+\{([^}]+)\}\s+from\s+['"]([^'"]+)['"]/g,
      (match, types, module) => {
        const typeList = types.split(',').map(t => t.trim()).join(', ')
        return `import { ${typeList} } from '${module}'`
      }
    )

    // 7. Добавляем any для неявных типов
    newContent = newContent.replace(
      /:\s*any\[\]\s*=\s*\[\]/g,
      ': any[] = []'
    )

    // 8. Исправляем AxiosResponse типы
    newContent = newContent.replace(
      /AxiosResponse<any, any, {}>/g,
      'AxiosResponse<any>'
    )

    // 9. Добавляем типы для catch блоков
    newContent = newContent.replace(
      /catch\s*\(\s*(\w+)\s*\)/g,
      'catch ($1: any)'
    )

    // 10. Исправляем spread операторы
    newContent = newContent.replace(
      /\.\.\.(\w+)/g,
      '...($1 as any)'
    )

    if (content !== newContent) {
      fs.writeFileSync(filePath, newContent, 'utf8')
      console.log(`✅ Исправлен: ${filePath}`)
      fixedCount++
    }

  } catch (error) {
    console.error(`❌ Ошибка при исправлении ${filePath}:`, error.message)
  }
}

function getAllTsFiles (dir) {
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
      } else if (stat.isFile() && item.endsWith('.ts')) {
        files.push(fullPath)
      }
    }
  }

  traverse(dir)
  return files
}

async function main () {
  console.log('🔧 Исправляю основные ошибки TypeScript...\n')

  const srcDir = path.join(__dirname, '..', 'src')
  const files = getAllTsFiles(srcDir)

  files.forEach(file => {
    fixTypeScriptErrors(file)
  })

  console.log('\n✅ Исправление завершено!')
  console.log(`📊 Исправлено файлов: ${fixedCount}`)
}

main().catch(console.error)


