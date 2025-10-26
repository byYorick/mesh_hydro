/**
 * Centralized error handling system
 * Replaces scattered try-catch blocks throughout the application
 */

import { logger } from './logger'
import { AppError, ApiError, ValidationError, NetworkError, ErrorType, ErrorSeverity, ErrorContext, ErrorHandlerOptions, ErrorHandlerConfig, ErrorRecoveryStrategy, ErrorRecoveryOptions } from '@/types/errors'

class ErrorHandler {
  private config: ErrorHandlerConfig
  private retryCounts = new Map<string, number>()

  constructor() {
    this.config = {
      defaultUserMessage: 'Произошла ошибка. Попробуйте еще раз.',
      enableRetry: true,
      maxRetries: 3,
      retryDelay: 1000,
      enableNotifications: true,
      enableLogging: true,
      enableErrorTracking: true
    }
  }

  /**
   * Create an AppError from various error sources
   */
  createError(
    error: Error | unknown,
    type: ErrorType = ErrorType.UNKNOWN,
    severity: ErrorSeverity = ErrorSeverity.MEDIUM,
    context?: ErrorContext
  ): AppError {
    const baseError = error instanceof Error ? error : new Error(String(error))
    
    const appError: AppError = {
      ...(baseError as any),
      type,
      severity,
      context: {
        ...(context as any),
        timestamp: new Date().toISOString(),
        userAgent: navigator.userAgent,
        url: window.location.href
      },
      timestamp: new Date().toISOString(),
      retryable: this.isRetryable(type),
      userMessage: this.getUserMessage(type, baseError.message),
      technicalMessage: baseError.message
    }

    return appError
  }

  /**
   * Create an ApiError from axios/fetch errors
   */
  createApiError(
    error: any,
    context?: ErrorContext
  ): ApiError {
    const type = this.getErrorTypeFromStatus(error.response?.status)
    const severity = this.getSeverityFromType(type)
    
    const apiError: ApiError = {
      ...(this as any).createError(error, type, severity, context),
      status: error.response?.status,
      statusText: error.response?.statusText,
      response: {
        data: error.response?.data,
        headers: error.response?.headers
      }
    }

    return apiError
  }

  /**
   * Create a ValidationError
   */
  createValidationError(
    message: string,
    field?: string,
    value?: any,
    constraint?: string,
    context?: ErrorContext
  ): ValidationError {
    return {
      ...(this as any).createError(new Error(message), ErrorType.VALIDATION, ErrorSeverity.LOW, context),
      field,
      value,
      constraint
    }
  }

  /**
   * Create a NetworkError
   */
  createNetworkError(
    error: Error,
    timeout = false,
    offline = false,
    context?: ErrorContext
  ): NetworkError {
    return {
      ...(this as any).createError(error, ErrorType.NETWORK, ErrorSeverity.MEDIUM, context),
      timeout,
      offline,
      retryCount: 0
    }
  }

  /**
   * Handle an error with options
   */
  handleError(
    error: AppError | Error | unknown,
    options: ErrorHandlerOptions = {}
  ): void {
    const appError = error instanceof Error && 'type' in error 
      ? error as AppError 
      : this.createError(error, ErrorType.UNKNOWN, ErrorSeverity.MEDIUM, options.context)

    // Log error
    if (options.logError !== false && this.config.enableLogging) {
      this.logError(appError)
    }

    // Show notification
    if (options.showNotification !== false && this.config.enableNotifications) {
      this.showNotification(appError, options.userMessage)
    }

    // Track error for analytics
    if (this.config.enableErrorTracking) {
      this.trackError(appError)
    }

    // Execute fallback action
    if (options.fallbackAction) {
      try {
        options.fallbackAction()
      } catch (fallbackError: any) {
        logger.error('Error in fallback action', { 
          originalError: appError.message,
          fallbackError: fallbackError instanceof Error ? fallbackError.message : String(fallbackError)
        })
      }
    }
  }

  /**
   * Handle API errors specifically
   */
  handleApiError(
    error: any,
    options: ErrorHandlerOptions = {}
  ): void {
    const apiError = this.createApiError(error, options.context)
    this.handleError(apiError, options)
  }

  /**
   * Handle store action errors
   */
  handleStoreAction<T>(
    action: () => Promise<T> | T,
    errorMessage: string,
    context?: ErrorContext
  ): Promise<T> {
    return new Promise(async (resolve, reject) => {
      try {
        const result = await action()
        resolve(result)
      } catch (error: any) {
        const appError = this.createError(
          error,
          ErrorType.CLIENT,
          ErrorSeverity.MEDIUM,
          { ...(context as any), action: 'store_action' }
        )
        
        this.handleError(appError, {
          userMessage: errorMessage,
          context
        })
        
        reject(appError)
      }
    })
  }

  /**
   * Retry an operation with exponential backoff
   */
  async retry<T>(
    operation: () => Promise<T>,
    context: ErrorContext,
    options: ErrorRecoveryOptions = { strategy: 'retry' }
  ): Promise<T> {
    const operationKey = `${context.component}-${context.action}`
    const currentRetries = this.retryCounts.get(operationKey) || 0

    if (currentRetries >= (options.maxRetries || this.config.maxRetries)) {
      throw this.createError(
        new Error('Maximum retries exceeded'),
        ErrorType.CLIENT,
        ErrorSeverity.HIGH,
        { ...(context as any), retryCount: currentRetries }
      )
    }

    try {
      const result = await operation()
      this.retryCounts.delete(operationKey)
      return result
    } catch (error: any) {
      const appError = this.createError(error, ErrorType.NETWORK, ErrorSeverity.MEDIUM, context)
      
      if (!appError.retryable) {
        throw appError
      }

      this.retryCounts.set(operationKey, currentRetries + 1)
      
      const delay = (options.retryDelay || this.config.retryDelay) * Math.pow(2, currentRetries)
      
      logger.warn(`Retrying operation in ${delay}ms`, { 
        ...(context as any), 
        retryCount: currentRetries + 1,
        delay 
      })

      await new Promise(resolve => setTimeout(resolve, delay))
      return this.retry(operation, context, options)
    }
  }

  /**
   * Clear retry counts
   */
  clearRetryCounts(): void {
    this.retryCounts.clear()
  }

  /**
   * Update configuration
   */
  updateConfig(newConfig: Partial<ErrorHandlerConfig>): void {
    this.config = { ...(this as any).config, ...(newConfig as any) }
  }

  private isRetryable(type: ErrorType): boolean {
    const retryableTypes: any[] = [
      ErrorType.NETWORK,
      ErrorType.SERVER
    ]
    return retryableTypes.includes(type)
  }

  private getErrorTypeFromStatus(status?: number): ErrorType {
    if (!status) return ErrorType.NETWORK
    
    if (status >= 400 && status < 500) {
      switch (status) {
        case 401: return ErrorType.AUTHENTICATION
        case 403: return ErrorType.AUTHORIZATION
        case 404: return ErrorType.NOT_FOUND
        case 422: return ErrorType.VALIDATION
        default: return ErrorType.CLIENT
      }
    }
    
    if (status >= 500) return ErrorType.SERVER
    
    return ErrorType.UNKNOWN
  }

  private getSeverityFromType(type: ErrorType): ErrorSeverity {
    switch (type) {
      case ErrorType.AUTHENTICATION:
      case ErrorType.AUTHORIZATION:
        return ErrorSeverity.HIGH
      case ErrorType.SERVER:
      case ErrorType.NETWORK:
        return ErrorSeverity.MEDIUM
      case ErrorType.VALIDATION:
      case ErrorType.NOT_FOUND:
        return ErrorSeverity.LOW
      default:
        return ErrorSeverity.MEDIUM
    }
  }

  private getUserMessage(type: ErrorType, technicalMessage: string): string {
    switch (type) {
      case ErrorType.NETWORK:
        return 'Проблема с подключением. Проверьте интернет-соединение.'
      case ErrorType.AUTHENTICATION:
        return 'Сессия истекла. Войдите в систему заново.'
      case ErrorType.AUTHORIZATION:
        return 'Недостаточно прав для выполнения операции.'
      case ErrorType.NOT_FOUND:
        return 'Запрашиваемый ресурс не найден.'
      case ErrorType.VALIDATION:
        return 'Проверьте правильность введенных данных.'
      case ErrorType.SERVER:
        return 'Ошибка сервера. Попробуйте позже.'
      default:
        return this.config.defaultUserMessage
    }
  }

  private logError(error: AppError): void {
    const logContext: any = {
      type: error.type,
      severity: error.severity,
      code: error.code,
      retryable: error.retryable,
      ...(error as any).context
    }

    switch (error.severity) {
      case ErrorSeverity.CRITICAL:
        logger.error(`CRITICAL ERROR: ${error.message}`, logContext)
        break
      case ErrorSeverity.HIGH:
        logger.error(`HIGH SEVERITY: ${error.message}`, logContext)
        break
      case ErrorSeverity.MEDIUM:
        logger.warn(`MEDIUM SEVERITY: ${error.message}`, logContext)
        break
      case ErrorSeverity.LOW:
        logger.info(`LOW SEVERITY: ${error.message}`, logContext)
        break
    }
  }

  private showNotification(error: AppError, customMessage?: string): void {
    const message = customMessage || error.userMessage || this.config.defaultUserMessage
    
    // This will be integrated with the notification system
    // For now, we'll use console for development
    if (import.meta.env.DEV) {
      console.warn('Notification:', message)
    }
    
    // TODO: Integrate with actual notification system
    // notificationStore.show(message, 'error')
  }

  private trackError(error: AppError): void {
    // This will be integrated with error tracking service
    // For now, we'll just log it
    logger.debug('Error tracked for analytics', {
      type: error.type,
      severity: error.severity,
      message: error.message,
      timestamp: error.timestamp
    })
    
    // TODO: Send to error tracking service (Sentry, etc.)
  }
}

// Export singleton instance
export const errorHandler = new ErrorHandler()

// Export convenience functions
export const handleError = (error: AppError | Error | unknown, options?: ErrorHandlerOptions) => 
  errorHandler.handleError(error, options)

export const handleApiError = (error: any, options?: ErrorHandlerOptions) => 
  errorHandler.handleApiError(error, options)

export const handleStoreAction = <T>(
  action: () => Promise<T> | T,
  errorMessage: string,
  context?: ErrorContext
) => errorHandler.handleStoreAction(action, errorMessage, context)

export const retry = <T>(
  operation: () => Promise<T>,
  context: ErrorContext,
  options?: ErrorRecoveryOptions
) => errorHandler.retry(operation, context, options)

export default errorHandler
