/**
 * Error types and interfaces for centralized error handling
 */

export enum ErrorType {
  NETWORK = 'NETWORK',
  VALIDATION = 'VALIDATION',
  AUTHENTICATION = 'AUTHENTICATION',
  AUTHORIZATION = 'AUTHORIZATION',
  NOT_FOUND = 'NOT_FOUND',
  SERVER = 'SERVER',
  CLIENT = 'CLIENT',
  UNKNOWN = 'UNKNOWN'
}

export enum ErrorSeverity {
  LOW = 'LOW',
  MEDIUM = 'MEDIUM',
  HIGH = 'HIGH',
  CRITICAL = 'CRITICAL'
}

export interface ErrorContext {
  component?: string
  action?: string
  userId?: string
  requestId?: string
  timestamp?: string
  userAgent?: string
  url?: string
  [key: string]: any
}

export interface AppError extends Error {
  type: ErrorType
  severity: ErrorSeverity
  context?: ErrorContext
  code?: string | number
  retryable?: boolean
  userMessage?: string
  technicalMessage?: string
  timestamp: string
}

export interface ApiError extends AppError {
  status?: number
  statusText?: string
  response?: {
    data?: any
    headers?: Record<string, string>
  }
}

export interface ValidationError extends AppError {
  field?: string
  value?: any
  constraint?: string
}

export interface NetworkError extends AppError {
  timeout?: boolean
  offline?: boolean
  retryCount?: number
}

export interface ErrorHandlerOptions {
  showNotification?: boolean
  logError?: boolean
  retryable?: boolean
  userMessage?: string
  fallbackAction?: () => void
  context?: ErrorContext
}

export interface ErrorHandlerConfig {
  defaultUserMessage: string
  enableRetry: boolean
  maxRetries: number
  retryDelay: number
  enableNotifications: boolean
  enableLogging: boolean
  enableErrorTracking: boolean
}

export type ErrorHandler = (error: AppError, options?: ErrorHandlerOptions) => void

export type ErrorRecoveryStrategy = 'retry' | 'fallback' | 'ignore' | 'escalate'

export interface ErrorRecoveryOptions {
  strategy: ErrorRecoveryStrategy
  maxRetries?: number
  retryDelay?: number
  fallbackAction?: () => void
  escalationAction?: () => void
}
