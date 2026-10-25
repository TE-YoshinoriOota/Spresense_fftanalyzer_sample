#include "AppScreen.h"

/*
 * Lissajous figure: CH1 on the X axis and CH2 on the Y axis.
 * The waveforms come with SID_REQ_WAV_WAV (the same data as WAV DUAL).
 * lissBuf keeps the brightness (0-255) of each dot: every frame makes it darker
 * (the afterglow) and draws the new lines at 255. The rows are converted to RGB565
 * in lissStrip and sent to the LCD by LSJ_STRIP rows (an RGB565 buffer of the whole
 * graph does not fit in the heap of Subcore1).
 */

static uint16_t lissPalette[256];
static bool     lissPaletteReady = false;
static int      lissCh0 = 1;
static int      lissCh1 = 2;
static uint32_t lissTimeUs = 0;
static int      lissTimeCount = 0;

static uint16_t lissRgb(int r, int g, int b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/* the background to bright green, then to white green like a phosphor */
static void makeLissPalette() {
  for (int v = 0; v < 256; ++v) {
    int r, g, b;
    if (v < 192) {
      r = 0  + (30  - 0)*v/191;
      g = 6  + (230 - 6)*v/191;
      b = 28 + (90 - 28)*v/191;
    } else {
      r = 30  + (200 - 30)*(v - 192)/63;
      g = 230 + (255 - 230)*(v - 192)/63;
      b = 90  + (200 - 90)*(v - 192)/63;
    }
    lissPalette[v] = lissRgb(r, g, b);
  }
  lissPaletteReady = true;
}

/* the grid of 8 divisions and the axes through the center */
static uint16_t lissBack(int x, int y) {
  if (x == LSJ_SIZE/2 || y == LSJ_SIZE/2) return GRAPH_AXIS_COLOR;
  if (x % LSJ_GRID == 0 || y % LSJ_GRID == 0) return GRAPH_GRID_COLOR;
  return GRAPH_BG_COLOR;
}

static void writeLineToLiss(int x0, int y0, int x1, int y1) {
  int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
  int dy = -abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;
  while (true) {
    lissBuf[y0*LSJ_SIZE + x0] = 255;
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2*err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

static void sendLissajous() {
  for (int y0 = 0; y0 < LSJ_SIZE; y0 += LSJ_STRIP) {
    for (int j = 0; j < LSJ_STRIP; ++j) {
      int y = y0 + j;
      const uint8_t* src = lissBuf + y*LSJ_SIZE;
      uint16_t* dst = lissStrip + j*LSJ_SIZE;
      for (int x = 0; x < LSJ_SIZE; ++x) {
        uint8_t v = src[x];
        dst[x] = (v < LSJ_DIM) ? lissBack(x, y) : lissPalette[v];
      }
    }
    tft.drawRGBBitmap(LSJ_SIDE, LSJ_HEAD + y0, lissStrip, LSJ_SIZE, LSJ_STRIP);
  }
}

/* the full scale on the right of the graph */
static void putLissajousScale(int amp) {
  char text[16];
  tft.fillRect(LSJ_TEXT_SIDE, LSJ_HEAD + 52, SCREEN_WIDTH - LSJ_TEXT_SIDE, 10, ILI9341_BLACK);
  float fs = WAV_MAX_VOL/amp;
  if (fs < 10.0) sprintf(text, "+/-%.2f mV", fs);
  else           sprintf(text, "+/-%.1f mV", fs);
  putText(LSJ_TEXT_SIDE, LSJ_HEAD + 52, text, ILI9341_YELLOW, 1);
}

void buildLissajous(DynamicJsonDocument* jdoc) {
  if (!lissPaletteReady) makeLissPalette();
  lissCh0 = (*jdoc)["ch0"] | 1;
  lissCh1 = (*jdoc)["ch1"] | 2;
  tft.drawRect(LSJ_SIDE-1, LSJ_HEAD-1, LSJ_SIZE+2, LSJ_SIZE+2, ILI9341_YELLOW);
  char text[16];
  sprintf(text, "X: CH%d", lissCh0);
  putText(LSJ_TEXT_SIDE, LSJ_HEAD, text, ILI9341_CYAN, 1);
  sprintf(text, "Y: CH%d", lissCh1);
  putText(LSJ_TEXT_SIDE, LSJ_HEAD + 14, text, ILI9341_GREEN, 1);
  putText(LSJ_TEXT_SIDE, LSJ_HEAD + 40, "FULL SCALE", ILI9341_YELLOW, 1);
  if (bMonoFile) putText(LSJ_TEXT_SIDE, LSJ_HEAD + 80, "MONO FILE", ILI9341_RED, 1);
}

void appDrawLissajous(float* pX, float* pY, int len) {
  static int last_amp = -1;
  if (lissBuf == NULL || lissStrip == NULL) return;
  uint32_t t0 = micros();

  pthread_mutex_lock(&mtx);
  int g_amp = amp;
  pthread_mutex_unlock(&mtx);
  if (g_amp != last_amp || scale_update) {
    last_amp = g_amp;
    scale_update = false;
    memset(lissBuf, 0, LSJ_SIZE*LSJ_SIZE);  /* the trails of the old scale */
    putLissajousScale(g_amp);
  }

  /* the afterglow */
  for (int i = 0; i < LSJ_SIZE*LSJ_SIZE; ++i) lissBuf[i] = (lissBuf[i]*LSJ_DECAY) >> 8;

  /* full scale (WAV_MAX_VOL/amp mV) is the edge of the graph */
  float scale = (float)g_amp*(LSJ_SIZE/2 - 1);
  int px = 0, py = 0;
  for (int i = 0; i < len; ++i) {
    int x = LSJ_SIZE/2 + (int)(pX[i]*scale);
    int y = LSJ_SIZE/2 - (int)((bMonoFile ? 0.0f : pY[i])*scale);
    if (x < 0) x = 0; else if (x > LSJ_SIZE-1) x = LSJ_SIZE-1;
    if (y < 0) y = 0; else if (y > LSJ_SIZE-1) y = LSJ_SIZE-1;
    if (i > 0) writeLineToLiss(px, py, x, y);
    px = x;
    py = y;
  }
  sendLissajous();

  lissTimeUs += micros() - t0;
  if (++lissTimeCount >= GRAPH_TIME_FRAMES) {
    MPLog("lissajous draw %d us\n", (int)(lissTimeUs/lissTimeCount));
    lissTimeUs = 0;
    lissTimeCount = 0;
  }
}
