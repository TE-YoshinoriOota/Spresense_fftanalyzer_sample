#include "AppScreen.h"

/*
 * Screen capture
 *  The screen is taken from the shadow buffer of tft (the LCD can't be read back on this board)
 *  in strips of CAP_STRIP_ROWS lines, from the bottom of the screen,
 *  and each strip is sent to Maincore which writes it to a BMP file on the SD card.
 *  The main loop does not draw while capturing, and the button interrupts are ignored.
 */

/* the copy of the screen kept by tft (150KB) */
uint16_t ShadowILI9341::shadow[SHADOW_WIDTH * SHADOW_HEIGHT];

static struct CapStrip capStrip;

/* get and clear the request set by BACK + NEXT */
bool takeCaptureRequest() {
  noInterrupts();
  bool req = capture_request;
  capture_request = false;
  interrupts();
  return req;
}

/* wait for the reply of a strip, the other replies should not come here */
static struct CapStatus* waitCapReply() {
  int8_t sid;
  void* data;
  uint32_t t0 = millis();
  while (millis() - t0 < CAP_REPLY_TIMEOUT) {
    if (MP.Recv(&sid, &data) < 0) continue;
    if (sid == SID_REQ_SCRCAP) return (struct CapStatus*)data;
    if (sid == SID_REQ_RECSTRT || sid == SID_REQ_RECSTOP) {
      /* the reply of REC/STOP pressed just before */
      if (scrType == SCR_TYPE_RECM) recStat = (struct RecStatus*)data;
      continue;
    }
    MPLog("Screen capture: unexpected sid 0x%02x\n", sid);
  }
  return NULL;
}

void captureScreen() {
  if (bRecording) {
    MPLog("Screen capture: REC BUSY\n");
    if (scrType == SCR_TYPE_RECM) {
      tft.fillRect(REC_STAT_SIDE, REC_STAT1_HEAD, REC_STAT_WIDTH, 8, ILI9341_BLACK);
      putText(REC_STAT_SIDE, REC_STAT1_HEAD, "REC BUSY", ILI9341_RED, 1);
    }
    return;
  }

  bCapturing = true;
  uint32_t t0 = millis();
  struct CapStatus* st = NULL;
  for (int16_t y = SCREEN_HEIGHT; y > 0; ) {
    int16_t rows = (y < CAP_STRIP_ROWS) ? y : CAP_STRIP_ROWS;
    y -= rows;
    capStrip.y      = y;
    capStrip.rows   = rows;
    capStrip.width  = SCREEN_WIDTH;
    capStrip.height = SCREEN_HEIGHT;
    capStrip.data   = MP.Virt2Phys(tft.shadowLine(y)); /* the lines are contiguous */
    int ret = MP.Send(SID_REQ_SCRCAP, &capStrip);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) {
        error_notifier(MP_ERROR);
      }
    }
    st = waitCapReply();
    if (st == NULL || st->state != CAP_STATE_OK) break;
  }
  bCapturing = false;

  if (st == NULL) {
    MPLog("Screen capture: no reply from Maincore\n");
  } else if (st->state == CAP_STATE_DONE) {
    MPLog("Screen capture: %s (%d ms)\n", st->filename, (int)(millis() - t0));
  } else {
    MPLog("Screen capture failed: %s\n", st->message);
  }
}
