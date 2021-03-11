#include <ArduinoJson.h>
#include "FFTClassKai.h"

#ifndef USE_ARDUINO_FFT
FFTClassKai* fft = NULL;
#endif

static uint32_t last_time;

static void init_processing(int8_t sid, DynamicJsonDocument* sys) {
  int err, ret;
  int ch, line;

  /* Initialize Parameters */
  memset(&sData, 0, sizeof(struct SensorData));
  if (g_chnm == 3) g_chnm = 4;
 
  /* Audio Initialization */
  buffer_size = MAX_FIFO_BUFF * g_chnm * sizeof(int16_t);

  /* Audio Memory Initialization */
  if (pRaw != NULL) free((char*)pRaw);
  if (pWav != NULL) free((float*)pWav);
  if (pSubWav != NULL) free((float*)pSubWav);
  if (ringbuff != NULL) delete[] ringbuff;
  pRaw = (char*)malloc(sizeof(char)*buffer_size);
  pWav = (float*)malloc(sizeof(float)*g_samp);
  pSubWav = (float*)malloc(sizeof(float)*g_samp);
  int ringbuff_size = MAX_FIFO_BUFF;
  ringbuff = new RingBuff[g_chnm](ringbuff_size); // keep double space
  memset(pRaw, 0, sizeof(char)*buffer_size);
  memset(pWav, 0, sizeof(float)*g_samp);
  memset(pSubWav, 0, sizeof(float)*g_samp);

  /* Setup Audio Libraries */
  Serial.println("theAudio->begin()");
  theAudio->begin();
  if (g_rate > 48000) {
    theAudio->setRenderingClockMode(AS_CLKMODE_HIRES);
  } else {
    theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
  }
  
  Serial.println("theAudio->setRecorderMode()");
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);
  err = theAudio->initRecorder(AS_CODECTYPE_PCM ,"/mnt/sd0/BIN" ,g_rate ,g_chnm);                             
  if (err != AUDIOLIB_ECODE_OK) {
    Serial.println("Recorder initialize error");
    return;
  }

  /* FFT initialization */
  if (g_sid & SID_FFT_MASK) {
    Serial.println("FFT initialization");
    fft = new FFTClassKai(g_samp);

    /* check if the memory is not free */
    if (pTmp != NULL) free((float*)pTmp);
    if (pFft != NULL) free((float*)pFft);
    if (pSubFft != NULL) free((float*)pSubFft);

    /* allocate memories from MainCore memory */
    pTmp = (float*)malloc(sizeof(float)*g_samp);
    pFft = (float*)malloc(sizeof(float)*g_samp);
    pSubFft = (float*)malloc(sizeof(float)*g_samp);
    if (pTmp == NULL || pFft == NULL || pSubFft == NULL) {
      Serial.println("not enough memory");
      exit(1);
    }

    memset(pTmp, 0, sizeof(float)*g_samp);
    memset(pFft, 0, sizeof(float)*g_samp);
    memset(pSubFft, 0, sizeof(float)*g_samp);
   
    fft->begin(WindowHamming, 0.5); // WindowHamming applied so far.
  }
  
  /* Start recordubg routine */
  Serial.println("Start Recording");
  theAudio->startRecorder(); 
  task_create("signal_processing", 120, 1024, signal_processing, NULL);
  usleep(1);
}


static void signal_processing(int argc, char* argv[]) {
  
  int err, ret;
  int read_size;
  last_time = millis();
  static uint32_t buffered_sample = 0;    
  
  Serial.println("Start Processsig...");
  
  /* Audio Thread starts from here */
  bThreadStopped = false;
  while(bProcessing) { 

    err = theAudio->readFrames(pRaw, buffer_size, &read_size);
    if (err != AUDIOLIB_ECODE_OK && err != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
      Serial.println("Error err = " + String(err));
      bProcessing = false;
      break;
    }
    
    /* check if the read data is zero, just rest and continue */ 
    if (read_size == 0) {
      usleep(1);
      continue;
    }

    // printf("read_size: %d\n", read_size);
     
    /* push the captured data to the ringbuffers */
    int captured_sample = read_size/(g_chnm*sizeof(int16_t));
    pthread_mutex_lock(&m);
    for (int i = 0; i < g_chnm; i++) 
      ringbuff[i].put((q15_t*)pRaw, captured_sample, g_chnm, i);
    pthread_mutex_unlock(&m);  
    usleep(1);
  }
  bThreadStopped = true;  
}

void finish_processing() {
  
  Serial.println("Stop Recording");
  theAudio->stopRecorder();
  theAudio->setReadyMode();
  theAudio->end();  
  usleep(1);

  /* free the memories for Audio Library */ 
  if (pRaw != NULL) free((char*)pRaw);
  if (pWav != NULL) free((float*)pWav);  
  if (pSubWav != NULL) free((float*)pSubWav);  
  if (ringbuff != NULL) delete[] ringbuff;
  pRaw = NULL;
  pWav = NULL;
  pSubWav = NULL;
  ringbuff = NULL;
  
  if (g_sid & SID_FFT_MASK) {

    if (pTmp != NULL) free((float*)pTmp);
    if (pFft != NULL) free((float*)pFft);
    if (pSubFft != NULL) free((float*)pSubFft);
    delete fft;
      
    pTmp = NULL;
    fft = NULL;
    pFft = NULL;
    pSubFft = NULL;
  }
}

static void calc_sensor_data(struct SensorData* sdata) {

  /* get the data from the ringbuffer */
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);   
  pthread_mutex_unlock(&m);  
  
  uint32_t cur_time = millis();
  uint32_t dt = cur_time - last_time;
  calcSensorData(&sData, pWav, g_samp*g_chnm, g_chnm, g_ch0, dt);
  last_time = cur_time;
}

static void calc_fft_data(struct FftWavData* fdata) {
  
  /* Copy data for FFT calculation */
  //uint32_t mTime = millis();
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);       
  pthread_mutex_unlock(&m); 
  
  memcpy(pTmp, pWav, sizeof(float)*g_samp);
  //Serial.print("cmsis fft: ");
  fft->clear();
  fft->fft_amp(pTmp, pFft);
  //mTime = millis() - mTime;
  //Serial.println(mTime);
  
  fdata->pWav = pWav;
  fdata->pFft = pFft;
  fdata->len  = g_samp;
  fdata->df   = (float)(g_rate)/g_samp;
}

static void calc_fft2_data(struct FftFftData* fdata) {
  
  /* Copy data for FFT calculation */
  uint32_t mTime = millis();
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);       
  ringbuff[(g_ch1-1)].get(pSubWav, g_samp);       
  pthread_mutex_unlock(&m); 

  /* ch0 fft transform */
  memcpy(pTmp, pWav, sizeof(float)*g_samp);
  //Serial.print("cmsis fft0: ");
  fft->clear();
  fft->fft_amp(pTmp, pFft);

  /* ch1 fft transform */
  memcpy(pTmp, pSubWav, sizeof(float)*g_samp);
  //Serial.print("cmsis fft1: ");
  fft->clear();
  fft->fft_amp(pTmp, pSubFft);
  //mTime = millis() - mTime;
  //Serial.println(mTime);

  fdata->pFft = pFft;
  fdata->pSubFft = pSubFft;
  fdata->len  = g_samp;
  fdata->df   = (float)(g_rate)/g_samp;
 
}


static void calcSensorData(struct SensorData *sdata, float *buf, int bufsize, int chnum, int ch, int32_t dt) {
  --ch;
  float acc = 0.0;
  float vel = 0.0;
  float dis = 0.0;
  for (int i = ch; i < bufsize; i += chnum) {
    acc += buf[i];
  }
  acc /= bufsize;
  vel += acc*dt/1000.;
  dis += vel*dt/1000.;

  sdata->acc = acc;
  sdata->vel = vel;
  sdata->dis = dis;

  return;
}
