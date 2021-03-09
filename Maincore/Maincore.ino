#include <ArduinoJson.h>
#include <SDHCI.h>
#include <MP.h>
#include <Audio.h>

/* Use CMSIS library */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>
#include <RingBuff.h>

#define SID_SIG_MASK 0x10
#define SID_END_MASK 0x20
#define SID_ERR_MASK 0x40

#define SID_DOC_MASK 0x01
#define SID_DAT_MASK 0x02
#define SID_WAV_MASK 0x04
#define SID_FFT_MASK 0x08

// #define COPY_BUFFER

#define SUBCORE (1)
#define JSON_BUFFER    (768)

#define MAX_FIFO_BUFF  (4096)
#define MAX_CHANNEL    (4)

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

struct FFTData {
  float* pWav;
  float* pFft;
  int len;
  float df;
};

SDClass theSD;

/*　json documents */
String dfile = "AA000.txt";
String sfile = "sys.txt";
DynamicJsonDocument* djson = new DynamicJsonDocument(JSON_BUFFER);
DynamicJsonDocument* sjson = new DynamicJsonDocument(JSON_BUFFER);

/* For Menu Framework */
struct Response *res = NULL;
struct SensorData sData;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/* For Singal Processing */
AudioClass *theAudio = AudioClass::getInstance();
static char *pRaw;
static float* pWav = NULL;
static bool bProcessing = false;
static bool bThreadStopped = true;
static bool bSignalProcessingStarted    = false;

/* For FFT processing */
RingBuff *ringbuff = NULL;
static float* pTmp = NULL;
static float* pFft = NULL;
static float* pSubWav = NULL;
static float* pSubFft = NULL;
static FFTData fftData;

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

static double unit;
static uint16_t buffer_size;


void setup() {
  Serial.begin(115200);
  MP.begin(SUBCORE);
  
  djson = updateJson(djson, NULL);
  int8_t sid = SID_DOC_MASK;
  MP.Send(sid, djson, SUBCORE);
  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {

  int8_t sid = 0;
  if (MP.Recv(&sid, &res, SUBCORE) < 0) {
    usleep(1);
    return;
  }
  printf("[Main] 0x%02x\n", sid);

  if (sid & SID_DOC_MASK) {
  /* request to change page from Subcore */

    /* stop the signal processing */
    bProcessing = false;
    Serial.println("Stop Thread");
    /* wait for stopping the thread */
    while (!bThreadStopped) { Serial.println("."); usleep(1); } 
    if (bSignalProcessingStarted) {
      finish_processing();
      bSignalProcessingStarted = false;
    }
    
    /* update system properties */
    updateSysprop(sid, sjson, res);

    /* get the new page resources and update the json document */
    djson = updateJson(djson, res);
    int ch = (*sjson)["ch"];
    int ch0 = ch & 0x00ff;
    int ch1 = ch >> 8;
    (*djson)["ch0"] = ch0;
    (*djson)["ch1"] = ch1;

    /* send the json document to change the page on LCD */
    MP.Send(sid, djson, SUBCORE);

  } else if (sid > SID_DOC_MASK) {
    /* request to launch sensor applications */
    if (bProcessing == false) {
      Serial.println("init_processing");
      readSysprop(sid, sjson);     
      bProcessing = true;
      init_processing(sid, sjson);
      bSignalProcessingStarted = true;
    }

    if (sid & SID_DAT_MASK) {
    /* request data to monitor sensors */
      
      calc_sensor_data(&sData);

      struct SensorData sdata;
      memcpy(&sdata, &sData, sizeof(sData));

      /* send the data to update the number on LCD display */
      sid = SID_DAT_MASK;
      MP.Send(sid, &sdata, SUBCORE);
      
    } else if (sid & SID_FFT_MASK) {
    /* request calculate fft data */

      Serial.println(">");
      calc_fft_data(&fftData);

#ifdef COPY_BUFFER
      struct FFTData fdata;
      memcpy(&fdata, &fftData, sizeof(struct FFTData));
      memcpy(pSubWav, fftData.pWav, sizeof(float)*g_samp);
      memcpy(pSubFft, fftData.pFft, sizeof(float)*g_samp/2);
      fdata.pWav = pSubWav;
      fdata.pFft = pSubFft;
#endif

      Serial.println("Send fft data to SUBCORE");

      /* send the wan and fft data to update the graph on LCD display */     
      sid = SID_FFT_MASK;
      MP.Send(sid, &fftData, SUBCORE);

    }
  }
  usleep(1);
}
