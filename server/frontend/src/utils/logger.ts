/**
 * Centralized logging utility with environment-aware levels
 * Replaces all console.log calls throughout the application
 */

export enum LogLevel {
  ERROR = 0,
  WARN = 1,
  INFO = 2,
  DEBUG = 3
}

export interface LogContext {
  component?: string
  action?: string
  userId?: string
  requestId?: string
  [key: string]: any
}

class Logger {
  private level: LogLevel
  private isDevelopment: boolean

  constructor() {
    this.isDevelopment = import.meta.env.DEV
    this.level = this.isDevelopment ? LogLevel.DEBUG : LogLevel.INFO
  }

  private shouldLog(level: LogLevel): boolean {
    return level <= this.level
  }

  private formatMessage(level: string, message: string, context?: LogContext): string {
    const timestamp = new Date().toISOString()
    const contextStr = context ? ` [${JSON.stringify(context)}]` : ''
    return `[${timestamp}] ${level}${contextStr}: ${message}`
  }

  private log(level: LogLevel, levelName: string, message: string, context?: LogContext, ...args: any[]): void {
    if (!this.shouldLog(level)) return

    const formattedMessage = this.formatMessage(levelName, message, context)
    
    switch (level) {
      case LogLevel.ERROR:
        console.error(formattedMessage, ...args)
        break
      case LogLevel.WARN:
        console.warn(formattedMessage, ...args)
        break
      case LogLevel.INFO:
        console.info(formattedMessage, ...args)
        break
      case LogLevel.DEBUG:
        console.debug(formattedMessage, ...args)
        break
    }
  }

  error(message: string, context?: LogContext, ...args: any[]): void {
    this.log(LogLevel.ERROR, 'ERROR', message, context, ...args)
  }

  warn(message: string, context?: LogContext, ...args: any[]): void {
    this.log(LogLevel.WARN, 'WARN', message, context, ...args)
  }

  info(message: string, context?: LogContext, ...args: any[]): void {
    this.log(LogLevel.INFO, 'INFO', message, context, ...args)
  }

  debug(message: string, context?: LogContext, ...args: any[]): void {
    this.log(LogLevel.DEBUG, 'DEBUG', message, context, ...args)
  }

  // Specialized logging methods
  api(message: string, context?: LogContext, ...args: any[]): void {
    this.debug(`[API] ${message}`, { ...context, category: 'api' }, ...args)
  }

  store(message: string, context?: LogContext, ...args: any[]): void {
    this.debug(`[STORE] ${message}`, { ...context, category: 'store' }, ...args)
  }

  component(message: string, componentName: string, context?: LogContext, ...args: any[]): void {
    this.debug(`[${componentName}] ${message}`, { ...context, component: componentName, category: 'component' }, ...args)
  }

  performance(message: string, context?: LogContext, ...args: any[]): void {
    this.info(`[PERF] ${message}`, { ...context, category: 'performance' }, ...args)
  }

  // Error tracking for production
  trackError(error: Error, context?: LogContext): void {
    this.error(`Error: ${error.message}`, {
      ...context,
      stack: error.stack,
      name: error.name
    })

    // In production, you might want to send this to an error tracking service
    if (!this.isDevelopment) {
      // Example: sendToErrorTracking(error, context)
    }
  }

  // Performance timing
  time(label: string): void {
    if (this.isDevelopment) {
      console.time(label)
    }
  }

  timeEnd(label: string): void {
    if (this.isDevelopment) {
      console.timeEnd(label)
    }
  }

  // Group logging for related operations
  group(label: string): void {
    if (this.isDevelopment) {
      console.group(label)
    }
  }

  groupEnd(): void {
    if (this.isDevelopment) {
      console.groupEnd()
    }
  }
}

// Export singleton instance
export const logger = new Logger()

// Export convenience functions
export const logError = (message: string, context?: LogContext, ...args: any[]) => logger.error(message, context, ...args)
export const logWarn = (message: string, context?: LogContext, ...args: any[]) => logger.warn(message, context, ...args)
export const logInfo = (message: string, context?: LogContext, ...args: any[]) => logger.info(message, context, ...args)
export const logDebug = (message: string, context?: LogContext, ...args: any[]) => logger.debug(message, context, ...args)
export const logApi = (message: string, context?: LogContext, ...args: any[]) => logger.api(message, context, ...args)
export const logStore = (message: string, context?: LogContext, ...args: any[]) => logger.store(message, context, ...args)
export const logComponent = (message: string, componentName: string, context?: LogContext, ...args: any[]) => logger.component(message, componentName, context, ...args)
export const logPerformance = (message: string, context?: LogContext, ...args: any[]) => logger.performance(message, context, ...args)
export const trackError = (error: Error, context?: LogContext) => logger.trackError(error, context)

export default logger
