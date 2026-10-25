#include "AppSystem.h"
#include <pthread.h>
#include <errno.h>
#include <malloc.h>

/*
 * WAV recorder
 *  signal_processing() (capture thread) pushes the selected channels to a fifo by rec_push().
 *  rec_writer() (writer thread) pops the fifo and writes it to the WAV file on the SD card.
 *  The fifo absorbs the stall of SD card writing (sometimes more than 100 msec).
 */

#define REC_WRITER_PRIORITY  (20)    /* higher than the capture thread (10) */
#define REC_WRITER_STACK     (8192)
#define REC_WRITE_CHUNK      (4096)  /* samples (8KB) per one write: larger writes are not faster and stall longer */
#define REC_WAV_HEADER_SIZE  (512)   /* padded by a JUNK chunk so that the data is aligned to the SD sector */
#define REC_MAX_DATA_BYTES   (0xF0000000UL) /* keep the WAV size under 4GB */
/* the SD card wrote about 400KB/s (408000 B/s) at most (measured on 2026-10-20) */
/* 192kHz 1ch (384000 B/s) is allowed, but it may stop by the audio DMA error (after 30 sec in a test) */
#define REC_MAX_BYTES_PER_SEC (400000)

/* fifo: the size is power of 2 to wrap the index by a mask */
static int16_t* rec_fifo = NULL;
static uint32_t rec_fifo_size = 0;            /* samples */
static volatile uint32_t rec_head = 0;        /* total samples pushed */
static volatile uint32_t rec_tail = 0;        /* total samples popped */

static volatile bool rec_active = false;      /* capture thread pushes data while true */
static volatile bool rec_in_push = false;     /* capture thread is touching the fifo */
static volatile bool rec_draining = false;    /* writer thread exits when the fifo is empty */
static bool rec_running = false;              /* writer thread and the file are alive */
static int  rec_nch = 1;
static int  rec_ch[2] = {0, 0};               /* index in the captured frame */
static uint32_t rec_data_bytes = 0;
static uint16_t rec_file_no = 1;
static File rec_file;
static pthread_t rec_thread;


/* debug: log the heap to see how much memory is left for the fifo */
void rec_log_heap(const char* when) {
#ifdef REC_DEBUG
  struct mallinfo mi = mallinfo();
  MPLog("Heap %s: free %d KB, largest %d KB\n", when, mi.fordblks/1024, mi.mxordblk/1024);
#endif
}

static void rec_set_error(struct RecStatus* st, const char* msg) {
  st->state = REC_STATE_ERROR;
  strncpy(st->message, msg, sizeof(st->message)-1);
  MPLog("Recording error: %s\n", msg);
}

static void rec_put_le16(uint8_t* p, uint16_t v) {
  p[0] = v & 0xff; p[1] = v >> 8;
}

static void rec_put_le32(uint8_t* p, uint32_t v) {
  p[0] = v & 0xff; p[1] = (v >> 8) & 0xff; p[2] = (v >> 16) & 0xff; p[3] = v >> 24;
}

static void rec_write_header(uint32_t data_bytes) {
  uint8_t h[REC_WAV_HEADER_SIZE];
  uint16_t block_align = rec_nch * sizeof(int16_t);
  memset(h, 0, sizeof(h));
  memcpy(h, "RIFF", 4);
  rec_put_le32(h+4, REC_WAV_HEADER_SIZE - 8 + data_bytes);
  memcpy(h+8, "WAVEfmt ", 8);
  rec_put_le32(h+16, 16);                       /* fmt chunk size */
  rec_put_le16(h+20, 1);                        /* PCM */
  rec_put_le16(h+22, rec_nch);
  rec_put_le32(h+24, g_rate);
  rec_put_le32(h+28, g_rate * block_align);     /* byte rate */
  rec_put_le16(h+32, block_align);
  rec_put_le16(h+34, 16);                       /* bits per sample */
  memcpy(h+36, "JUNK", 4);                      /* padding chunk */
  rec_put_le32(h+40, REC_WAV_HEADER_SIZE - 44 - 8);
  memcpy(h+REC_WAV_HEADER_SIZE-8, "data", 4);
  rec_put_le32(h+REC_WAV_HEADER_SIZE-4, data_bytes);
  rec_file.seek(0);
  rec_file.write(h, REC_WAV_HEADER_SIZE);
}

/* allocate the fifo as large as possible after the signal processing took its memories */
static bool rec_alloc_fifo() {
  if (rec_fifo != NULL) return true;
  for (uint32_t size = 131072; size >= 16384; size >>= 1) {
    rec_fifo = (int16_t*)malloc(sizeof(int16_t)*size);
    if (rec_fifo != NULL) {
      rec_fifo_size = size;
      MPLog("Recording fifo: %d KB\n", (int)(size*sizeof(int16_t)/1024));
      return true;
    }
  }
  return false;
}

/* writer thread */
static void* rec_writer(void* arg) {
  struct RecStatus* st = (struct RecStatus*)arg;
  uint32_t mask = rec_fifo_size - 1;
  /* keep the half of the fifo free for the capture thread while writing */
  uint32_t chunk = (REC_WRITE_CHUNK < rec_fifo_size/2) ? REC_WRITE_CHUNK : rec_fifo_size/2;
  uint32_t last_dropped = 0;
  uint32_t max_write_ms = 0;          /* debug: the longest stall of SD writing */
  uint32_t max_fill = 0;              /* debug: the highest fifo level */
  uint32_t last_report = millis();
  uint32_t last_bytes = 0;

  while (true) {
    uint32_t avail = rec_head - rec_tail;
    if (avail > max_fill) max_fill = avail;
    if (bAudioError && st->state == REC_STATE_RECORDING) {
      rec_set_error(st, "AUDIO ERROR");  /* the capture stopped */
    }

#ifdef REC_DEBUG
    /* debug: report even if the writer is always busy */
    uint32_t now = millis();
    if (now - last_report >= 5000) {
      MPLog("Recording: %d KB written, %d KB/s, max write %d ms, max fifo %d/%d, dropped %d\n"
           , (int)(rec_data_bytes/1024)
           , (int)((rec_data_bytes - last_bytes) / 1024 * 1000 / (now - last_report))
           , (int)max_write_ms, (int)max_fill, (int)rec_fifo_size, (int)st->dropped);
      last_report = now;
      last_bytes = rec_data_bytes;
    }
#endif

    if (avail < chunk && !(rec_draining && avail > 0)) {
      if (rec_draining) break;
      if (st->dropped != last_dropped) {
        REC_DLOG("Recording: %d frames dropped\n", (int)st->dropped);
        last_dropped = st->dropped;
      }
      usleep(5000);
      continue;
    }

    uint32_t n = (avail < chunk) ? avail : chunk;
    uint32_t idx = rec_tail & mask;
    uint32_t first = rec_fifo_size - idx;
    if (first > n) first = n;
    uint32_t bytes = n * sizeof(int16_t);

    if (st->state == REC_STATE_RECORDING) {
      if (rec_data_bytes + bytes > REC_MAX_DATA_BYTES) {
        rec_set_error(st, "FILE FULL");
      } else {
        uint32_t t0 = millis();
        size_t w = rec_file.write((uint8_t*)&rec_fifo[idx], first*sizeof(int16_t));
        if (n > first) w += rec_file.write((uint8_t*)rec_fifo, (n-first)*sizeof(int16_t));
        uint32_t dt = millis() - t0;
        if (dt > max_write_ms) max_write_ms = dt;
        if (w != bytes) {
          int err = errno;
          MPLog("SD write failed: errno %d, %d/%d bytes, %d ms, at %d bytes\n"
               , err, (int)w, (int)bytes, (int)dt, (int)rec_data_bytes);
          rec_log_heap("at the error");
          rec_set_error(st, "SD WRITE");
        }
        else rec_data_bytes += bytes;
      }
    }
    /* the data is discarded after an error, to keep the capture running */
    rec_tail += n;
  }
  return NULL;
}


void rec_start(struct RecStatus* st) {
  memset(st, 0, sizeof(struct RecStatus));
  st->rate = g_rate;

  if (!bSignalProcessingStarted) {
    rec_set_error(st, "NOT READY");
    return;
  }
  if (g_src != SRC_MIC) {
    rec_set_error(st, "SET SRC TO MIC");  /* INPUT SOURCE -> MIC */
    return;
  }

  /* channels are numbered from 1 in sys.txt */
  rec_nch = (g_rech == 2) ? 2 : 1;
  rec_ch[0] = g_ch0 - 1;
  rec_ch[1] = g_ch1 - 1;
  for (int i = 0; i < rec_nch; ++i) {
    if (rec_ch[i] < 0 || rec_ch[i] >= g_chnm) {
      rec_set_error(st, "CH > AVAIL");
      return;
    }
  }
  if ((uint32_t)g_rate * rec_nch * sizeof(int16_t) > REC_MAX_BYTES_PER_SEC) {
    rec_set_error(st, "RATE TOO HIGH");
    return;
  }

  rec_log_heap("before the fifo");
  if (!rec_alloc_fifo()) {
    rec_set_error(st, "NO MEMORY");
    return;
  }

  /* find an unused file name */
  char path[DATA_PATH_LEN];
  data_dir(REC_DIR);
  for (int i = 0; i < 100000; ++i, ++rec_file_no) {
    if (rec_file_no == 0 || rec_file_no > 99999) rec_file_no = 1;
    sprintf(st->filename, "REC%05d.WAV", rec_file_no);
    sprintf(path, REC_DIR "/%s", st->filename);
    if (!theSD.exists(path)) break;
  }
  rec_file = theSD.open(path, FILE_WRITE);
  if (!rec_file) {
    rec_set_error(st, "SD OPEN");
    return;
  }
  ++rec_file_no;
  rec_data_bytes = 0;
  rec_write_header(0); /* the sizes are written in rec_stop() */

  rec_head = 0;
  rec_tail = 0;
  rec_draining = false;
  st->state = REC_STATE_RECORDING;

  struct sched_param param;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, REC_WRITER_STACK);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority = REC_WRITER_PRIORITY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  if (pthread_create(&rec_thread, &attr, rec_writer, st) != 0) {
    rec_file.close();
    rec_set_error(st, "NO THREAD");
    return;
  }

  rec_running = true;
  rec_active = true;
  MPLog("Recording start: %s %dch %dHz (CH%d", st->filename, rec_nch, (int)g_rate, rec_ch[0]+1);
  if (rec_nch == 2) MPLog(",CH%d", rec_ch[1]+1);
  MPLog(")\n");
}


void rec_stop(struct RecStatus* st) {
  if (!rec_running) return; /* not started or failed to start */

  /* stop pushing, write the rest of the fifo and finish the writer thread */
  rec_active = false;
  __sync_synchronize();
  while (rec_in_push) usleep(1); /* the capture thread leaves rec_push() soon */
  rec_draining = true;
  pthread_join(rec_thread, NULL);

  rec_write_header(rec_data_bytes);
  rec_file.close();

  /* give the memory back now: stopping the audio library needs it */
  rec_release();
  rec_log_heap("after closing");
  rec_running = false;
  if (st->state == REC_STATE_RECORDING) st->state = REC_STATE_STOPPED;
  MPLog("Recording stop: %s %d bytes, %d frames dropped\n"
       , st->filename, (int)rec_data_bytes, (int)st->dropped);
}


/* called from the capture thread: copy the selected channels of the captured frames */
void rec_push(const int16_t* raw, int frames) {
  if (!rec_active) return;

  /* tell rec_stop() not to free the fifo, then check again (rec_stop() may have just cleared rec_active) */
  rec_in_push = true;
  __sync_synchronize();
  if (!rec_active) {
    rec_in_push = false;
    return;
  }

  uint32_t need = frames * rec_nch;
  if (rec_fifo_size - (rec_head - rec_tail) < need) {
    recStatus.dropped += frames;
    rec_in_push = false;
    return;
  }

  uint32_t mask = rec_fifo_size - 1;
  uint32_t h = rec_head;
  for (int n = 0; n < frames; ++n) {
    const int16_t* frame = raw + n*g_chnm;
    rec_fifo[h++ & mask] = frame[rec_ch[0]];
    if (rec_nch == 2) rec_fifo[h++ & mask] = frame[rec_ch[1]];
  }
  __sync_synchronize(); /* the data must be in the fifo before the writer sees the new head */
  rec_head = h;
  recStatus.frames += frames;
  rec_in_push = false;
}


/* free the fifo: call this when the capture thread does not push (rec_active is false) */
void rec_release() {
  if (rec_fifo != NULL) free(rec_fifo);
  rec_fifo = NULL;
  rec_fifo_size = 0;
}


bool rec_is_active() {
  return rec_running;
}
