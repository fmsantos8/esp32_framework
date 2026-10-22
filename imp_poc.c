

struct app_button_config {
    button_t *component;                  /** Initialized component with polarity, debounce, and time source configured. */
    app_button_press_callback_t on_press; /** Optional receiver of a valid press. */
    void *press_context;                  /** Context passed to on_press. */
};

/** @brief Storage supplied by the integration; fields are private to the task. */
struct app_button {
    app_button_config_t config; /** Copied references without ownership transfer. */
    bool previous_pressed;      /** Last successfully read logical state. */
    bool initialized;           /** True after app_button_init() succeeds. */
};

static _Atomic uint32_t timer_notifications;

static void IRAM_ATTR timer_tick(void) {
    atomic_fetch_add_explicit(&timer_notifications, 1, memory_order_relaxed);
}

static uint32_t get_ticks(void) {
    return atomic_load_explicit(&timer_notifications, memory_order_relaxed);
}

void main(void) {

    const hal_system_config_t config = {
        .memory_buffer = hal_storage,
        .memory_capacity = (uint32_t)sizeof(hal_storage),
    };
    if (hal_system_init(&config) != HAL_STATUS_OK || bsp_init(timer_tick, &board) != HAL_STATUS_OK ||
        hal_system_seal() != HAL_STATUS_OK) {
        return;
    }

    button_config_t button_config = {
        .gpio = &board.button,
        .active_level = true,
        .debounce = 30,
        .get_ticks = get_ticks,
    };

    button_t button = {0};
}