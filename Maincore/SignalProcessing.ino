#include "AppSystem.h"
#include <malloc.h>

#define SIG_PROCESSING_PTHREAD

#ifdef SIG_PROCESSING_PTHREAD
#include <pthread.h>
pthread_t sig_thread;
#endif

static uint32_t last_time; // To measure the processing time

/* called by the audio library when its internal error occurs */
static void audio_attention_cb(const ErrorAttentionParam *atprm) {
  if (atprm->error_code >= AS_ATTENTION_CODE_ERROR) {
    bAudioError = true;
  }
}
/* the free heap of Maincore: the buffers depend on the FFT length and the channels */
static void log_heap(const char* when) {
  struct mallinfo mi = mallinfo();
  MPLog("Heap %s: free %d KB, largest %d KB (samp %d, chnm %d)\n"
      , when, mi.fordblks/1024, mi.mxordblk/1024, g_samp, g_chnm);
}

static void stop_sound() {
  if (!g_sound) return;
  snd_stop();
  g_sound = false;
}

void init_processing(int8_t sid, DynamicJsonDocument* sys) {
  int err, ret;
  int ch, line;

  log_heap("at init");
  /* Initialize Parameters */
  memset(&sData, 0, sizeof(struct SensorData));
  g_play_ok = false;
  g_sound = false;
  if (g_src != SRC_MIC) {
    /* the rate and the channels come from the file, CH1/CH2 are the channels of the file */
    struct PlayInfo info;
    g_play_ok = play_open(g_pfile, &info);
    if (g_play_ok) {
      g_rate = info.rate;
      g_chnm = info.chnm;
    } else {
      g_chnm = 1;
    }
    g_nring = (g_chnm < 2) ? 2 : g_chnm;
    if (g_ch0 < 1 || g_ch0 > g_chnm) g_ch0 = 1;
    if (g_chnm == 1) g_ch1 = 2;  /* the second ring buffer is kept zero */
    else if (g_ch1 < 1 || g_ch1 > g_chnm) g_ch1 = 2;  /* 0 after the single channel select page */
    MPLog("Play: CH%d CH%d\n", g_ch0, g_ch1);
  } else {
    if (g_chnm == 3) g_chnm = 4;
    g_nring = g_chnm;
  }
 
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
  ringbuff = new RingBuff*[g_nring]; // keep double space
  for (int n = 0; n < g_nring; ++n) {
    ringbuff[n] = new RingBuff(ringbuff_size);
  }
  if (pRaw == NULL || pWav == NULL || pSubWav == NULL || pTmp == NULL || ringbuff == NULL) {
    MPLog("not enough memory\n");
    log_heap("at the error");
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
    lpf2 = new IIRClassKai();
    lpf2->begin(TYPE_LPF, g_lpf, g_rate, 0.5, g_samp, Planar);
  }
  if (g_hpf != 0) { 
    hpf = new IIRClassKai();
    hpf->begin(TYPE_HPF, g_hpf, g_rate, 0.5, g_samp, Planar);
    hpf2 = new IIRClassKai();
    hpf2->begin(TYPE_HPF, g_hpf, g_rate, 0.5, g_samp, Planar);
  }

  /* Setup Audio Libraries */
  bAudioError = false;
  if (g_src == SRC_MIC) {
  MPLog("theAudio->begin()\n");
  theAudio->begin(audio_attention_cb);
  if (g_rate > 48000) {
    theAudio->setRenderingClockMode(AS_CLKMODE_HIRES);
  } else {
    theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
  }
  
  MPLog("theAudio->setRecorderMode()\n");
  /* the default fifo of the recorder (160KB) leaves no heap for FFT of 4096 x 4ch */
  /* keep AUDIO_FIFO_MS of the input in frames of the recorder instead */
  uint32_t fifo = (uint32_t)g_rate * g_chnm * sizeof(int16_t) / 1000 * AUDIO_FIFO_MS;
  fifo = (fifo + READ_FIFO_FRAME_SIZE - 1) / READ_FIFO_FRAME_SIZE * READ_FIFO_FRAME_SIZE;
  if (fifo < AUDIO_FIFO_MIN_FRAMES * READ_FIFO_FRAME_SIZE) fifo = AUDIO_FIFO_MIN_FRAMES * READ_FIFO_FRAME_SIZE;
  if (fifo > SIMPLE_FIFO_BUF_SIZE) fifo = SIMPLE_FIFO_BUF_SIZE;
  MPLog("recorder fifo: %d KB\n", (int)(fifo/1024));
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC, 0, fifo, false);
#ifdef USE_SD_CARD
  err = theAudio->initRecorder(AS_CODECTYPE_PCM ,"/mnt/sd0/BIN" ,g_rate ,g_chnm);
#else
  err = theAudio->initRecorder(AS_CODECTYPE_PCM ,"/mnt/spif/BIN" ,g_rate ,g_chnm);
#endif 
  if (err != AUDIOLIB_ECODE_OK) {
    MPLog("Recorder initialize error\n");
    return;
  }
  } /* g_src == SRC_MIC */

  /* FFT initialization */
  /* 0x10-0x70 is reserved for FFT applications */
  if (g_sid & 0x70) {
    MPLog("FFT initialization\n");
    log_heap("before FFT");
    fft = new FFTClassKai(g_samp);

    /* check if the memory is not free */
    if (pFft != NULL) free((float*)pFft);
    if (pSubFft != NULL) free((float*)pSubFft);

    /* allocate memories from MainCore memory */
    pFft = (float*)malloc(sizeof(float)*g_samp);
    pSubFft = (float*)malloc(sizeof(float)*g_samp);
    if (pFft == NULL || pSubFft == NULL) {
      MPLog("not enough memory\n");
      log_heap("at the error");
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
  if (g_src == SRC_MIC) {
    theAudio->startRecorder(); 
  } else {
    if (g_src == SRC_FILE_SOUND && g_play_ok) {
      struct PlayInfo info = { g_chnm, (uint32_t)g_rate, 0, 0 };
      g_sound = snd_start(g_pfile, &info, g_vol);
    }
    /* the graphs start with the sound */
    play_restart_clock();
  }
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

    /* the WAV file instead of the microphones */
    if (g_src != SRC_MIC) {
      if (g_sound) snd_fill();
      read_size = play_read(pRaw, MAX_FIFO_BUFF);
      if (read_size == 0) {
        usleep(5000);  /* wait for the next frames, and yield the process to the main loop */
        continue;
      }
      int frames = read_size/(g_chnm*sizeof(int16_t));
      pthread_mutex_lock(&m);
      for (int i = 0; i < g_chnm; i++) {
        ringbuff[i]->put((q15_t*)pRaw, frames, g_chnm, i);
      }
      pthread_mutex_unlock(&m);
      continue;
    }

    /* the capture does not work after the audio error, wait for finish_processing() */
    if (bAudioError) {
      usleep(10000);
      continue;
    }

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

    /* copy the selected channels to the recording fifo */
    rec_push((int16_t*)pRaw, captured_sample);  
    //attension!: if you enable usleep, 192kHz 4096 will not work!!
    //usleep(1); // yield the process to main loop
  }
  bThreadStopped = true;  
}


void finish_processing() {
  
  MPLog("Stop Recording\n");
  rec_log_heap("before stopping audio");
  if (g_src != SRC_MIC) {
    stop_sound();
    play_close();
  } else {
    if (bAudioError) {
      /* stopRecorder() did not return after the DMA error */
      MPLog("Audio error: skip stopRecorder\n");
    } else {
      theAudio->stopRecorder();
      REC_DLOG("stopRecorder done\n");
    }
    theAudio->setReadyMode();
    REC_DLOG("setReadyMode done\n");
    theAudio->end();  
    REC_DLOG("Audio end done\n");
  }
  usleep(1);

  /* free the memory for Audio Library */ 
  if (pRaw != NULL) free((char*)pRaw);
  if (pWav != NULL) free((float*)pWav);  
  if (pSubWav != NULL) free((float*)pSubWav);  
  if (ringbuff != NULL) {
    for (int n = 0; n < g_nring; ++n) {
      delete ringbuff[n];
    }
    delete[] ringbuff;
  }
  if (pTmp != NULL) free((float*)pTmp);
  rec_release();
  REC_DLOG("Audio memories freed\n");
  pRaw = NULL;
  pWav = NULL;
  pSubWav = NULL;
  pTmp = NULL;
  ringbuff = NULL;

  /* free the memory for Filtering */ 
  if (lpf != NULL) delete lpf;
  if (hpf != NULL) delete hpf;
  if (lpf2 != NULL) delete lpf2;
  if (hpf2 != NULL) delete hpf2;
  lpf = NULL;
  hpf = NULL;
  lpf2 = NULL;
  hpf2 = NULL;

  /* sid of 0x10-0x70 is reserved for FFT applications */
  if (g_sid & 0x70) {
    if (pFft != NULL) free((float*)pFft);
    if (pSubFft != NULL) free((float*)pSubFft);
    delete fft;     
    pFft = NULL;
    pSubFft = NULL;
    fft = NULL;
  }
  log_heap("after finish");
}

void pause_processing() {
  /* the file is read while the data is saved: keep the sound and the clock going */
  if (g_src != SRC_MIC) return;
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
  if (g_src != SRC_MIC) return;  /* see pause_processing() */
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
  if (lpf2 != NULL) {
    lpf2->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  }
  if (hpf2 != NULL) {
    hpf2->get(pTmp, pSubWav, g_samp);
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
  if (lpf2 != NULL) {
    lpf2->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  } 
  if (hpf2 != NULL) {
    hpf2->get(pTmp, pSubWav, g_samp);
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
