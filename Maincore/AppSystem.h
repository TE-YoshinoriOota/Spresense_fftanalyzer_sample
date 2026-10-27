#ifndef __APP_SYSTEM_HEADER_GUARD__
#define __APP_SYSTEM_HEADER_GUARD__

#define USE_SD_CARD
#define ENABLE_SAVE_DATA
/* debug logs of the WAV recorder: heap, SD writing speed, steps of stopping audio */
// #define REC_DEBUG

#ifdef REC_DEBUG
#define REC_DLOG(...) MPLog(__VA_ARGS__)
#else
#define REC_DLOG(...)
#endif

#include <ArduinoJson.h>
#include <MP.h>
#include <Audio.h>
/* include them before SUBCORE is defined (they check it) */
#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <arch/board/board.h>

#include <SDHCI.h>
#include <Flash.h>
SDClass theSD;

/* Use CMSIS library */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>
#include <RingBuff.h>
#include "FFTClassKai.h"
#include "IIRKai.h"

#define LED0_MASK 0x01
#define LED1_MASK 0x02
#define LED2_MASK 0x04
#define LED3_MASK 0x08

#define FILE_ERROR  (0x01)   /* LED0 Blink */
#define JSON_ERROR  (0x02)   /* LED1 Blink */
#define MP_ERROR    (0x04)   /* LED2 Blink */
#define MEM_ERROR   (0x08)   /* LED3 Blink */

/* sid definitions */
#define SID_REQ_ESDCARD  (0x01)
#define SID_REQ_JSONDOC  (0x02)
#define SID_REQ_MONDATA  (0x03)
#define SID_REQ_WAV_WAV  (0x04)
#define SID_REQ_RAW_FIL  (0x05)
#define SID_REQ_RECSTRT  (0x06)
#define SID_REQ_RECSTOP  (0x07)
#define SID_REQ_WAV_FFT  (0x10)
#define SID_REQ_SCRCAP   (0x11)  /* 0x09-0x0F can't be used: 0x08 is the save flag */
#define SID_REQ_SAVED    (0x12)  /* Maincore -> Subcore1: the data is saved (struct SaveStatus) */
#define SID_REQ_FFT_FFT  (0x20)
#define SID_REQ_SPECTRO  (0x40)

/* limits for the high sampling rate (more than 48kHz) */
#define HIRATE_MAX_CHNM  (2)   /* capture 2 channels at most */
#define HIRATE_MAX_RECH  (1)   /* the SD card is too slow to record 2 channels */

/* channel select pages of the recorder (AA710.txt, AA711.txt) */
#define REC_CH_PAGE      (710)
#define REC_DCH_PAGE     (711)

/* page type of REC MONITOR (same as SCR_TYPE_RECM in AppScreen.h) */
#define PAGE_TYPE_RECM   (16)
/* page types which show CH2 (same as SCR_TYPE_xxx in AppScreen.h) */
#define PAGE_TYPE_FFT2   (6)
#define PAGE_TYPE_LSJU   (17)
#define PAGE_TYPE_WVWV   (9)
#define PAGE_TYPE_FTLG   (11)
#define PAGE_TYPE_FFDB   (14)

/* input source (src of sys.txt) */
#define SRC_MIC          (0)
#define SRC_FILE         (1)
#define SRC_FILE_SOUND   (2)   /* the file, and play it on the headphone jack */
#define SRC_FILE_ERROR   (3)   /* sent to Subcore only: the file can't be played */
#define PLAY_VOL_INIT    (-30) /* volume of the playback in dB (pvol of sys.txt) */
#define PLAY_VOL_MIN     (-102) /* range of OutputMixer::setVolume() */
#define PLAY_VOL_MAX     (12)
#define SND_CHUNK        (8192) /* bytes written to the player at a time */
/* the file list page of the player (AA810.txt) */
#define PLAY_FILE_PAGE   (810)
#define PLAY_LIST_MAX    (20)  /* the menu scrolls on Subcore1 */
#define PLAY_SCAN_MAX    (64)  /* the newest files probed for the file to compare */
/* the page to select the second mono file to compare (FILE only, AA815.txt) */
#define PLAY_CMP_PAGE    (815)
#define PLAY_ERROR_MS    (3000) /* "SD READ ERROR" again after it disappears (SAVE_TEXT_MS of Subcore1) */
/* FFT lines: 3200 and 6400 lines (8192 and 16384 samples) are for the WAV file without the sound */
#define LINE_MAX         (1600)
#define LINE_MAX_FILE    (6400)
/* cap the FFT length on REC MONITOR to keep the heap for the recording fifo */
#define REC_MAX_SAMP     (1024)

/* save flag */
#define SID_MSK_SAVEDAT   (0x08)
#define SID_UMSK_SAVEDAT  (0xF7)

/* FFT WINDOW FUNCTION */
#define FFT_WINDOW_RECTANGULAR  (0x00)
#define FFT_WINDOW_HAMMING      (0x01)
#define FFT_WINDOW_HANNING      (0x02)
#define FFT_WINDOW_FLATTOP      (0x03)


/* difinitions for application */
#define SUBCORE (1)
#define JSON_BUFFER    (4096)  /* the file list page of 20 files needs more than 2048 */

#define MAX_FIFO_BUFF  (4096)
#define MAX_CHANNEL    (4)

/* the fifo of the recorder in the audio library: the time of the input it keeps */
#define AUDIO_FIFO_MS          (200)
#define AUDIO_FIFO_MIN_FRAMES  (3)

/* folders of the data files on the SD card */
#define REC_DIR          "RECDATA"   /* the recordings RECnnnnn.WAV */
#define CSV_DIR          "CSVDATA"   /* the saved data *.csv */
#define CAP_DIR          "CAPIMAGE"  /* the screen captures SCRnnnnn.BMP */
#define DATA_PATH_LEN    (32)        /* "CAPIMAGE/SCRnnnnn.BMP" and so on */

/* maximum capturing time is 1/16000 x 4096 */
#define MAX_USTIME_FOR_CAPTURING (256000)

struct Response {
  char label[5];
  int  value;
  int  cur0;
  int  cur1;
  int  next_id;
  int  has_amp;  /* 1: the page is a graph, write amp to the page file */
  int  amp;      /* the magnification of the graph (same layout in AppScreen.h) */
};

struct SensorData {
  float acc;
  float vel;
  float dis;
};

struct FftWavData {
  float* pWav;
  float* pFft;
  int len;
  float df;
};

struct FftFftData {
  float* pFft;
  float* pSubFft;
  int len;
  float df;
};

struct WavWavData {
  float* pWav;
  float* pSubWav;
  int len;
  float df; 
};

/* recording status shared with Subcore (same layout in AppScreen.h) */
#define REC_STATE_IDLE      (0)
#define REC_STATE_RECORDING (1)
#define REC_STATE_STOPPED   (2)
#define REC_STATE_ERROR     (3)

/* screen capture: Subcore1 sends the LCD in strips, Maincore writes them to a BMP file */
/* (same layout in AppSystem.h and AppScreen.h) */
#define CAP_STATE_OK    (0)   /* the strip is written */
#define CAP_STATE_DONE  (1)   /* the last strip is written and the file is closed */
#define CAP_STATE_ERROR (2)

struct CapStrip {
  int16_t  y;        /* the top line of the strip */
  int16_t  rows;
  int16_t  width;
  int16_t  height;   /* of the screen */
  uint32_t data;     /* physical address of the RGB565 pixels (top line first) */
};

/* the result of saving the data (CSV): shown on the graph screen */
/* (same layout in AppSystem.h and AppScreen.h) */
#define SAVE_STATE_OK    (0)
#define SAVE_STATE_ERROR (1)
#define SAVE_STATE_MESSAGE (2)  /* the message in filename, in red (e.g. "SD READ ERROR") */

struct SaveStatus {
  int  state;
  int  rows;          /* the rows saved on the screen */
  char filename[16];  /* the first CSV file (without the folder) */
};

struct CapStatus {
  int  state;
  char filename[16];
  char message[16];
};

struct PlayInfo {
  int      chnm;
  uint32_t rate;
  uint32_t data_start;
  uint32_t data_bytes;
};

struct RecStatus {
  int      state;
  uint32_t rate;
  uint32_t frames;   /* frames written to the fifo */
  uint32_t dropped;  /* frames dropped because the fifo was full */
  char     filename[16];
  char     message[16];
};


/*　json documents */
String dfile = "AA000.txt";
String sfile = "sys.txt";
DynamicJsonDocument *djson = new DynamicJsonDocument(JSON_BUFFER);
DynamicJsonDocument *sjson = new DynamicJsonDocument(JSON_BUFFER);

/* For Menu Framework */
struct Response* res = NULL;
struct SensorData sData;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/* For Singal Processing */
AudioClass *theAudio = AudioClass::getInstance();
static char*  pRaw = NULL;
static float* pWav = NULL;
static float* pSubWav = NULL;
static bool bProcessing = false;
static bool bThreadStopped = true;
static bool bSignalProcessingStarted = false;
static volatile bool bAudioError = false;  /* set by the attention callback of the audio library */

/* For Filtering process */
static IIRClassKai* lpf = NULL;
static IIRClassKai* hpf = NULL;
static IIRClassKai* lpf2 = NULL;  /* CH2 keeps its own state of the filters */
static IIRClassKai* hpf2 = NULL;

/* For FFT processing */
RingBuff** ringbuff = NULL;
static float* pTmp = NULL;
static float* pFft = NULL;
static float* pSubFft = NULL;
static struct FftWavData fftWavData;
static struct FftFftData fft2Data;
static FFTClassKai *fft = NULL;
static int fft_buf_len = 0;  /* the length of pFft and pSubFft */

/* For WAV processing */
static struct WavWavData wav2Data;

/* For WAV recording */
static struct RecStatus recStatus;

/* For screen capture */
static struct CapStatus capStatus;

/* System properties */
static int  g_sid;
static int  g_sens;
static int  g_gain;
static int  g_chnm;
static int  g_ch0;
static int  g_ch1;
static int  g_rate;
static int  g_samp;
static int  g_lpf;
static int  g_hpf;
static int  g_win;
static int  g_rech;  /* number of channels to record */
static int  g_src;   /* input source: SRC_MIC or SRC_FILE */
static int  g_pfile; /* number of the file to play (RECnnnnn.WAV) */
static int  g_pcmp;  /* number of the mono file to compare as CH2 (FILE only, 0: none) */
static bool g_booted = false;  /* setup() is done: restart at an error of the SD card */
static int  g_nring; /* number of the ring buffers (2 for a mono file, CH2 is kept zero) */
static bool g_play_ok; /* the file is opened for SRC_FILE */
static bool g_sound;   /* the file is played on the headphone jack */
static int  g_vol;     /* volume of the playback in dB (-102 to +12) */

static uint16_t buffer_size;

/* JsonProcessor functions */
void update_system_properties(int8_t sid, DynamicJsonDocument* sys);
void readSysprop(int8_t sid, DynamicJsonDocument* doc);
void cfg_migrate();
void data_dir(const char* dir);
void data_migrate();
void updateSysprop(int8_t sid, DynamicJsonDocument* doc, struct Response* res);
DynamicJsonDocument* updateJson(DynamicJsonDocument* doc, struct Response* res);
void patchPage(DynamicJsonDocument* doc, DynamicJsonDocument* sys);
void patchSource(DynamicJsonDocument* doc, DynamicJsonDocument* sys);

/* SignalProcessing functions */
void error_notifier(int n);
void init_processing(int8_t sid, DynamicJsonDocument* sys);
void finish_processing();
void pause_processing();
void resume_processing();

void calc_sensor_data(struct SensorData* sdata);
void calc_fft_data(struct FftWavData* fdata);
void calc_fft2_data(struct FftFftData* fdata);
void get_wav2_data(struct WavWavData* wdata);

/* WavRecorder functions */
void rec_start(struct RecStatus* st);
void rec_stop(struct RecStatus* st);
void rec_push(const int16_t* raw, int frames);
void rec_release();
bool rec_is_active();
void rec_log_heap(const char* when);

/* WavPlayer functions */
bool play_probe(int no, struct PlayInfo* info);
bool play_open(int no, struct PlayInfo* info);
void play_close();
void play_restart_clock();
int  play_read(char* buf, int max_frames);
void play_list_files(DynamicJsonDocument* doc, int pfile);
void play_list_compare(DynamicJsonDocument* doc, int pfile, int pcmp);
bool play_pair_ok(int pfile, int pcmp);
bool play_open_compare(int no, const struct PlayInfo* first);
void play_notify_error();
void play_route_compare(struct Response* res, DynamicJsonDocument* sys);
bool snd_start(int no, struct PlayInfo* info, int vol);
void snd_fill();
void snd_stop();
void snd_close();

/* ScreenCapture functions */
void cap_strip(struct CapStrip* strip, struct CapStatus* st);

/* StoringProccedData */
#ifdef ENABLE_SAVE_DATA
void openStorage(uint8_t sid);
void closeStorage();
void saveData(struct SensorData* data);
void saveData(struct FftWavData* data);
void saveData(struct FftFftData* data);
void saveData(struct WavWavData* data);
void sendSaveStatus();
#endif

#endif /* __APP_SYSTEM_HEADER_GUARD__ */
