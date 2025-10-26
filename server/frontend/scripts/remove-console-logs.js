/**
 * Remove console.log statements from all Vue, JS, and TS files
 * Run with: node scripts/remove-console-logs.js
 */

import { readdir, readFile, writeFile } from 'fs/promises'
import { join, dirname } from 'path'
import { fileURLToPath } from 'url'

const __filename = fileURLToPath(import.meta.url)
const __dirname = dirname(__filename)

const SRC_DIR = join(__dirname, '../src')
const FILES_TO_PROCESS = ['.vue', '.js', '.ts', '.jsx', '.tsx']

let filesProcessed = 0
let logsRemoved = 0

async function processDirectory(dir) {
  const entries = await readdir(dir, { withFileTypes: true })
  
  for (const entry of entries) {
    const fullPath = join(dir, entry.name)
    
    if (entry.isDirectory()) {
      // Skip node_modules and other ignored directories
      if (!['node_modules', '.git', 'dist', 'build'].includes(entry.name)) {
        await processDirectory(fullPath)
      }
    } else if (entry.isFile()) {
      const ext = entry.name.substring(entry.name.lastIndexOf('.'))
      if (FILES_TO_PROCESS.includes(ext)) {
        await processFile(fullPath)
      }
    }
  }
}

async function processFile(filePath) {
  try {
    let content = await readFile(filePath, 'utf-8')
    const originalContent = content
    
    // Remove console.log statements
    // Pattern 1: console.log(...); on single line
    content = content.replace(/console\.log\([^)]*\);?\s*\n?/g, '')
    
    // Pattern 2: console.log(...) with newlines in quotes
    content = content.replace(/console\.log\([^;]*?\);?\s*\n?/g, '')
    
    // Pattern 3: Multi-line console.log (more complex)
    const lines = content.split('\n')
    const filteredLines = []
    let skipLine = false
    
    for (let i = 0; i < lines.length; i++) {
      const line = lines[i]
      const trimmedLine = line.trim()
      
      // Skip standalone console.log lines
      if (trimmedLine.match(/^console\.log\(/) && trimmedLine.match(/\);?\s*$/)) {
        const logsInLine = (line.match(/console\.log/g) || []).length
        logsRemoved += logsInLine
        skipLine = true
        continue
      }
      
      // Skip lines that are just closing parenthesis and semicolon from console.log
      if (skipLine && trimmedLine === ');') {
        skipLine = false
        continue
      }
      
      skipLine = false
      filteredLines.push(line)
    }
    
    content = filteredLines.join('\n')
    
    // Count how many logs were removed
    const originalLogs = (originalContent.match(/console\.log/g) || []).length
    const remainingLogs = (content.match(/console\.log/g) || []).length
    const removedFromFile = originalLogs - remainingLogs
    
    if (removedFromFile > 0) {
      await writeFile(filePath, content, 'utf-8')
      console.log(`✅ ${filePath}: Removed ${removedFromFile} console.log statements`)
      filesProcessed++
      logsRemoved += removedFromFile
    }
  } catch (error) {
    console.error(`❌ Error processing ${filePath}:`, error.message)
  }
}

async function main() {
  console.log('🧹 Starting console.log removal...\n')
  
  try {
    await processDirectory(SRC_DIR)
    
    console.log(`\n✅ Done!`)
    console.log(`   Files processed: ${filesProcessed}`)
    console.log(`   Total console.log removed: ${logsRemoved}`)
  } catch (error) {
    console.error('❌ Error:', error)
    process.exit(1)
  }
}

main()
