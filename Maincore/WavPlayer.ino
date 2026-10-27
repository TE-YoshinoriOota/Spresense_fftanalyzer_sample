#include "AppSystem.h"

/*
 * WAV player: the input of the signal processing when src of sys.txt is FILE.
 *  play_read() is called by the capture thread instead of theAudio->readFrames().
 *  It reads the frames for the elapsed time, so the file is played at the recorded speed.
 *  At the end of the file it goes back to the top of the data.
 *  play_list_files() makes the file list page (AA810) of the RECnnnnn.WAV files.
 *  FILE (not FILE + SOUND) can add a second mono file of the same rate (AA815):
 *  the two files are read as CH1 and CH2, and both go back to the top at the end of the shorter.
 */

static File     play_file;
static bool     play_opened     = false;
static uint32_t play_data_start = 0;  /* offset of the data chunk */
static uint32_t play_data_bytes = 0;
static uint32_t play_pos        = 0;  /* bytes read from the data chunk */
static int      play_frame_bytes = 2;
static uint32_t play_rate       = 0;
static uint32_t play_start_us   = 0;
static uint32_t play_frames_out = 0;  /* frames read since play_start_us */
static volatile bool play_failed = false;  /* the file could not be read: shown on the graph */

/* the second mono file to compare (CH2) */
static File     cmp_file;
static bool     cmp_opened      = false;
static uint32_t cmp_data_start  = 0;
static uint32_t cmp_data_bytes  = 0;
static uint32_t cmp_pos         = 0;
#define PLAY_PAIR_CHUNK (1024) /* frames read from each file at a time (fewer reads of the SD card) */
/* not on the stack: the capture thread has a small stack (the default of pthread) */
/* aligned: the FAT reads whole sectors into the buffer by DMA, which times out (ETIMEDOUT) */
/* on a buffer of 2-byte alignment (the buffer of play_read() comes from malloc)             */
static int16_t  pair_a[PLAY_PAIR_CHUNK] __attribute__((aligned(32)));
static int16_t  pair_b[PLAY_PAIR_CHUNK] __attribute__((aligned(32)));

/* read the bytes: a read may return less than asked */
static bool play_read_all(File& f, uint8_t* p, int bytes) {
  while (bytes > 0) {
    int r = f.read(p, bytes);
    if (r <= 0) {
      MPLog("Play: read %d of %d bytes (errno %d)\n", r, bytes, errno);
      return false;
    }
    p += r;
    bytes -= r;
  }
  return true;
}

static uint32_t play_le32(const uint8_t* p) {
  return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t play_le16(const uint8_t* p) {
  return p[0] | (p[1] << 8);
}

static void play_file_name(int no, char* name) {
  sprintf(name, REC_DIR "/REC%05d.WAV", no);
}

/* find the fmt and data chunks: false if the file is not 16bit PCM */
static bool play_parse(File& f, struct PlayInfo* info) {
  uint8_t h[16];
  uint32_t size = f.size();
  if (size < 44) return false;
  f.seek(0);
  if (f.read(h, 12) != 12) return false;
  if (memcmp(h, "RIFF", 4) != 0 || memcmp(h+8, "WAVE", 4) != 0) return false;

  bool has_fmt = false;
  uint32_t pos = 12;
  while (pos + 8 <= size) {
    f.seek(pos);
    if (f.read(h, 8) != 8) return false;
    uint32_t len = play_le32(h+4);
    if (memcmp(h, "fmt ", 4) == 0) {
      if (len < 16 || f.read(h, 16) != 16) return false;
      int format = play_le16(h);
      info->chnm = play_le16(h+2);
      info->rate = play_le32(h+4);
      int bits   = play_le16(h+14);
      if (format != 1 || bits != 16) return false;
      if (info->chnm < 1 || info->chnm > MAX_CHANNEL || info->rate == 0) return false;
      has_fmt = true;
    } else if (memcmp(h, "data", 4) == 0) {
      if (!has_fmt) return false;
      info->data_start = pos + 8;
      /* the size is not written back if the recording did not stop normally */
      if (len == 0 || info->data_start + len > size) len = size - info->data_start;
      info->data_bytes = len - len % (info->chnm * sizeof(int16_t));
      return info->data_bytes > 0;
    }
    pos += 8 + len + (len & 1);
  }
  return false;
}

/* read the header only (for the page of the file list and the screens) */
bool play_probe(int no, struct PlayInfo* info) {
  char name[DATA_PATH_LEN];
  play_file_name(no, name);
  if (no <= 0 || !theSD.exists(name)) return false;
  File f = theSD.open(name, FILE_READ);
  if (!f) return false;
  bool ok = play_parse(f, info);
  f.close();
  return ok;
}

bool play_open(int no, struct PlayInfo* info) {
  char name[DATA_PATH_LEN];
  play_close();
  play_file_name(no, name);
  if (no <= 0 || !theSD.exists(name)) {
    MPLog("Play: %s not found\n", name);
    return false;
  }
  play_file = theSD.open(name, FILE_READ);
  if (!play_file) {
    MPLog("Play: cannot open %s\n", name);
    return false;
  }
  if (!play_parse(play_file, info)) {
    MPLog("Play: %s is not 16bit PCM\n", name);
    play_file.close();
    return false;
  }
  play_data_start  = info->data_start;
  play_data_bytes  = info->data_bytes;
  play_frame_bytes = info->chnm * sizeof(int16_t);
  play_rate        = info->rate;
  play_pos         = 0;
  play_file.seek(play_data_start);
  play_opened = true;
  play_failed = false;
  MPLog("Play: %s %dch %dHz %d bytes\n", name, info->chnm, (int)info->rate, (int)play_data_bytes);
  return true;
}

void play_close() {
  if (play_opened) play_file.close();
  play_opened = false;
  if (cmp_opened) cmp_file.close();
  cmp_opened = false;
}

/* open the second mono file of the same rate as the first one: the frames become stereo */
bool play_open_compare(int no, const struct PlayInfo* first) {
  char name[DATA_PATH_LEN];
  struct PlayInfo info;
  if (cmp_opened) cmp_file.close();
  cmp_opened = false;
  if (!play_opened || first->chnm != 1 || no <= 0) return false;
  play_file_name(no, name);
  if (!theSD.exists(name)) return false;
  cmp_file = theSD.open(name, FILE_READ);
  if (!cmp_file) return false;
  if (!play_parse(cmp_file, &info) || info.chnm != 1 || info.rate != first->rate) {
    MPLog("Play: %s can't be compared\n", name);
    cmp_file.close();
    return false;
  }
  cmp_data_start = info.data_start;
  cmp_data_bytes = info.data_bytes;
  cmp_pos = 0;
  cmp_file.seek(cmp_data_start);
  play_pos = 0;
  play_file.seek(play_data_start);
  play_frame_bytes = 2 * sizeof(int16_t);
  cmp_opened = true;
  MPLog("Play: compare with %s\n", name);
  return true;
}

/* the two mono files into the stereo frames: false at a read error */
static bool play_read_pair(int16_t* out, uint32_t frames) {
  int16_t* a = pair_a;
  int16_t* b = pair_b;
  while (frames > 0) {
    uint32_t n = frames;
    if (n > PLAY_PAIR_CHUNK) n = PLAY_PAIR_CHUNK;
    uint32_t ra = (play_data_bytes - play_pos) / sizeof(int16_t);
    uint32_t rb = (cmp_data_bytes - cmp_pos) / sizeof(int16_t);
    if (ra == 0 || rb == 0) {
      /* the end of the shorter file: both go back to the top to keep them together */
      play_pos = 0;
      play_file.seek(play_data_start);
      cmp_pos = 0;
      cmp_file.seek(cmp_data_start);
      continue;
    }
    if (n > ra) n = ra;
    if (n > rb) n = rb;
    int bytes = n * sizeof(int16_t);
    if (!play_read_all(play_file, (uint8_t*)a, bytes)) {
      MPLog("Play: read error (CH1 at %d)\n", (int)play_pos);
      return false;
    }
    if (!play_read_all(cmp_file, (uint8_t*)b, bytes)) {
      MPLog("Play: read error (CH2 at %d)\n", (int)cmp_pos);
      return false;
    }
    play_pos += bytes;
    cmp_pos += bytes;
    for (uint32_t i = 0; i < n; ++i) {
      out[2*i]   = a[i];
      out[2*i+1] = b[i];
    }
    out += 2 * n;
    frames -= n;
  }
  return true;
}

/* "SD READ ERROR" on the graph while the file can't be read (every SAVE_TEXT_MS of Subcore1) */
/* call it before the data is sent: Subcore1 shows it with the result of saving the data     */
void play_notify_error() {
  static uint32_t last_ms = 0;
  static struct SaveStatus msg;
  if (!play_failed || millis() - last_ms < PLAY_ERROR_MS) return;
  last_ms = millis();
  memset(&msg, 0, sizeof(msg));
  msg.state = SAVE_STATE_MESSAGE;
  strcpy(msg.filename, "SD READ ERROR");
  MP.Send(SID_REQ_SAVED, &msg, SUBCORE);
}

/* call it when the capture thread starts */
void play_restart_clock() {
  play_start_us   = micros();
  play_frames_out = 0;
}

/* read the frames for the elapsed time into buf: returns the bytes read */
int play_read(char* buf, int max_frames) {
  if (!play_opened) return 0;

  uint32_t elapsed = micros() - play_start_us;
  uint32_t due = (uint32_t)((uint64_t)elapsed * play_rate / 1000000);
  if (due <= play_frames_out) return 0;
  uint32_t frames = due - play_frames_out;
  if (frames > (uint32_t)max_frames) {
    /* too late (e.g. the SD card was busy): skip the time, not the data */
    play_frames_out = due - max_frames;
    frames = max_frames;
  }

  uint32_t want = frames * play_frame_bytes;
  uint32_t done = 0;
  if (cmp_opened) {
    if (play_read_pair((int16_t*)buf, frames)) {
      done = want;
    } else {
      MPLog("Play: read error\n");
      play_close();
      play_failed = true;
    }
  }
  while (!cmp_opened && play_opened && done < want) {
    if (play_pos >= play_data_bytes) {
      /* the end of the file: play it again from the top */
      play_pos = 0;
      play_file.seek(play_data_start);
    }
    uint32_t n = want - done;
    if (n > play_data_bytes - play_pos) n = play_data_bytes - play_pos;
    int r = play_file.read(buf + done, n);
    if (r <= 0) {
      MPLog("Play: read error\n");
      play_close();
      play_failed = true;
      break;
    }
    done += r;
    play_pos += r;
  }
  done -= done % play_frame_bytes;
  play_frames_out += done / play_frame_bytes;

  /* move the base time forward to keep the numbers small */
  if (play_frames_out >= play_rate) {
    play_frames_out -= play_rate;
    play_start_us += 1000000;
  }
  return done;
}

/*
 * sound output without the decoder DSP:
 *  the PCM of the same file (opened again) is sent to the OutputMixer (renderer) directly.
 *  The renderer takes 16bit stereo at 48kHz (or 192kHz in the HiReso clock mode),
 *  so a mono file is copied to both channels.
 *  The capture thread reads the file into snd_bridge (snd_fill()),
 *  and the done callback of the mixer sends the next frame from snd_bridge.
 *  play_read() reads the frames for the elapsed time from the start of the sound,
 *  so the sound and the graphs go together.
 */
#define SND_FRAME_SAMPLES (1024)
#define SND_FRAME_BYTES   (SND_FRAME_SAMPLES * 2 * sizeof(int16_t))  /* stereo */
#define SND_INFLIGHT      (4)          /* frames sent to the mixer and not done yet */
#define SND_BRIDGE_SIZE   (96*1024)

static OutputMixer*      snd_mixer = NULL;
static File              snd_file;
static bool              snd_opened = false;
static uint32_t          snd_pos = 0;          /* bytes read from the data chunk */
static int               snd_chnm = 2;
static uint8_t*          snd_bridge = NULL;    /* ring buffer of the stereo PCM */
static volatile uint32_t snd_wp = 0;
static volatile uint32_t snd_rp = 0;
static volatile int      snd_inflight = 0;
static volatile bool     snd_running = false;
static volatile bool     snd_end_done = false;
static pthread_mutex_t   snd_mtx = PTHREAD_MUTEX_INITIALIZER;

/* the digital filters on the sound: the same LPF and HPF as the graphs */
/* the sound is a continuous stream, so each channel keeps the state of its own filters */
#define SND_FILT_MAX  (2)   /* LPF and HPF */
struct SndBiquad {
  float32_t c[5];           /* b0, b1, b2, a1, a2 (df2T of CMSIS) */
  float32_t d1[2], d2[2];   /* the state of L and R */
};
static struct SndBiquad snd_filt[SND_FILT_MAX];
static int snd_nfilt = 0;

static void snd_filter_add(filterType_t type, int cutoff, int rate) {
  IIRClassKai iir;
  iir.begin(type, cutoff, rate, 0.5);  /* the same Q as the graphs */
  struct SndBiquad* f = &snd_filt[snd_nfilt++];
  memcpy(f->c, iir.coef(), sizeof(f->c));
  memset(f->d1, 0, sizeof(f->d1));
  memset(f->d2, 0, sizeof(f->d2));
}

static void snd_filter_init(int rate) {
  snd_nfilt = 0;
  if (g_lpf != 0) snd_filter_add(TYPE_LPF, g_lpf, rate);
  if (g_hpf != 0) snd_filter_add(TYPE_HPF, g_hpf, rate);
  MPLog("Sound: filter LPF %d HPF %d\n", g_lpf, g_hpf);
}

/* filter the frames of the file in place (before the mono expansion) */
static void snd_filter(int16_t* p, int frames, int chnm) {
  if (snd_nfilt == 0) return;
  if (chnm > 2) chnm = 2;
  for (int i = 0; i < frames; ++i) {
    for (int ch = 0; ch < chnm; ++ch) {
      float32_t x = p[i*chnm + ch];
      for (int k = 0; k < snd_nfilt; ++k) {
        struct SndBiquad* f = &snd_filt[k];
        float32_t y = f->c[0]*x + f->d1[ch];
        f->d1[ch] = f->c[1]*x + f->c[3]*y + f->d2[ch];
        f->d2[ch] = f->c[2]*x + f->c[4]*y;
        x = y;
      }
      /* saturate: a filter can make the peaks higher than the input */
      if (x > 32767.0f) x = 32767.0f;
      else if (x < -32768.0f) x = -32768.0f;
      p[i*chnm + ch] = (int16_t)x;
    }
  }
}

static void snd_attention_cb(const ErrorAttentionParam *atprm) {
  MPLog("Sound: attention %d/%d\n", atprm->error_code, (int)atprm->error_att_sub_code);
}

static void snd_mixer_done_cb(MsgQueId requester_dtq, MsgType reply_of, AsOutputMixDoneParam *done_param) {
}

static uint32_t snd_used() {
  return (snd_wp + SND_BRIDGE_SIZE - snd_rp) % SND_BRIDGE_SIZE;
}

static void snd_done_cb(int32_t identifier, bool is_end);

/* send a frame from snd_bridge (zeros if it is empty) */
static bool snd_send_frame(bool is_end) {
  AsPcmDataParam pcm;
  if (pcm.mh.allocSeg(S0_REND_PCM_BUF_POOL, SND_FRAME_BYTES) != ERR_OK) return false;
  uint8_t* dst = (uint8_t*)pcm.mh.getPa();
  uint32_t n = snd_used();
  if (n > SND_FRAME_BYTES) n = SND_FRAME_BYTES;
  n -= n % 4;
  uint32_t rp = snd_rp;
  uint32_t first = SND_BRIDGE_SIZE - rp;
  if (first > n) first = n;
  memcpy(dst, snd_bridge + rp, first);
  memcpy(dst + first, snd_bridge, n - first);
  if (n < SND_FRAME_BYTES) memset(dst + n, 0, SND_FRAME_BYTES - n);  /* underflow: silence */
  __sync_synchronize();
  snd_rp = (rp + n) % SND_BRIDGE_SIZE;

  pcm.identifier = 0;
  pcm.callback   = 0;
  pcm.bit_length = 16;
  pcm.size       = SND_FRAME_BYTES;
  pcm.sample     = SND_FRAME_SAMPLES;
  pcm.is_end     = is_end;
  pcm.is_valid   = true;
  __sync_fetch_and_add(&snd_inflight, 1);
  if (snd_mixer->sendData(OutputMixer0, snd_done_cb, pcm) != OUTPUTMIXER_ECODE_OK) {
    __sync_fetch_and_sub(&snd_inflight, 1);
    return false;
  }
  return true;
}

/* keep SND_INFLIGHT frames in the mixer */
static void snd_top_up() {
  pthread_mutex_lock(&snd_mtx);
  while (snd_running && snd_inflight < SND_INFLIGHT) {
    if (!snd_send_frame(false)) break;
  }
  pthread_mutex_unlock(&snd_mtx);
}

/* called by the mixer when a frame is played */
static void snd_done_cb(int32_t identifier, bool is_end) {
  __sync_fetch_and_sub(&snd_inflight, 1);
  if (is_end) {
    snd_end_done = true;
    return;
  }
  snd_top_up();
}

/* read the file into snd_bridge: called by the capture thread */
void snd_fill() {
  static uint8_t buf[SND_CHUNK];
  if (!snd_opened || snd_bridge == NULL) return;
  while (true) {
    uint32_t space = SND_BRIDGE_SIZE - 1 - snd_used();
    /* bytes to read from the file for the space (stereo in snd_bridge) */
    uint32_t want = (snd_chnm == 1) ? space / 2 : space;
    if (want > SND_CHUNK / 2) want = SND_CHUNK / 2;  /* half of buf for the mono expansion */
    want -= want % (snd_chnm * sizeof(int16_t));
    if (want < 1024) break;
    if (snd_pos >= play_data_bytes) {
      /* the end of the file: play it again from the top */
      snd_pos = 0;
      snd_file.seek(play_data_start);
    }
    if (want > play_data_bytes - snd_pos) want = play_data_bytes - snd_pos;
    int r = snd_file.read(buf, want);
    if (r <= 0) {
      MPLog("Sound: read error\n");
      snd_close();
      return;
    }
    snd_pos += r;
    snd_filter((int16_t*)buf, r/(snd_chnm*sizeof(int16_t)), snd_chnm);
    uint32_t n = r;
    if (snd_chnm == 1) {
      /* mono: copy the sample to L and R (from the end, in place) */
      int16_t* p = (int16_t*)buf;
      for (int i = r/2 - 1; i >= 0; --i) {
        p[2*i+1] = p[i];
        p[2*i]   = p[i];
      }
      n = r * 2;
    }
    uint32_t wp = snd_wp;
    uint32_t first = SND_BRIDGE_SIZE - wp;
    if (first > n) first = n;
    memcpy(snd_bridge + wp, buf, first);
    memcpy(snd_bridge, buf + first, n - first);
    __sync_synchronize();
    snd_wp = (wp + n) % SND_BRIDGE_SIZE;
  }
  /* in case the callback could not send a frame */
  snd_top_up();
}

/* open the file again and start the renderer: false if the sound can't be played */
bool snd_start(int no, struct PlayInfo* info, int vol) {
  char name[DATA_PATH_LEN];
  if (info->rate != 48000 && info->rate != 192000) {
    MPLog("Sound: %d Hz is not supported\n", (int)info->rate);
    return false;
  }
  play_file_name(no, name);
  snd_file = theSD.open(name, FILE_READ);
  if (!snd_file) return false;
  snd_file.seek(play_data_start);
  snd_opened = true;
  snd_pos  = 0;
  snd_chnm = info->chnm;
  snd_wp = snd_rp = 0;
  snd_inflight = 0;
  snd_end_done = false;
  snd_bridge = (uint8_t*)malloc(SND_BRIDGE_SIZE);
  if (snd_bridge == NULL) {
    MPLog("Sound: no memory\n");
    snd_close();
    return false;
  }

  int ret = initMemoryPools();
  if (ret != 0) {
    MPLog("Sound: initMemoryPools error %d\n", ret);
    snd_close();
    return false;
  }
  createStaticPools(MEM_LAYOUT_PLAYER);
  snd_mixer = OutputMixer::getInstance();
  snd_mixer->activateBaseband();
  if (snd_mixer->create(snd_attention_cb) != OUTPUTMIXER_ECODE_OK) {
    MPLog("Sound: mixer create error\n");
    snd_stop();
    return false;
  }
  snd_mixer->setRenderingClkMode((info->rate > 48000) ? OUTPUTMIXER_RNDCLK_HIRESO : OUTPUTMIXER_RNDCLK_NORMAL);
  snd_mixer->activate(OutputMixer0, HPOutputDevice, snd_mixer_done_cb);
  usleep(100 * 1000);
  if (snd_mixer->setVolume(vol*10, 0, 0) != OUTPUTMIXER_ECODE_OK) {
    MPLog("Sound: setVolume(%d) error\n", vol*10);
  }
  board_external_amp_mute_control(false);

  snd_filter_init(info->rate);
  snd_fill();
  snd_running = true;
  snd_top_up();
  MPLog("Sound: start %d Hz %dch %+d dB\n", (int)info->rate, snd_chnm, vol);
  return true;
}

/* stop the renderer and free everything (also after a failure of snd_start()) */
void snd_stop() {
  if (snd_running) {
    pthread_mutex_lock(&snd_mtx);
    snd_running = false;
    bool sent = snd_send_frame(true);
    pthread_mutex_unlock(&snd_mtx);
    /* wait for the last frame */
    for (int i = 0; sent && !snd_end_done && i < 100; ++i) usleep(10 * 1000);
    if (!snd_end_done) MPLog("Sound: no end reply\n");
  }
  if (snd_mixer != NULL) {
    board_external_amp_mute_control(true);
    snd_mixer->deactivate(OutputMixer0);
    usleep(100 * 1000);
    AS_DeleteOutputMix();
    AS_DeleteRenderer();
    snd_mixer->end();
    finalizeMemoryPools();
    snd_mixer = NULL;
  }
  snd_close();
}

void snd_close() {
  if (snd_opened) snd_file.close();
  snd_opened = false;
  if (snd_bridge != NULL && snd_mixer == NULL) {
    free(snd_bridge);
    snd_bridge = NULL;
  }
}

/* the numbers of RECnnnnn.WAV in the folder: the newest max files in descending order */
static int play_scan(int* nums, int max) {
  int count = 0;
  File root = theSD.open(REC_DIR);
  if (root) {
    while (true) {
      File e = root.openNextFile();
      if (!e) break;
      bool dir = e.isDirectory();
      const char* path = e.name();
      const char* base = strrchr(path, '/');
      base = (base == NULL) ? path : base + 1;
      e.close();
      if (dir) continue;
      /* RECnnnnn.WAV */
      if (strlen(base) != 12 || strncmp(base, "REC", 3) != 0 || strcmp(base+8, ".WAV") != 0) continue;
      int no = atoi(base+3);
      if (no <= 0) continue;
      /* keep the numbers in descending order */
      int i = count;
      if (count < max) ++count;
      else if (no < nums[count-1]) continue;
      else i = count - 1;
      while (i > 0 && nums[i-1] < no) { nums[i] = nums[i-1]; --i; }
      nums[i] = no;
    }
    root.close();
  }
  return count;
}

/* an item of the file list: "REC00012 48k 1ch 1:05" */
static void play_item_text(int no, char* text) {
  struct PlayInfo info;
  if (play_probe(no, &info)) {
    uint32_t sec = info.data_bytes / (info.chnm * sizeof(int16_t)) / info.rate;
    sprintf(text, "REC%05d %dk %dch %d:%02d", no, (int)(info.rate/1000), info.chnm
          , (int)(sec/60), (int)(sec%60));
  } else {
    sprintf(text, "REC%05d ???", no);
  }
}

/* the mono files of the same rate as pfile (except pfile): the newest max files */
static int play_compare_files(int pfile, int* nums, int max) {
  struct PlayInfo first, info;
  if (!play_probe(pfile, &first) || first.chnm != 1) return 0;
  int all[PLAY_SCAN_MAX];
  int n = play_scan(all, PLAY_SCAN_MAX);
  int count = 0;
  for (int i = 0; i < n && count < max; ++i) {
    if (all[i] == pfile) continue;
    if (!play_probe(all[i], &info) || info.chnm != 1 || info.rate != first.rate) continue;
    nums[count++] = all[i];
  }
  return count;
}

/* the second file can be compared with the first one */
bool play_pair_ok(int pfile, int pcmp) {
  struct PlayInfo a, b;
  if (pcmp <= 0 || pcmp == pfile) return false;
  if (!play_probe(pfile, &a) || !play_probe(pcmp, &b)) return false;
  return a.chnm == 1 && b.chnm == 1 && a.rate == b.rate;
}

/* after the file list page of FILE: go to the page of the file to compare if there is one */
void play_route_compare(struct Response* res, DynamicJsonDocument* sys) {
  if (strcmp(res->label, "pfil") != 0 || res->next_id != 0) return;  /* NEXT only */
  if (((*sys)["src"] | SRC_MIC) != SRC_FILE) return;
  int nums[1];
  if (play_compare_files(res->value, nums, 1) == 0) return;
  res->next_id = PLAY_CMP_PAGE;
}

/* make the items of the page of the file to compare: "NONE" and the mono files of the same rate */
void play_list_compare(DynamicJsonDocument* doc, int pfile, int pcmp) {
  int nums[PLAY_LIST_MAX - 1];
  int count = play_compare_files(pfile, nums, PLAY_LIST_MAX - 1);
  int cur = 0;
  (*doc)["item0"] = "NONE";
  (*doc)["item0_v"] = 0;
  for (int i = 0; i < count; ++i) {
    char text[32];
    play_item_text(nums[i], text);
    (*doc)["item" + String(i+1)] = String(text);
    (*doc)["item" + String(i+1) + "_v"] = nums[i];
    if (nums[i] == pcmp) cur = i + 1;
  }
  (*doc)["item_num"] = count + 1;
  (*doc)["cur0"] = cur;
  MPLog("Play: %d files to compare\n", count);
}

/* make the items of the file list page: the newest PLAY_LIST_MAX files */
void play_list_files(DynamicJsonDocument* doc, int pfile) {
  int nums[PLAY_LIST_MAX];
  int count = play_scan(nums, PLAY_LIST_MAX);

  int cur = 0;
  for (int i = 0; i < count; ++i) {
    char text[32];
    play_item_text(nums[i], text);
    (*doc)["item" + String(i)] = String(text);
    (*doc)["item" + String(i) + "_v"] = nums[i];
    if (nums[i] == pfile) cur = i;
  }
  if (count == 0) {
    (*doc)["item0"] = "NO FILE";
    (*doc)["item0_v"] = 0;
    count = 1;
  }
  (*doc)["item_num"] = count;
  (*doc)["cur0"] = cur;
  MPLog("Play: %d files listed\n", count);
}
