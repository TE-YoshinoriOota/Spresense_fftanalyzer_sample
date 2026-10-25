#include "AppSystem.h"

/*
 * Screen capture
 *  Subcore1 reads the LCD and sends it in strips from the bottom of the screen.
 *  The rows of a BMP file are stored from the bottom, so each strip is appended
 *  with its rows in the reverse order, and the file is written without seeking.
 *  The format is 16bit RGB565 (BI_BITFIELDS), the same as the LCD.
 */

#define CAP_BMP_HEADER_SIZE  (14 + 40 + 12)  /* file header, info header, RGB masks */

static File cap_file;
static uint16_t cap_file_no = 1;

static void cap_put_le16(uint8_t* p, uint16_t v) {
  p[0] = v & 0xff; p[1] = v >> 8;
}

static void cap_put_le32(uint8_t* p, uint32_t v) {
  p[0] = v & 0xff; p[1] = (v >> 8) & 0xff; p[2] = (v >> 16) & 0xff; p[3] = v >> 24;
}

static void cap_set_error(struct CapStatus* st, const char* msg) {
  st->state = CAP_STATE_ERROR;
  strncpy(st->message, msg, sizeof(st->message)-1);
  MPLog("Screen capture error: %s\n", msg);
  if (cap_file) cap_file.close();
}

static bool cap_open(struct CapStrip* s, struct CapStatus* st) {
  /* find an unused file name */
  char path[DATA_PATH_LEN];
  data_dir(CAP_DIR);
  for (int i = 0; i < 100000; ++i, ++cap_file_no) {
    if (cap_file_no == 0 || cap_file_no > 99999) cap_file_no = 1;
    sprintf(st->filename, "SCR%05d.BMP", cap_file_no);
    sprintf(path, CAP_DIR "/%s", st->filename);
    if (!theSD.exists(path)) break;
  }
  cap_file = theSD.open(path, FILE_WRITE);
  if (!cap_file) {
    cap_set_error(st, "SD OPEN");
    return false;
  }
  ++cap_file_no;

  uint32_t image_bytes = (uint32_t)s->width * 2 * s->height;
  uint8_t h[CAP_BMP_HEADER_SIZE];
  memset(h, 0, sizeof(h));
  h[0] = 'B'; h[1] = 'M';
  cap_put_le32(h+2,  CAP_BMP_HEADER_SIZE + image_bytes);
  cap_put_le32(h+10, CAP_BMP_HEADER_SIZE);       /* offset of the pixels */
  cap_put_le32(h+14, 40);                        /* info header size */
  cap_put_le32(h+18, s->width);
  cap_put_le32(h+22, s->height);                 /* positive: bottom-up */
  cap_put_le16(h+26, 1);                         /* planes */
  cap_put_le16(h+28, 16);                        /* bits per pixel */
  cap_put_le32(h+30, 3);                         /* BI_BITFIELDS */
  cap_put_le32(h+34, image_bytes);
  cap_put_le32(h+38, 2835);                      /* 72 dpi */
  cap_put_le32(h+42, 2835);
  cap_put_le32(h+54, 0xF800);                    /* red mask */
  cap_put_le32(h+58, 0x07E0);                    /* green mask */
  cap_put_le32(h+62, 0x001F);                    /* blue mask */
  if (cap_file.write(h, sizeof(h)) != sizeof(h)) {
    cap_set_error(st, "SD WRITE");
    return false;
  }
  return true;
}

/* save the CSV of the data on the screen: the buffers keep the data drawn last */
/* until the next request from Subcore1, so the CSV matches the BMP */
static void cap_save_csv() {
#ifdef ENABLE_SAVE_DATA
  if (!bSignalProcessingStarted || rec_is_active()) return; /* a menu page, or recording */
  pause_processing();
  switch (g_sid & SID_UMSK_SAVEDAT) {
  case SID_REQ_MONDATA:
    saveData(&sData);
    break;
  case SID_REQ_WAV_FFT:
  case SID_REQ_SPECTRO:
    if (fftWavData.len > 0) saveData(&fftWavData);
    break;
  case SID_REQ_FFT_FFT:
    if (fft2Data.len > 0) saveData(&fft2Data);
    break;
  case SID_REQ_WAV_WAV:
  case SID_REQ_RAW_FIL:
    if (wav2Data.len > 0) saveData(&wav2Data);
    break;
  }
  resume_processing();
#endif
}

/* called for each strip: the first strip is the bottom one, the last strip has y = 0 */
void cap_strip(struct CapStrip* s, struct CapStatus* st) {
  bool first = (s->y + s->rows >= s->height);
  if (first) {
    if (cap_file) cap_file.close();  /* the previous capture was broken off */
    memset(st, 0, sizeof(struct CapStatus));
    if (rec_is_active()) {           /* Subcore1 checks it too */
      cap_set_error(st, "REC BUSY");
      return;
    }
    if (!cap_open(s, st)) return;
  }
  if (st->state != CAP_STATE_OK || !cap_file) return;

  /* the width of the LCD (320) needs no padding: a BMP row is aligned to 4 bytes */
  const uint16_t* pixels = (const uint16_t*)s->data;
  size_t row_bytes = s->width * sizeof(uint16_t);
  for (int j = s->rows - 1; j >= 0; --j) {
    if (cap_file.write((const uint8_t*)(pixels + j * s->width), row_bytes) != row_bytes) {
      cap_set_error(st, "SD WRITE");
      return;
    }
  }

  if (s->y == 0) {
    cap_file.close();
    MPLog("Screen capture: %s\n", st->filename);
    cap_save_csv();
    st->state = CAP_STATE_DONE;
  }
}
