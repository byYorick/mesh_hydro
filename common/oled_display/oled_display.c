#include "oled_display.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define OLED_TAG "oled_display"

#define OLED_DEFAULT_ADDRESS        0x3C
#define OLED_DEFAULT_CLK_SPEED_HZ   400000
#define OLED_DEFAULT_WIDTH          128
#define OLED_DEFAULT_HEIGHT         64
#define OLED_DEFAULT_LINES          4

#define OLED_FONT_WIDTH             6U
#define OLED_FONT_DATA_WIDTH        5U
#define OLED_BYTES_PER_PAGE(width)  (width)
#define OLED_PAGE_COUNT(height)     ((height) / 8U)

#define OLED_MAX_LINES              8
#define OLED_MAX_TEMPLATE_LEN       64
#define OLED_MAX_LINE_TEXT_LEN      32
#define OLED_MAX_KV_PAIRS           12
#define OLED_MAX_KEY_LEN            16
#define OLED_MAX_VALUE_LEN          32

#define OLED_QUEUE_MIN_DEPTH        4
#define OLED_TASK_STACK_MIN         3072
#define OLED_HEART_TIMEOUT_DEFAULT  pdMS_TO_TICKS(1500)
#define OLED_RENDER_TIMEOUT_MS      100

#define HEART_SYMBOL "<3"

typedef enum {
    OLED_MSG_RENDER,
    OLED_MSG_HEARTBEAT
} oled_msg_type_t;

typedef struct {
    size_t count;
    char keys[OLED_MAX_KV_PAIRS][OLED_MAX_KEY_LEN];
    char values[OLED_MAX_KV_PAIRS][OLED_MAX_VALUE_LEN];
} oled_render_payload_t;

typedef struct {
    oled_msg_type_t type;
    union {
        oled_render_payload_t render;
        bool heartbeat_active;
    } data;
} oled_queue_msg_t;

typedef struct {
    oled_display_config_t cfg;
    bool initialised;
    bool task_started;
    QueueHandle_t queue;
    TaskHandle_t task;
    TickType_t heartbeat_timeout;
    bool heartbeat_active;
    TickType_t heartbeat_deadline;
    size_t line_count;

    char templates[OLED_MAX_LINES][OLED_MAX_TEMPLATE_LEN];
    char rendered_lines[OLED_MAX_LINES][OLED_MAX_LINE_TEXT_LEN];

    oled_render_payload_t current_values;

    uint8_t framebuffer[OLED_DEFAULT_WIDTH * OLED_PAGE_COUNT(OLED_DEFAULT_HEIGHT)];
} oled_display_ctx_t;

static oled_display_ctx_t s_ctx = {0};

static const uint8_t s_font5x7[][OLED_FONT_DATA_WIDTH] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* 0x20 ' ' */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* 0x21 '!' */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* 0x22 '"' */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* 0x23 '#' */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* 0x24 '$' */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* 0x25 '%' */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* 0x26 '&' */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* 0x27 ''' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* 0x28 '(' */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* 0x29 ')' */
    {0x14, 0x08, 0x3E, 0x08, 0x14}, /* 0x2A '*' */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* 0x2B '+' */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* 0x2C ',' */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* 0x2D '-' */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* 0x2E '.' */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* 0x2F '/' */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0x30 '0' */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 0x31 '1' */
    {0x72, 0x49, 0x49, 0x49, 0x46}, /* 0x32 '2' */
    {0x21, 0x41, 0x49, 0x4D, 0x33}, /* 0x33 '3' */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 0x34 '4' */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 0x35 '5' */
    {0x3C, 0x4A, 0x49, 0x49, 0x31}, /* 0x36 '6' */
    {0x03, 0x01, 0x79, 0x05, 0x03}, /* 0x37 '7' */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 0x38 '8' */
    {0x46, 0x49, 0x49, 0x29, 0x1E}, /* 0x39 '9' */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* 0x3A ':' */
    {0x00, 0x56, 0x36, 0x00, 0x00}, /* 0x3B ';' */
    {0x08, 0x14, 0x22, 0x41, 0x00}, /* 0x3C '<' */
    {0x14, 0x14, 0x14, 0x14, 0x14}, /* 0x3D '=' */
    {0x00, 0x41, 0x22, 0x14, 0x08}, /* 0x3E '>' */
    {0x02, 0x01, 0x59, 0x09, 0x06}, /* 0x3F '?' */
    {0x3E, 0x41, 0x5D, 0x59, 0x4E}, /* 0x40 '@' */
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, /* 0x41 'A' */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* 0x42 'B' */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* 0x43 'C' */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* 0x44 'D' */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* 0x45 'E' */
    {0x7F, 0x09, 0x09, 0x09, 0x01}, /* 0x46 'F' */
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, /* 0x47 'G' */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* 0x48 'H' */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* 0x49 'I' */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* 0x4A 'J' */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* 0x4B 'K' */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* 0x4C 'L' */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, /* 0x4D 'M' */
    {0x7F, 0x02, 0x04, 0x08, 0x7F}, /* 0x4E 'N' */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* 0x4F 'O' */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* 0x50 'P' */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* 0x51 'Q' */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* 0x52 'R' */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* 0x53 'S' */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* 0x54 'T' */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* 0x55 'U' */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* 0x56 'V' */
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, /* 0x57 'W' */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* 0x58 'X' */
    {0x07, 0x08, 0x70, 0x08, 0x07}, /* 0x59 'Y' */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* 0x5A 'Z' */
    {0x00, 0x7F, 0x41, 0x41, 0x00}, /* 0x5B '[' */
    {0x02, 0x04, 0x08, 0x10, 0x20}, /* 0x5C '\' */
    {0x00, 0x41, 0x41, 0x7F, 0x00}, /* 0x5D ']' */
    {0x04, 0x02, 0x01, 0x02, 0x04}, /* 0x5E '^' */
    {0x80, 0x80, 0x80, 0x80, 0x80}, /* 0x5F '_' */
    {0x00, 0x03, 0x07, 0x00, 0x00}, /* 0x60 '`' */
    {0x20, 0x54, 0x54, 0x54, 0x78}, /* 0x61 'a' */
    {0x7F, 0x48, 0x44, 0x44, 0x38}, /* 0x62 'b' */
    {0x38, 0x44, 0x44, 0x44, 0x20}, /* 0x63 'c' */
    {0x38, 0x44, 0x44, 0x48, 0x7F}, /* 0x64 'd' */
    {0x38, 0x54, 0x54, 0x54, 0x18}, /* 0x65 'e' */
    {0x08, 0x7E, 0x09, 0x01, 0x02}, /* 0x66 'f' */
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, /* 0x67 'g' */
    {0x7F, 0x08, 0x04, 0x04, 0x78}, /* 0x68 'h' */
    {0x00, 0x44, 0x7D, 0x40, 0x00}, /* 0x69 'i' */
    {0x20, 0x40, 0x44, 0x3D, 0x00}, /* 0x6A 'j' */
    {0x7F, 0x10, 0x28, 0x44, 0x00}, /* 0x6B 'k' */
    {0x00, 0x41, 0x7F, 0x40, 0x00}, /* 0x6C 'l' */
    {0x7C, 0x04, 0x18, 0x04, 0x78}, /* 0x6D 'm' */
    {0x7C, 0x08, 0x04, 0x04, 0x78}, /* 0x6E 'n' */
    {0x38, 0x44, 0x44, 0x44, 0x38}, /* 0x6F 'o' */
    {0x7C, 0x14, 0x14, 0x14, 0x08}, /* 0x70 'p' */
    {0x08, 0x14, 0x14, 0x18, 0x7C}, /* 0x71 'q' */
    {0x7C, 0x08, 0x04, 0x04, 0x08}, /* 0x72 'r' */
    {0x48, 0x54, 0x54, 0x54, 0x20}, /* 0x73 's' */
    {0x04, 0x3F, 0x44, 0x40, 0x20}, /* 0x74 't' */
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, /* 0x75 'u' */
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, /* 0x76 'v' */
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, /* 0x77 'w' */
    {0x44, 0x28, 0x10, 0x28, 0x44}, /* 0x78 'x' */
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, /* 0x79 'y' */
    {0x44, 0x64, 0x54, 0x4C, 0x44}, /* 0x7A 'z' */
    {0x00, 0x08, 0x36, 0x41, 0x00}, /* 0x7B '{' */
    {0x00, 0x00, 0x7F, 0x00, 0x00}, /* 0x7C '|' */
    {0x00, 0x41, 0x36, 0x08, 0x00}, /* 0x7D '}' */
    {0x10, 0x08, 0x08, 0x10, 0x08}, /* 0x7E '~' */
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* 0x7F */
};

static esp_err_t ssd1306_send_command(uint8_t cmd);
static esp_err_t ssd1306_send_commands(const uint8_t *cmds, size_t len);
static esp_err_t ssd1306_send_data(const uint8_t *data, size_t len);
static void ssd1306_clear_framebuffer(void);
static void ssd1306_render_lines(void);
static void ssd1306_draw_line(size_t line_index, const char *text);
static void oled_task(void *arg);
static void apply_render_payload(const oled_render_payload_t *payload);
static void refresh_display(bool force);
static const char *find_value_for_key(const char *key);
static void expand_templates(void);
static void update_heartbeat_state(bool active);
static void heartbeat_timeout_check(void);

static inline size_t ssd1306_buffer_size(void)
{
    return s_ctx.cfg.width * OLED_PAGE_COUNT(s_ctx.cfg.height);
}

esp_err_t oled_display_init(const oled_display_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_ctx.initialised) {
        ESP_LOGW(OLED_TAG, "Already initialised");
        return ESP_OK;
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.cfg = *config;

    if (s_ctx.cfg.i2c_address == 0) {
        s_ctx.cfg.i2c_address = OLED_DEFAULT_ADDRESS;
    }
    if (s_ctx.cfg.clk_speed_hz == 0) {
        s_ctx.cfg.clk_speed_hz = OLED_DEFAULT_CLK_SPEED_HZ;
    }
    if (s_ctx.cfg.width == 0) {
        s_ctx.cfg.width = OLED_DEFAULT_WIDTH;
    }
    if (s_ctx.cfg.height == 0) {
        s_ctx.cfg.height = OLED_DEFAULT_HEIGHT;
    }
    if (s_ctx.cfg.line_count == 0 || s_ctx.cfg.line_count > OLED_MAX_LINES) {
        s_ctx.cfg.line_count = OLED_DEFAULT_LINES;
    }
    s_ctx.line_count = s_ctx.cfg.line_count;
    if (s_ctx.line_count > OLED_PAGE_COUNT(s_ctx.cfg.height)) {
        s_ctx.line_count = OLED_PAGE_COUNT(s_ctx.cfg.height);
    }
    s_ctx.heartbeat_timeout = OLED_HEART_TIMEOUT_DEFAULT;

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = s_ctx.cfg.sda_pin,
        .scl_io_num = s_ctx.cfg.scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = s_ctx.cfg.clk_speed_hz,
    };

    // Сначала пытаемся установить параметры (это работает даже если драйвер уже установлен)
    esp_err_t err = i2c_param_config(s_ctx.cfg.i2c_port, &i2c_conf);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(OLED_TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    // Пытаемся установить драйвер. Если он уже установлен, получим ESP_ERR_INVALID_STATE или ESP_FAIL
    err = i2c_driver_install(s_ctx.cfg.i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) {
        // Драйвер уже установлен - это нормально, продолжаем
        ESP_LOGI(OLED_TAG, "I2C driver already installed, using existing driver");
    } else if (err == ESP_FAIL) {
        // ESP_FAIL может означать, что драйвер уже установлен с другими параметрами
        // Попробуем использовать существующий драйвер
        ESP_LOGW(OLED_TAG, "i2c_driver_install returned ESP_FAIL, assuming driver already installed");
        // Обновляем параметры для существующего драйвера
        err = i2c_param_config(s_ctx.cfg.i2c_port, &i2c_conf);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(OLED_TAG, "Failed to update I2C params: %s", esp_err_to_name(err));
            return err;
        }
    } else if (err != ESP_OK) {
        ESP_LOGE(OLED_TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    static const uint8_t init_cmds[] = {
        0xAE,
        0xD5, 0x80,
        0xA8, 0x3F,
        0xD3, 0x00,
        0x40,
        0x8D, 0x14,
        0x20, 0x00,
        0xA1,
        0xC8,
        0xDA, 0x12,
        0x81, 0xCF,
        0xD9, 0xF1,
        0xDB, 0x40,
        0xA4,
        0xA6,
        0x2E,
        0xAF
    };

    ESP_RETURN_ON_ERROR(ssd1306_send_commands(init_cmds, sizeof(init_cmds)), OLED_TAG, "Init sequence failed");
    ssd1306_clear_framebuffer();
    refresh_display(true);

    s_ctx.initialised = true;
    ESP_LOGI(OLED_TAG, "OLED initialised: addr=0x%02X size=%ux%u", s_ctx.cfg.i2c_address, s_ctx.cfg.width, s_ctx.cfg.height);
    return ESP_OK;
}

esp_err_t oled_display_set_template(size_t line_index, const char *template_str)
{
    if (!s_ctx.initialised) {
        return ESP_ERR_INVALID_STATE;
    }
    if (line_index >= s_ctx.line_count || !template_str) {
        return ESP_ERR_INVALID_ARG;
    }
    size_t len = strnlen(template_str, OLED_MAX_TEMPLATE_LEN - 1);
    memcpy(s_ctx.templates[line_index], template_str, len);
    s_ctx.templates[line_index][len] = '\0';
    return ESP_OK;
}

esp_err_t oled_display_start_task(const oled_display_task_config_t *task_cfg)
{
    if (!s_ctx.initialised) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_ctx.task_started) {
        return ESP_OK;
    }

    uint32_t stack = (task_cfg && task_cfg->stack_size >= OLED_TASK_STACK_MIN) ? task_cfg->stack_size : OLED_TASK_STACK_MIN;
    UBaseType_t prio = (task_cfg && task_cfg->priority > 0) ? task_cfg->priority : (tskIDLE_PRIORITY + 2);
    size_t queue_depth = (task_cfg && task_cfg->queue_depth >= OLED_QUEUE_MIN_DEPTH) ? task_cfg->queue_depth : OLED_QUEUE_MIN_DEPTH;
    TickType_t hb_timeout = (task_cfg && task_cfg->heartbeat_timeout_ticks) ? task_cfg->heartbeat_timeout_ticks : OLED_HEART_TIMEOUT_DEFAULT;

    s_ctx.heartbeat_timeout = hb_timeout;

    s_ctx.queue = xQueueCreate(queue_depth, sizeof(oled_queue_msg_t));
    if (!s_ctx.queue) {
        ESP_LOGE(OLED_TAG, "Failed to create queue");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t res = xTaskCreatePinnedToCore(oled_task, "oled_display", stack, NULL, prio, &s_ctx.task, tskNO_AFFINITY);
    if (res != pdPASS) {
        vQueueDelete(s_ctx.queue);
        s_ctx.queue = NULL;
        ESP_LOGE(OLED_TAG, "Failed to create task");
        return ESP_FAIL;
    }

    s_ctx.task_started = true;
    return ESP_OK;
}

esp_err_t oled_display_queue_render(const oled_display_kv_t *items, size_t count, TickType_t timeout_ticks)
{
    if (!s_ctx.task_started || !items) {
        return ESP_ERR_INVALID_STATE;
    }
    if (count > OLED_MAX_KV_PAIRS) {
        ESP_LOGW(OLED_TAG, "KV count truncated to %d", OLED_MAX_KV_PAIRS);
        count = OLED_MAX_KV_PAIRS;
    }

    oled_queue_msg_t msg = {
        .type = OLED_MSG_RENDER,
    };
    msg.data.render.count = count;
    for (size_t i = 0; i < count; ++i) {
        if (!items[i].key || !items[i].value) {
            msg.data.render.keys[i][0] = '\0';
            msg.data.render.values[i][0] = '\0';
            continue;
        }
        strlcpy(msg.data.render.keys[i], items[i].key, OLED_MAX_KEY_LEN);
        strlcpy(msg.data.render.values[i], items[i].value, OLED_MAX_VALUE_LEN);
    }

    TickType_t ticks = timeout_ticks ? timeout_ticks : pdMS_TO_TICKS(OLED_RENDER_TIMEOUT_MS);
    if (xQueueSend(s_ctx.queue, &msg, ticks) != pdPASS) {
        ESP_LOGW(OLED_TAG, "Render queue full");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t oled_display_show_heartbeat(bool active)
{
    if (!s_ctx.task_started) {
        return ESP_ERR_INVALID_STATE;
    }
    oled_queue_msg_t msg = {
        .type = OLED_MSG_HEARTBEAT,
    };
    msg.data.heartbeat_active = active;
    if (xQueueSend(s_ctx.queue, &msg, 0) != pdPASS) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

void oled_display_shutdown(void)
{
    if (s_ctx.task_started && s_ctx.task) {
        vTaskDelete(s_ctx.task);
        s_ctx.task = NULL;
    }
    if (s_ctx.queue) {
        vQueueDelete(s_ctx.queue);
        s_ctx.queue = NULL;
    }
    if (s_ctx.initialised) {
        i2c_driver_delete(s_ctx.cfg.i2c_port);
    }
    memset(&s_ctx, 0, sizeof(s_ctx));
}

static esp_err_t ssd1306_send_command(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    return i2c_master_write_to_device(s_ctx.cfg.i2c_port, s_ctx.cfg.i2c_address, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static esp_err_t ssd1306_send_commands(const uint8_t *cmds, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        ESP_RETURN_ON_ERROR(ssd1306_send_command(cmds[i]), OLED_TAG, "send command failed");
    }
    return ESP_OK;
}

static esp_err_t ssd1306_send_data(const uint8_t *data, size_t len)
{
    if (!len) {
        return ESP_OK;
    }
    uint8_t buffer[128 + 1];
    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > sizeof(buffer) - 1) {
            chunk = sizeof(buffer) - 1;
        }
        buffer[0] = 0x40;
        memcpy(&buffer[1], data + offset, chunk);
        ESP_RETURN_ON_ERROR(i2c_master_write_to_device(s_ctx.cfg.i2c_port, s_ctx.cfg.i2c_address, buffer, chunk + 1, pdMS_TO_TICKS(100)), OLED_TAG, "send data failed");
        offset += chunk;
    }
    return ESP_OK;
}

static void ssd1306_clear_framebuffer(void)
{
    memset(s_ctx.framebuffer, 0, ssd1306_buffer_size());
}

// Функция для отрисовки строки крупным шрифтом (2x) - занимает 2 строки
static void ssd1306_draw_line_2x(size_t line_index, const char *text)
{
    if (line_index >= OLED_PAGE_COUNT(s_ctx.cfg.height) - 1) {
        return; // Нужно минимум 2 строки для двойного размера
    }
    
    size_t offset_upper = line_index * s_ctx.cfg.width;
    size_t offset_lower = (line_index + 1) * s_ctx.cfg.width;
    
    // Очищаем обе строки
    memset(&s_ctx.framebuffer[offset_upper], 0x00, s_ctx.cfg.width);
    memset(&s_ctx.framebuffer[offset_lower], 0x00, s_ctx.cfg.width);
    
    if (!text) {
        return;
    }
    
    // Крупный шрифт: каждый символ занимает 2x ширину и 2x высоту
    size_t max_chars = s_ctx.cfg.width / (OLED_FONT_WIDTH * 2);
    size_t length = strnlen(text, max_chars);
    size_t x_pos = 0;
    
    for (size_t i = 0; i < length && x_pos + (OLED_FONT_WIDTH * 2) <= s_ctx.cfg.width; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < 0x20 || c > 0x7F) {
            c = 0x20;
        }
        
        const uint8_t *glyph = s_font5x7[c - 0x20];
        
        // Отрисовка символа в двойном размере: каждый пиксель становится 2x2
        for (size_t col = 0; col < OLED_FONT_DATA_WIDTH; ++col) {
            uint8_t glyph_col = glyph[col];
            size_t x = x_pos + (col * 2);
            
            if (x + 1 < s_ctx.cfg.width) {
                // Масштабируем колонку: каждый бит становится 2 бита (2 пикселя по вертикали)
                // Для верхней строки (line_index)
                uint8_t upper_byte = 0;
                // Для нижней строки (line_index + 1) - дублируем верхнюю
                uint8_t lower_byte = 0;
                
                for (int bit = 0; bit < 7; ++bit) {  // 7 бит для высоты символа
                    if (glyph_col & (1 << bit)) {
                        // Каждый пиксель становится 2 пикселя по вертикали
                        upper_byte |= (1 << bit);
                        lower_byte |= (1 << bit);
                    }
                }
                
                // Дублируем по горизонтали (2 колонки)
                s_ctx.framebuffer[offset_upper + x] = upper_byte;
                s_ctx.framebuffer[offset_upper + x + 1] = upper_byte;
                s_ctx.framebuffer[offset_lower + x] = lower_byte;
                s_ctx.framebuffer[offset_lower + x + 1] = lower_byte;
            }
        }
        
        x_pos += OLED_FONT_WIDTH * 2; // Двойная ширина символа
    }
}

static void ssd1306_draw_line(size_t line_index, const char *text)
{
    if (line_index >= OLED_PAGE_COUNT(s_ctx.cfg.height)) {
        return;
    }

    size_t max_chars = s_ctx.cfg.width / OLED_FONT_WIDTH;
    size_t offset = line_index * s_ctx.cfg.width;
    memset(&s_ctx.framebuffer[offset], 0x00, s_ctx.cfg.width);

    if (!text) {
        return;
    }

    size_t length = strnlen(text, max_chars);
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < 0x20 || c > 0x7F) {
            c = 0x20;
        }
        const uint8_t *glyph = s_font5x7[c - 0x20];
        size_t x = offset + (i * OLED_FONT_WIDTH);
        for (size_t col = 0; col < OLED_FONT_DATA_WIDTH; ++col) {
            if (x + col < offset + s_ctx.cfg.width) {
                s_ctx.framebuffer[x + col] = glyph[col];
            }
        }
        size_t spacing_idx = x + OLED_FONT_DATA_WIDTH;
        if (spacing_idx < offset + s_ctx.cfg.width) {
            s_ctx.framebuffer[spacing_idx] = 0x00;
        }
    }
}

static void ssd1306_render_lines(void)
{
    for (size_t line = 0; line < s_ctx.line_count; ++line) {
        const char *rendered = s_ctx.rendered_lines[line];
        // Обычная отрисовка (крупный шрифт отключен)
        ssd1306_draw_line(line, rendered);
    }
}

static void refresh_display(bool force)
{
    if (!s_ctx.initialised) {
        return;
    }

    expand_templates();
    if (s_ctx.heartbeat_active && s_ctx.line_count > 0) {
        size_t len = strnlen(s_ctx.rendered_lines[0], OLED_MAX_LINE_TEXT_LEN - 1);
        if (len + strlen(HEART_SYMBOL) + 1 < OLED_MAX_LINE_TEXT_LEN) {
            if (len > 0 && len < OLED_MAX_LINE_TEXT_LEN - 1) {
                s_ctx.rendered_lines[0][len++] = ' ';
            }
            strlcpy(&s_ctx.rendered_lines[0][len], HEART_SYMBOL, OLED_MAX_LINE_TEXT_LEN - len);
        }
    }

    ssd1306_render_lines();

    for (uint8_t page = 0; page < OLED_PAGE_COUNT(s_ctx.cfg.height); ++page) {
        uint8_t set_page_cmds[] = {
            (uint8_t)(0xB0 | page),
            0x00,
            0x10
        };
        if (ssd1306_send_commands(set_page_cmds, sizeof(set_page_cmds)) != ESP_OK) {
            return;
        }
        size_t offset = page * s_ctx.cfg.width;
        size_t len = s_ctx.cfg.width;
        if (ssd1306_send_data(&s_ctx.framebuffer[offset], len) != ESP_OK) {
            return;
        }
    }

    (void)force;
}

static void apply_render_payload(const oled_render_payload_t *payload)
{
    if (!payload) {
        return;
    }
    s_ctx.current_values.count = payload->count;
    for (size_t i = 0; i < payload->count; ++i) {
        strlcpy(s_ctx.current_values.keys[i], payload->keys[i], OLED_MAX_KEY_LEN);
        strlcpy(s_ctx.current_values.values[i], payload->values[i], OLED_MAX_VALUE_LEN);
    }
}

static const char *find_value_for_key(const char *key)
{
    if (!key) {
        return "";
    }
    for (size_t i = 0; i < s_ctx.current_values.count; ++i) {
        if (strncmp(key, s_ctx.current_values.keys[i], OLED_MAX_KEY_LEN) == 0) {
            return s_ctx.current_values.values[i];
        }
    }
    return "";
}

static void expand_templates(void)
{
    char buffer[OLED_MAX_TEMPLATE_LEN];

    for (size_t line = 0; line < s_ctx.line_count; ++line) {
        const char *tmpl = s_ctx.templates[line];
        if (!tmpl[0]) {
            s_ctx.rendered_lines[line][0] = '\0';
            continue;
        }

        size_t out_idx = 0;
        size_t tmpl_len = strnlen(tmpl, OLED_MAX_TEMPLATE_LEN);
        for (size_t i = 0; i < tmpl_len && out_idx < OLED_MAX_LINE_TEXT_LEN - 1; ++i) {
            if (tmpl[i] == '{') {
                size_t start = i + 1;
                size_t end = start;
                while (end < tmpl_len && tmpl[end] != '}') {
                    ++end;
                }
                if (end < tmpl_len && tmpl[end] == '}') {
                    size_t key_len = end - start;
                    if (key_len < sizeof(buffer)) {
                        memcpy(buffer, &tmpl[start], key_len);
                        buffer[key_len] = '\0';
                        const char *value = find_value_for_key(buffer);
                        size_t value_len = strnlen(value, OLED_MAX_LINE_TEXT_LEN - 1);
                        size_t copy_len = value_len;
                        if (out_idx + copy_len >= OLED_MAX_LINE_TEXT_LEN - 1) {
                            copy_len = (OLED_MAX_LINE_TEXT_LEN - 1) - out_idx;
                        }
                        memcpy(&s_ctx.rendered_lines[line][out_idx], value, copy_len);
                        out_idx += copy_len;
                    }
                    i = end;
                    continue;
                }
            }
            s_ctx.rendered_lines[line][out_idx++] = tmpl[i];
        }
        s_ctx.rendered_lines[line][out_idx] = '\0';
    }
}

static void update_heartbeat_state(bool active)
{
    s_ctx.heartbeat_active = active;
    if (active) {
        s_ctx.heartbeat_deadline = xTaskGetTickCount() + s_ctx.heartbeat_timeout;
    } else {
        s_ctx.heartbeat_deadline = 0;
    }
}

static void heartbeat_timeout_check(void)
{
    if (s_ctx.heartbeat_active && s_ctx.heartbeat_deadline != 0) {
        if ((int32_t)(xTaskGetTickCount() - s_ctx.heartbeat_deadline) >= 0) {
            s_ctx.heartbeat_active = false;
            s_ctx.heartbeat_deadline = 0;
            refresh_display(false);
        }
    }
}

static void oled_task(void *arg)
{
    (void)arg;
    oled_queue_msg_t msg;

    while (1) {
        if (xQueueReceive(s_ctx.queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            switch (msg.type) {
                case OLED_MSG_RENDER:
                    apply_render_payload(&msg.data.render);
                    refresh_display(false);
                    break;
                case OLED_MSG_HEARTBEAT:
                    update_heartbeat_state(msg.data.heartbeat_active);
                    refresh_display(false);
                    break;
                default:
                    break;
            }
        }
        heartbeat_timeout_check();
    }
}

