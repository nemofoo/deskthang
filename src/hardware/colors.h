#ifndef __COLORS_H
#define __COLORS_H

// 16-bit color definitions (RGB565 format)
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_MAGENTA   0xF81F
#define COLOR_CYAN      0x07FF
#define COLOR_ORANGE    0xFD20
#define COLOR_PURPLE    0x8010
#define COLOR_PINK      0xFE19
#define COLOR_GRAY      0x8410

// Color conversion macro (for RGB565 format)
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

#endif
