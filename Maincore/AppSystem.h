#ifndef __APP_SYSTEM_HEADER_GUARD__
#define __APP_SYSTEM_HEADER_GUARD__

#define USE_SD_CARD
#define ENABLE_SAVE_DATA

#include <ArduinoJson.h>
#include <MP.h>
#include <Audio.h>

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
#define MEM_ERROR   (0x03)   /* LED0 & LED2 Blink */

/* sid definitions */
#define SID_REQ_ESDCARD  (0x01)
#define SID_REQ_JSONDOC  (0x02)
#define SID_REQ_MONDATA  (0x03)
#define SID_REQ_WAV_WAV  (0x04)
#define SID_REQ_RAW_FIL  (0x05)
#define SID_REQ_WAV_FFT  (0x10)
#define SID_REQ_FFT_FFT  (0x20)
#define SID_REQ_ORBITDT  (0x30)
#define SID_REQ_SPECTRO  (0x40)

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
#define JSON_BUFFER    (1024)

#define MAX_FIFO_BUFF  (4096)
#define MAX_CHANNEL    (4)

/* maximum capturing time is 1/16000 x 4096 */
#define MAX_USTIME_FOR_CAPTURING (256000)

struct Response {
  char label[5];
  int  value;
  int  cur0;
  int  cur1;
  int  next_id;
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

struct OrbitData {
  float acc0;
  float vel0;
  float dis0;
  float acc1;
  float vel1;
  float dis1;
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

/* For Filtering process */
static IIRClassKai* lpf = NULL;
static IIRClassKai* hpf = NULL;

/* For FFT processing */
RingBuff* ringbuff = NULL;
static float* pTmp = NULL;
static float* pFft = NULL;
static float* pSubFft = NULL;
static struct FftWavData fftWavData;
static struct FftFftData fft2Data;
static FFTClassKai *fft = NULL;

/* For WAV processing */
static struct WavWavData wav2Data;

/* For Orbit processing */
static struct OrbitData orbitData;

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

static uint16_t buffer_size;

/* JsonProcessor functions */
void update_system_properties(int8_t sid, DynamicJsonDocument* sys);
void readSysprop(int8_t sid, DynamicJsonDocument* doc);
void updateSysprop(int8_t sid, DynamicJsonDocument* doc, struct Response* res);
DynamicJsonDocument* updateJson(DynamicJsonDocument* doc, struct Response* res);

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
void calc_orbit_data(struct OrbitData* odata);

/* StoringProccedData */
#ifdef ENABLE_SAVE_DATA
void openStorage(uint8_t sid);
void closeStorage();
void saveData(struct SensorData* data);
void saveData(struct FftWavData* data);
void saveData(struct FftFftData* data);
void saveData(struct WavWavData* data);
void saveData(struct OrbitData* data);
#endif

#endif /* __APP_SYSTEM_HEADER_GUARD__ */
