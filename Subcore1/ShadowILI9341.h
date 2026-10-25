#ifndef __SHADOW_ILI9341_HEADER_GUARD__
#define __SHADOW_ILI9341_HEADER_GUARD__

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

/*
 * ILI9341 with a copy of the screen in RAM for the screen capture.
 * Reading back the LCD (RAMRD) returned only 0xff on this board,
 * so every drawing primitive also writes the pixels to the shadow buffer.
 * The other drawing functions of Adafruit_GFX (text, lines, rectangles)
 * end up in these primitives.
 */
#define SHADOW_WIDTH   (320)   /* after setRotation(3) */
#define SHADOW_HEIGHT  (240)

class ShadowILI9341 : public Adafruit_ILI9341 {
public:
  ShadowILI9341(SPIClass* spi, int8_t dc, int8_t cs, int8_t rst)
    : Adafruit_ILI9341(spi, dc, cs, rst) {}

  /* the pixels of the line y (RGB565) */
  uint16_t* shadowLine(int16_t y) { return &shadow[(int32_t)y * SHADOW_WIDTH]; }

  void drawPixel(int16_t x, int16_t y, uint16_t color) {
    shadowFill(x, y, 1, 1, color);
    Adafruit_ILI9341::drawPixel(x, y, color);
  }
  void writePixel(int16_t x, int16_t y, uint16_t color) {
    shadowFill(x, y, 1, 1, color);
    Adafruit_ILI9341::writePixel(x, y, color);
  }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    shadowFill(x, y, w, h, color);
    Adafruit_ILI9341::fillRect(x, y, w, h, color);
  }
  void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    shadowFill(x, y, w, h, color);
    Adafruit_ILI9341::writeFillRect(x, y, w, h, color);
  }
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    shadowFill(x, y, w, 1, color);
    Adafruit_ILI9341::drawFastHLine(x, y, w, color);
  }
  void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    shadowFill(x, y, w, 1, color);
    Adafruit_ILI9341::writeFastHLine(x, y, w, color);
  }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    shadowFill(x, y, 1, h, color);
    Adafruit_ILI9341::drawFastVLine(x, y, h, color);
  }
  void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    shadowFill(x, y, 1, h, color);
    Adafruit_ILI9341::writeFastVLine(x, y, h, color);
  }
  void fillScreen(uint16_t color) {
    shadowFill(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT, color);
    Adafruit_ILI9341::fillScreen(color);
  }

  using Adafruit_ILI9341::drawRGBBitmap;
  void drawRGBBitmap(int16_t x, int16_t y, uint16_t* pcolors, int16_t w, int16_t h) {
    for (int16_t j = 0; j < h; ++j) {
      int16_t yy = y + j;
      if (yy < 0 || yy >= SHADOW_HEIGHT) continue;
      for (int16_t i = 0; i < w; ++i) {
        int16_t xx = x + i;
        if (xx < 0 || xx >= SHADOW_WIDTH) continue;
        shadow[(int32_t)yy * SHADOW_WIDTH + xx] = pcolors[(int32_t)j * w + i];
      }
    }
    Adafruit_ILI9341::drawRGBBitmap(x, y, pcolors, w, h);
  }

private:
  static uint16_t shadow[SHADOW_WIDTH * SHADOW_HEIGHT];

  /* clip the rectangle to the screen and fill it in the shadow buffer */
  void shadowFill(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w < 0) { x += w + 1; w = -w; }
    if (h < 0) { y += h + 1; h = -h; }
    int16_t x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > SHADOW_WIDTH)  x1 = SHADOW_WIDTH;
    if (y1 > SHADOW_HEIGHT) y1 = SHADOW_HEIGHT;
    for (int16_t yy = y; yy < y1; ++yy) {
      uint16_t* p = &shadow[(int32_t)yy * SHADOW_WIDTH];
      for (int16_t xx = x; xx < x1; ++xx) p[xx] = color;
    }
  }
};

#endif /* __SHADOW_ILI9341_HEADER_GUARD__ */
