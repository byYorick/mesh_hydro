import { describe, expect, it, vi } from 'vitest'
import { formatDistanceToNow, formatDateTime, formatDate, formatTime } from '@/utils/time'

describe('time utils', () => {
  it('formats distance to now', () => {
    const now = Date.now()
    const short = formatDistanceToNow(now - 2000)
    expect(['только что', '2 сек назад']).toContain(short)
    expect(formatDistanceToNow(now - 5 * 60 * 1000)).toBe('5 мин назад')
    expect(formatDistanceToNow(now - 2 * 60 * 60 * 1000)).toBe('2 ч назад')
    expect(formatDistanceToNow(now - 3 * 24 * 60 * 60 * 1000)).toBe('3 д назад')
  })

  it('falls back to date for long intervals', () => {
    const oldDate = new Date('2020-01-01T00:00:00Z')
    expect(formatDistanceToNow(oldDate)).toBe(oldDate.toLocaleDateString('ru-RU'))
  })

  it('formats date and time helpers', () => {
    const fixed = new Date('2023-01-02T03:04:05Z')
    const localeSpy = vi.spyOn(Date.prototype, 'toLocaleString')
    formatDateTime(fixed)
    expect(localeSpy).toHaveBeenCalledWith('ru-RU', expect.objectContaining({ second: '2-digit' }))
    localeSpy.mockRestore()

    const dateSpy = vi.spyOn(Date.prototype, 'toLocaleDateString')
    formatDate(fixed)
    expect(dateSpy).toHaveBeenCalledWith('ru-RU', expect.objectContaining({ day: '2-digit' }))
    dateSpy.mockRestore()

    const timeSpy = vi.spyOn(Date.prototype, 'toLocaleTimeString')
    formatTime(fixed)
    expect(timeSpy).toHaveBeenCalledWith('ru-RU', expect.objectContaining({ minute: '2-digit' }))
    timeSpy.mockRestore()
  })
})
