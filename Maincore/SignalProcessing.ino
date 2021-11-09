#include "AppSystem.h"

#define SIG_PROCESSING_PTHREAD

#ifdef SIG_PROCESSING_PTHREAD
#include <pthread.h>
pthread_t sig_thread;
#endif

static uint32_t last_time; // To measure the processing time
void init_processing(int8_t sid, DynamicJsonDocument* sys) {
  int err, ret;
  int ch, line;

  /* Initialize Parameters */
  memset(&sData, 0, sizeof(struct SensorData));
  if (g_chnm == 3) g_chnm = 4;
 
  /* Audio Initialization */
  buffer_size = MAX_FIFO_BUFF * g_chnm * sizeof(int16_t);

  /* Audio Memory Initialization */
  int ringbuff_size = MAX_FIFO_BUFF;
  if (pRaw != NULL) free((char*)pRaw);
  if (pWav != NULL) free((float*)pWav);
  if (pSubWav != NULL) free((float*)pSubWav);
  if (pTmp != NULL) free((float*)pTmp);
  if (ringbuff != NULL) delete[] ringbuff;
  pRaw = (char*)malloc(sizeof(char)*buffer_size);
  pWav = (float*)malloc(sizeof(float)*g_samp);
  pSubWav = (float*)malloc(sizeof(float)*g_samp);
  pTmp = (float*)malloc(sizeof(float)*g_samp);
  ringbuff = new RingBuff*[g_chnm]; // keep double space
  for (int n = 0; n < g_chnm; ++n) {
    ringbuff[n] = new RingBuff(ringbuff_size);
  }
  if (pRaw == NULL || pWav == NULL || pSubWav == NULL || pTmp == NULL || ringbuff == NULL) {
    MPLog("not enough memory\n");
    while (true) {
      error_notifier(MEM_ERROR);
    }
  }
  /* clean up the memory */
  memset(pRaw, 0, sizeof(char)*buffer_size);
  memset(pWav, 0, sizeof(float)*g_samp);
  memset(pSubWav, 0, sizeof(float)*g_samp);
  memset(pTmp, 0, sizeof(float)*g_samp);

  /* Filter Initialization */
  MPLog("g_lpf: %d\n", g_lpf);
  MPLog("g_hpf: %d\n", g_hpf);
  if (g_lpf != 0) {
    lpf = new IIRClassKai();
    lpf->begin(TYPE_LPF, g_lpf, g_rate, 0.5, g_samp, Planar);
  }
  if (g_hpf != 0) { 
    hpf = new IIRClassKai();
    hpf->begin(TYPE_HPF, g_hpf, g_rate, 0.5, g_samp, Planar);
  }

  /* Setup Audio Libraries */
  MPLog("theAudio->begin()\n");
  theAudio->begin();
  if (g_rate > 48000) {
    theAudio->setRenderingClockMode(AS_CLKMODE_HIRES);
  } else {
    theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
  }
  
  MPLog("theAudio->setRecorderMode()\n");
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);
#ifdef USE_SD_CARD
  err = theAudio->initRecorder(AS_CODECTYPE_PCM ,"/mnt/sd0/BIN" ,g_rate ,g_chnm);
#else
  err = theAudio->initRecorder(AS_CODECTYPE_PCM ,"/mnt/spif/BIN" ,g_rate ,g_chnm);
#endif 
  if (err != AUDIOLIB_ECODE_OK) {
    MPLog("Recorder initialize error\n");
    return;
  }

  /* FFT initialization */
  /* 0x10-0x70 is reserved for FFT applications */
  if (g_sid & 0x70) {
    MPLog("FFT initialization\n");
    fft = new FFTClassKai(g_samp);

    /* check if the memory is not free */
    if (pFft != NULL) free((float*)pFft);
    if (pSubFft != NULL) free((float*)pSubFft);

    /* allocate memories from MainCore memory */
    pFft = (float*)malloc(sizeof(float)*g_samp);
    pSubFft = (float*)malloc(sizeof(float)*g_samp);
    if (pFft == NULL || pSubFft == NULL) {
      MPLog("not enough memory\n");
      while (true) {
        error_notifier(MEM_ERROR);
      }
    }

    memset(pFft, 0, sizeof(float)*g_samp);
    memset(pSubFft, 0, sizeof(float)*g_samp);
    switch(g_win) {
    case FFT_WINDOW_RECTANGULAR:
      fft->begin(WindowRectangle, 0.5); // WindowHamming applied so far.
      break;
    case FFT_WINDOW_HAMMING:
      fft->begin(WindowHamming, 0.5);
      break;
    case FFT_WINDOW_HANNING:
      fft->begin(WindowHanning, 0.5);
      break;
    case FFT_WINDOW_FLATTOP:
      fft->begin(WindowFlattop, 0.5);
      break;
    }
  }
  
  /* Start recordubg routine */
  MPLog("Start Recording\n");
  theAudio->startRecorder(); 
#ifndef SIG_PROCESSING_PTHREAD
  ret = task_create("signal_processing", 120, 1024, signal_processing, NULL);
#else
  struct sched_param param;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority = 10;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  ret = pthread_create(&sig_thread, &attr, signal_processing, NULL);
#endif
  if (ret < 0) {
    MPLog("task_create failure. not enough memory?\n");
    error_notifier(MEM_ERROR);
  }
  usleep(1);
}

#ifndef SIG_PROCESSING_PTHREAD
static void signal_processing(int argc, char* argv[]) {
#else
static void signal_processing(void* arg) {
#endif
  
  int err, ret;
  int read_size;
  last_time = millis();
  static uint32_t buffered_sample = 0;    
  
  MPLog("Start Processsig...\n");
  
  /* Audio Thread starts from here */
  bThreadStopped = false;
  while(bProcessing) { 

    err = theAudio->readFrames(pRaw, buffer_size, &read_size);
    if (err != AUDIOLIB_ECODE_OK && err != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
      MPLog("Error err = %d\n", err);
      bProcessing = false;
      break;
    }
    
    /* check if the read data is zero, just rest and continue */ 
    if (read_size == 0) {
      //attension!: if you enable usleep, 192kHz 4096 will not work!!
      //usleep(1); // yield the process to the main loop 
      continue;
    }

    // MPLog("read_size: %d\n", read_size);
     
    /* push the captured data to the ringbuffers */
    int captured_sample = read_size/(g_chnm*sizeof(int16_t));
    pthread_mutex_lock(&m);
    for (int i = 0; i < g_chnm; i++) {
      ringbuff[i]->put((q15_t*)pRaw, captured_sample, g_chnm, i);
    }
    pthread_mutex_unlock(&m);  
    //attension!: if you enable usleep, 192kHz 4096 will not work!!
    //usleep(1); // yield the process to main loop
  }
  bThreadStopped = true;  
}


void finish_processing() {
  
  MPLog("Stop Recording\n");
  theAudio->stopRecorder();
  theAudio->setReadyMode();
  theAudio->end();  
  usleep(1);

  /* free the memory for Audio Library */ 
  if (pRaw != NULL) free((char*)pRaw);
  if (pWav != NULL) free((float*)pWav);  
  if (pSubWav != NULL) free((float*)pSubWav);  
  if (ringbuff != NULL) {
    for (int n = 0; n < g_chnm; ++n) {
      delete ringbuff[n];
    }
    delete[] ringbuff;
  }
  if (pTmp != NULL) free((float*)pTmp);
  pRaw = NULL;
  pWav = NULL;
  pSubWav = NULL;
  pTmp = NULL;
  ringbuff = NULL;

  /* free the memory for Filtering */ 
  if (lpf != NULL) delete lpf;
  if (hpf != NULL) delete hpf;
  lpf = NULL;
  hpf = NULL;

  /* sid of 0x10-0x70 is reserved for FFT applications */
  if (g_sid & 0x70) {
    if (pFft != NULL) free((float*)pFft);
    if (pSubFft != NULL) free((float*)pSubFft);
    delete fft;     
    pFft = NULL;
    pSubFft = NULL;
    fft = NULL;
  }
}

void pause_processing() {
  if (bProcessing == false) {
    MPLog("Signal Processing already stopped\n");
    return;
  }
  
  MPLog("Pause Signal Processing\n");
  bProcessing = false;
  while (!bThreadStopped) {
    usleep(1);
  }
  theAudio->stopRecorder(); // stop recorder to avoid FIFO overflow
}

void resume_processing() {
  int ret;
  if (bProcessing == true) {
    MPLog("Signal Processing already running\n");
    return;
  }
  
  MPLog("Resume Signal Processing\n");
  bProcessing = true;
  theAudio->startRecorder(); 
#ifndef SIG_PROCESSING_PTHREAD
  ret = task_create("signal_processing", 120, 1024, signal_processing, NULL);
#else
  struct sched_param param;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority = 10;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  ret = pthread_create(&sig_thread, &attr, signal_processing, NULL);
#endif
  if (ret < 0) {
    MPLog("task_create failure. not enough memory?\n");
    error_notifier(MEM_ERROR);
  }
  usleep(1);
}



void calc_sensor_data(struct SensorData* sdata) {

  /* get the data from the ringbuffer */
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)]->get(pWav, g_samp);   
  pthread_mutex_unlock(&m);  
  
  uint32_t cur_time = millis();
  uint32_t dt = cur_time - last_time;
  calcSensorData(&sData, pWav, g_samp, dt);
  last_time = cur_time;
}

void calc_fft_data(struct FftWavData* fdata) {
  
  /* Copy data for FFT calculation */
  //uint32_t mTime = millis();
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)]->get(pWav, g_samp);       
  pthread_mutex_unlock(&m); 


  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);
  }
  if (hpf != NULL) {
    hpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);    
  }
  
  /* ch0 fft transform */
  memcpy(pTmp, pWav, sizeof(float)*g_samp); 
  fft->clear();
  //fft->fft_amp(pFft, pTmp);
  fft->fft_scaled_amp(pFft, pTmp);
  //mTime = millis() - mTime;
  //MPLog("time: %d\n", mTime);
   
  fdata->pWav = pWav;
  fdata->pFft = pFft;
  fdata->len  = g_samp;
  fdata->df   = (float)(g_rate)/g_samp;
}

void calc_fft2_data(struct FftFftData* fdata) {
  
  /* Copy data for FFT calculation */
  uint32_t mTime = millis();
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)]->get(pWav, g_samp);
  if (g_chnm > 1) {   /* fool proof */
    ringbuff[(g_ch1-1)]->get(pSubWav, g_samp);
  } else {
    memset(pSubWav, 0, g_samp*sizeof(float));
  }
  pthread_mutex_unlock(&m); 

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);
  }
  if (hpf != NULL) {
    hpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);    
  }
  
  /* ch0 fft transform */
  memcpy(pTmp, pWav, sizeof(float)*g_samp);
  fft->clear();
  // fft->fft_amp(pFft, pTmp);
  fft->fft_scaled_amp(pFft, pTmp);

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  }
  if (hpf != NULL) {
    hpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);    
  }

  /* ch1 fft transform */
  memcpy(pTmp, pSubWav, sizeof(float)*g_samp);
  fft->clear();
  // fft->fft_amp(pSubFft, pTmp);
  fft->fft_scaled_amp(pSubFft, pTmp);
  //mTime = millis() - mTime;
  //MPLog("time: %d\n", mTime);

  fdata->pFft = pFft;
  fdata->pSubFft = pSubFft;
  fdata->len  = g_samp;
  fdata->df   = (float)(g_rate)/g_samp;
 
}


void get_wav2_data(struct WavWavData* wdata) {
  
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)]->get(pWav, g_samp);       
  ringbuff[(g_ch1-1)]->get(pSubWav, g_samp);       
  pthread_mutex_unlock(&m); 

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);
  } 
  if (hpf != NULL) {
    hpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);    
  }

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  } 
  if (hpf != NULL) {
    hpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);    
  }
  
  wdata->pWav = pWav;
  wdata->pSubWav = pSubWav;
  wdata->len  = g_samp;
  wdata->df   = (float)(g_rate)/g_samp;
  
}

void get_rawfil_data(struct WavWavData* wdata) {
  
  pthread_mutex_lock(&m); 
  ringbuff[(g_ch0-1)]->get(pWav, g_samp);   
  pthread_mutex_unlock(&m); 

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  /* note that this process is slightly different from above for keeping the original raw data*/
  if (lpf != NULL) {
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  } else { 
    // in case of no lpf applied.
    memcpy(pSubWav, pWav, sizeof(float)*g_samp);    
  }
  
  if (hpf != NULL) {
    hpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);    
  }
  
  wdata->pWav = pWav;
  wdata->pSubWav = pSubWav;
  wdata->len  = g_samp;
  wdata->df   = (float)(g_rate)/g_samp;  
}


void calc_orbit_data(struct OrbitData* odata) {

  /* get the data from the ringbuffer */
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)]->get(pWav, g_samp);
  if (g_chnm > 1) { /* fool proof */ 
    ringbuff[(g_ch1-1)]->get(pSubWav, g_samp);     
  } else {
    memset(pSubWav, 0, g_samp*sizeof(float));
  }
  pthread_mutex_unlock(&m);  
  
  uint32_t cur_time = millis();
  uint32_t dt = cur_time - last_time;
  
  struct SensorData sData;
  calcSensorData(&sData, pWav, g_samp, dt);
  odata->acc0 = sData.acc;
  odata->vel0 = sData.vel;
  odata->dis0 = sData.dis;
  
  calcSensorData(&sData, pSubWav, g_samp, dt);
  odata->acc1 = sData.acc;
  odata->vel1 = sData.vel;
  odata->dis1 = sData.dis;
  last_time = cur_time;
}


static void calcSensorData(struct SensorData* sdata, float* buf, int bufsize, int32_t dt) {
  
  float acc = 0.0;
  float vel = 0.0;
  float dis = 0.0;
  for (int i = 0; i < bufsize; ++i) {
    acc += buf[i];
  }
  acc /= bufsize;
  acc = acc*45.5 / g_sens;  // acc:mv g_sens:mv/m/s2
  vel += acc*dt;  // mm/sec
  dis += vel*dt;  // um

  sdata->acc = acc;
  sdata->vel = vel;
  sdata->dis = dis;

  return;
}
