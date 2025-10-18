/**
 * Format distance to now in Russian
 */
export function formatDistanceToNow(date) {
  const now = new Date()
  const past = new Date(date)
  const seconds = Math.floor((now - past) / 1000)

  if (seconds < 10) return 'только что'
  if (seconds < 60) return `${seconds} сек назад`
  
  const minutes = Math.floor(seconds / 60)
  if (minutes < 60) return `${minutes} мин назад`
  
  const hours = Math.floor(minutes / 60)
  if (hours < 24) return `${hours} ч назад`
  
  const days = Math.floor(hours / 24)
  if (days < 30) return `${days} д назад`
  
  return past.toLocaleDateString('ru-RU')
}

/**
 * Format date to local string
 */
export function formatDateTime(date) {
  return new Date(date).toLocaleString('ru-RU', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  })
}

/**
 * Format date only
 */
export function formatDate(date) {
  return new Date(date).toLocaleDateString('ru-RU', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
  })
}

/**
 * Format time only
 */
export function formatTime(date) {
  return new Date(date).toLocaleTimeString('ru-RU', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  })
}

