import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'

const originalEnv = { ...import.meta.env }

describe('logger', () => {
  beforeEach(() => {
    vi.clearAllMocks()
  })

  afterEach(() => {
    Object.assign(import.meta.env, originalEnv)
    vi.resetModules()
  })

  it('logs messages respecting log levels in development', async () => {
    Object.assign(import.meta.env, { DEV: true })
    const consoleError = vi.spyOn(console, 'error').mockImplementation(() => {})
    const consoleWarn = vi.spyOn(console, 'warn').mockImplementation(() => {})
    const consoleInfo = vi.spyOn(console, 'info').mockImplementation(() => {})
    const consoleDebug = vi.spyOn(console, 'debug').mockImplementation(() => {})

    const { logger, logApi, logStore } = await import('@/utils/logger')

    logger.error('error', { context: 'test' })
    logger.warn('warn')
    logger.info('info')
    logger.debug('debug')
    logApi('call', { endpoint: '/test' })
    logStore('update')

    expect(consoleError).toHaveBeenCalled()
    expect(consoleWarn).toHaveBeenCalled()
    expect(consoleInfo).toHaveBeenCalled()
    expect(consoleDebug).toHaveBeenCalled()
  })

  it('suppresses debug logs in production mode', async () => {
    Object.assign(import.meta.env, { DEV: false })
    const consoleDebug = vi.spyOn(console, 'debug').mockImplementation(() => {})
    const consoleInfo = vi.spyOn(console, 'info').mockImplementation(() => {})

    const { logger } = await import('@/utils/logger')

    logger.debug('debug hidden')
    logger.info('info visible')

    expect(consoleDebug).not.toHaveBeenCalled()
    expect(consoleInfo).toHaveBeenCalled()
  })
})
