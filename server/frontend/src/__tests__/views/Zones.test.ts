import { describe, expect, it } from 'vitest'
import { mount } from '@vue/test-utils'
import ZonesView from '@/views/Zones.vue'

vi.mock('@/components/zones/ZoneDashboard.vue', () => ({
  default: {
    name: 'ZoneDashboardStub',
    template: '<div data-test="dashboard-stub">zones dashboard</div>'
  }
}))

describe('Zones view', () => {
  it('renders zone dashboard component', () => {
    const wrapper = mount(ZonesView)
    expect(wrapper.find('[data-test="dashboard-stub"]').exists()).toBe(true)
  })
})
