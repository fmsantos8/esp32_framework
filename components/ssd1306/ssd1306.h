/**
 * @file ssd1306.h
 * @brief Framebuffer graphics and incremental updates for a 128x64 I2C SSD1306.
 */
#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Fixed panel geometry, in pixels, and framebuffer capacity, in bytes. */
enum {
    SSD1306_WIDTH = 128,
    SSD1306_HEIGHT = 64,
    SSD1306_FRAMEBUFFER_SIZE = SSD1306_WIDTH * SSD1306_HEIGHT / 8
};

/** @brief Caller-owned context; its storage definition is in ssd1306_config.h. */
typedef struct ssd1306 ssd1306_t;

/** @brief Construction dependencies, defined in ssd1306_config.h. */
typedef struct ssd1306_config ssd1306_config_t;

/** @brief Component results, independent of HAL and SDK status types. */
typedef enum {
    SSD1306_STATUS_OK = 0,             /** Operation completed successfully. */
    SSD1306_STATUS_INVALID_ARGUMENT,   /** A required pointer, coordinate, size, or address is invalid. */
    SSD1306_STATUS_NOT_INITIALIZED,    /** The display or its I2C dependency is not initialized. */
    SSD1306_STATUS_NOT_SUPPORTED,      /** The selected font, character, or HAL operation is unavailable. */
    SSD1306_STATUS_TIMEOUT,            /** The configured I2C driver timeout expired. */
    SSD1306_STATUS_COMMUNICATION_ERROR /** The I2C dependency could not complete a transfer. */
} ssd1306_status_t;

/** @brief Built-in monochrome fonts; unsupported characters are rejected. */
typedef enum {
    SSD1306_FONT_SMALL = 0, /** 5x7 pixels, printable ASCII and the UTF-8 degree symbol. */
    SSD1306_FONT_MEDIUM,    /** Proportional font, 16 pixels high, printable ASCII and the UTF-8 degree symbol. */
    SSD1306_FONT_LARGE      /** Proportional numerals, 28 pixels high: space, digits, '+', '-', '.', ','. */
} ssd1306_font_t;

/**
 * @brief Monochrome image, stored as rows of ceil(width / 8) bytes, MSB first.
 *
 * @details Zero bits clear pixels and one bits set pixels. Unused low bits in
 * each row's final byte are ignored. The caller retains ownership of data.
 */
typedef struct {
    const uint8_t *data; /** Readable source bytes, independent of the display context. */
    uint32_t size;       /** Available byte count; must cover every row. */
    uint32_t width;      /** Positive image width in pixels. */
    uint32_t height;     /** Positive image height in pixels. */
} ssd1306_bitmap_t;

/**
 * @brief Configure the controller, clear its RAM while off, and turn it on.
 *
 * @details Initializes the entire caller-owned context; no prior zeroing is
 * required. Supply the dependencies and storage through ssd1306_config.h.
 * Invalid arguments leave the context unchanged. Once a valid attempt starts,
 * the context is replaced and remains unavailable until initialization succeeds.
 * Initialization can be retried, including after failure, and clears all drawing.
 * A failed transfer can leave the physical panel partly configured.
 *
 * All operations require serialized calls from normal execution context. They
 * provide no synchronization or ISR support. Share the I2C bus sequentially with
 * other components. The context and bus must outlive all uses. Do not modify the
 * context fields or overlap it with inputs or outputs. Apart from init(), calls
 * require a context previously passed to init() (or initially zero-initialized).
 * No dynamic allocation, arena reservation, bus construction, or SDK call is
 * performed by this component.
 *
 * @param[out] display Required caller-owned context.
 * @param[in] config Required initialized dependencies.
 * @return Component status; hardware transfers are synchronous and use the
 * timeout retained by the supplied I2C bus.
 */
ssd1306_status_t ssd1306_init(ssd1306_t *display, const ssd1306_config_t *config);

/**
 * @brief Clear the framebuffer, marking only changed pages for a later update.
 * @details No I2C transfer occurs. Drawing calls below likewise affect RAM only.
 * A clear followed by drawing does not send an intermediate blank frame.
 */
ssd1306_status_t ssd1306_clear(ssd1306_t *display);

/**
 * @brief Set or clear a nonempty rectangle entirely inside the panel.
 * @details Coordinates start at the top left. Bounds are checked before any
 * pixels change; oversized shapes are rejected instead of clipped.
 */
ssd1306_status_t ssd1306_fill_rect(ssd1306_t *display, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool on);

/** @brief Set or clear a line, including both endpoints, which must be inside the panel. */
ssd1306_status_t ssd1306_draw_line(ssd1306_t *display, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, bool on);

/**
 * @brief Draw white text on a transparent background, using a top-left origin.
 *
 * @details Only printable ASCII and the UTF-8 degree symbol (C2 B0) are accepted
 * by the small and medium fonts. The large font supports the subset documented
 * above. No wrapping, newline, or general Unicode support is provided. Use
 * fill_rect(..., false) to erase an old value before drawing a replacement.
 *
 * The entire text and its bounds are checked before modifying any pixels.
 * Empty text is accepted and draws nothing; its font-height box must still fit.
 * A required pointer or invalid bounds returns INVALID_ARGUMENT; unsupported
 * text or font returns NOT_SUPPORTED. All drawing calls return NOT_INITIALIZED
 * for an unavailable context, and never perform I2C transfers.
 */
ssd1306_status_t ssd1306_draw_text(ssd1306_t *display, uint32_t x, uint32_t y, const char *text, ssd1306_font_t font);

/**
 * @brief Measure the same text box used by draw_text(), without a display.
 * @details Width includes one pixel between glyphs and no trailing gap. Height
 * is the font's fixed height, including for empty text (width zero). Outputs
 * must be non-NULL, distinct, and are written only on success. Unsupported text
 * or font returns NOT_SUPPORTED; invalid pointers or width overflow returns
 * INVALID_ARGUMENT. Text must be a readable NUL-terminated string.
 */
ssd1306_status_t ssd1306_measure_text(const char *text, ssd1306_font_t font, uint32_t *out_width, uint32_t *out_height);

/**
 * @brief Copy an opaque bitmap entirely inside the panel into the framebuffer.
 * @details Validates the source extent and bounds before modifying any pixels.
 */
ssd1306_status_t ssd1306_draw_bitmap(ssd1306_t *display, uint32_t x, uint32_t y, const ssd1306_bitmap_t *bitmap);

/**
 * @brief Transfer at most one dirty 128x8 page, then report pending work.
 *
 * @details A dirty page uses two synchronous I2C writes: address commands and
 * 129 bytes of control/data. A clean framebuffer requires no I2C traffic. This
 * bounds work per call but does not make the transfer asynchronous or guarantee
 * a strict whole-call deadline. Changed pages become visible progressively;
 * updates are not an atomic frame swap.
 *
 * On success, out_pending reports whether dirty pages remain. On failure, the
 * output is unchanged and the page remains dirty for a later retry. Both address
 * and data are resent on retry, even after a partially completed transfer.
 * Recovery after a controller reset/power loss requires init() and redrawing.
 * The caller chooses when to call again or retry a communication failure.
 *
 * @param[in,out] display Required initialized context.
 * @param[out] out_pending Required output, written only on success.
 * @return OK, INVALID_ARGUMENT, NOT_INITIALIZED, NOT_SUPPORTED, TIMEOUT, or
 * COMMUNICATION_ERROR, according to validation and the I2C result.
 */
ssd1306_status_t ssd1306_update(ssd1306_t *display, bool *out_pending);

#ifdef __cplusplus
}
#endif

#endif
