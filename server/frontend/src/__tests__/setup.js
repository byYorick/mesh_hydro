import { config } from '@vue/test-utils'
import { vi } from 'vitest'
import { createVuetify } from 'vuetify'
import * as components from 'vuetify/components'
import * as directives from 'vuetify/directives'

vi.mock('axios', () => {
  const axiosMock = {
    get: vi.fn().mockResolvedValue({ data: [] }),
    post: vi.fn().mockResolvedValue({ data: {} }),
    put: vi.fn().mockResolvedValue({ data: {} }),
    delete: vi.fn().mockResolvedValue({ data: {} }),
    patch: vi.fn().mockResolvedValue({ data: {} }),
    interceptors: {
      request: {
        use: vi.fn(() => 0),
        eject: vi.fn()
      },
      response: {
        use: vi.fn(() => 0),
        eject: vi.fn()
      }
    },
    create: vi.fn(() => axiosMock)
  }

  return {
    default: axiosMock,
    ...axiosMock
  }
})

// Mock Vuetify
const vuetify = createVuetify({
  components,
  directives
})

config.global.plugins = [vuetify]

// Mock window.matchMedia (для Vuetify responsive)
Object.defineProperty(window, 'matchMedia', {
  writable: true,
  value: vi.fn().mockImplementation(query => ({
    matches: false,
    media: query,
    onchange: null,
    addListener: vi.fn(),
    removeListener: vi.fn(),
    addEventListener: vi.fn(),
    removeEventListener: vi.fn(),
    dispatchEvent: vi.fn()
  }))
})

// Mock IntersectionObserver
global.IntersectionObserver = class IntersectionObserver {
  constructor () {}
  disconnect () {}
  observe () {}
  takeRecords () {
    return []
  }
  unobserve () {}
}

// Mock visualViewport (используется Vuetify диалогами)
if (!global.visualViewport) {
  global.visualViewport = {
    width: 1024,
    height: 768,
    scale: 1,
    addEventListener: vi.fn(),
    removeEventListener: vi.fn(),
    dispatchEvent: vi.fn()
  }
}

// Stub scrollTo чтобы избегать ошибок в тестах
if (!window.scrollTo) {
  window.scrollTo = vi.fn()
}

const HTMLElementCtor = typeof window !== 'undefined' && window.HTMLElement ? window.HTMLElement : class {}
const ElementCtor = typeof window !== 'undefined' && window.Element ? window.Element : HTMLElementCtor
const SVGElementCtor = typeof window !== 'undefined' && window.SVGElement ? window.SVGElement : class {}

global.HTMLElement = HTMLElementCtor
global.Element = ElementCtor
global.SVGElement = SVGElementCtor
globalThis.HTMLElement = HTMLElementCtor
globalThis.Element = ElementCtor
globalThis.SVGElement = SVGElementCtor

if (typeof window !== 'undefined') {
  window.HTMLElement = HTMLElementCtor
  window.Element = ElementCtor
  window.SVGElement = SVGElementCtor
}

// Mock ResizeObserver
global.ResizeObserver = class ResizeObserver {
  constructor () {}
  disconnect () {}
  observe () {}
  unobserve () {}
}

// Mock localStorage
const localStorageMock = {
  getItem: vi.fn(),
  setItem: vi.fn(),
  removeItem: vi.fn(),
  clear: vi.fn()
}
global.localStorage = localStorageMock

// Polyfill DOMRect для Vuetify анимаций на happy-dom
if (typeof window !== 'undefined' && !window.DOMRect) {
  class DOMRectImpl {
    constructor (x = 0, y = 0, width = 0, height = 0) {
      this.x = x
      this.y = y
      this.width = width
      this.height = height
      this.top = y
      this.left = x
      this.right = x + width
      this.bottom = y + height
    }

    static fromRect (rect = {}) {
      return new DOMRectImpl(
        rect.x ?? 0,
        rect.y ?? 0,
        rect.width ?? 0,
        rect.height ?? 0
      )
    }
  }

  window.DOMRect = DOMRectImpl
  global.DOMRect = DOMRectImpl
}

// Mock для CSS импортов через Vite
// CSS файлы будут обработаны через Vite плагины

const ignoredInstanceofMessage = "Right-hand side of 'instanceof' is not an object"

process.on('uncaughtException', (err) => {
  if (err && typeof err.message === 'string' && err.message.includes(ignoredInstanceofMessage)) {
    return
  }
  throw err
})

process.on('unhandledRejection', (reason) => {
  if (reason && typeof reason.message === 'string' && reason.message.includes(ignoredInstanceofMessage)) {
    return
  }
  throw reason
})