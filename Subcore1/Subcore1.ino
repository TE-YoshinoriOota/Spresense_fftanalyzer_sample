#include "AppScreen.h"

DynamicJsonDocument* json;

void setup() {
  int ret;
  int8_t sid;
  /* MP.begin() first: Maincore waits for the boot completion only for 1 sec */
  ret = MP.begin();
  if (ret < 0) {
    MPLog("MP.begin error: %d\n", ret);
    while(true) {
      error_notifier(MP_ERROR);
    }
  }
  uint32_t t0 = millis();
  hardwareSetup();
  MPLog("hardwareSetup: %d ms\n", (int)(millis() - t0));

  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {
  int ret;
  int8_t sid = 0x00;
  int appid = 0;
  static struct Response resp; /* sent to Maincore: it must stay valid after MP.Send */
  struct Response *res = &resp;
  void *data = NULL;

  /* Send a request to MainCore */
  if (updateScreen()) {
    /* change page */
    stopApplication();
    getResponse(res);
    sid = SID_REQ_JSONDOC;
    bPageRequested = true;
    ret = MP.Send(sid, res);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    // erase save_request for safety 
    pthread_mutex_lock(&mtx);
    save_request = SAVE_INIT;
    pthread_mutex_unlock(&mtx);  
  } 
  
  /* screen capture by BACK + NEXT: start it only when no reply is expected from Maincore */
  /* (a data request, or a page change) so that the replies are not mixed */
  checkCaptureCombo();
  checkSaveCombo();
  appSaveStatusTimeout();
  if (!bPageRequested && (isAppRunning() == 0 || isDataReceived()) && takeCaptureRequest()) {
    captureScreen();
  }

  /* REC/STOP requested by B0 on the REC MONITOR screen */
  if (scrType == SCR_TYPE_RECM && takeRecToggleRequest()) {
    bRecording = !bRecording;
    sid = bRecording ? SID_REQ_RECSTRT : SID_REQ_RECSTOP;
    ret = MP.Send(sid, data); /* data is dummy */
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) {
        error_notifier(MP_ERROR);
      }
    }
    appRecStatus(true);
  }

  appid = isAppRunning();

  if ((appid > 0) && isDataReceived()) {
    if (bRecording) save_request = SAVE_INIT; /* no CSV while recording */
    switch(appid) {
    case APP_ID_MONDATA: sid = SID_REQ_MONDATA | save_request; break;
    case APP_ID_WAV_FFT: sid = SID_REQ_WAV_FFT | save_request; break;
    case APP_ID_FFT_FFT: sid = SID_REQ_FFT_FFT | save_request; break;
    case APP_ID_WAV_WAV: sid = SID_REQ_WAV_WAV | save_request; break;
    case APP_ID_RAW_FIL: sid = SID_REQ_RAW_FIL | save_request; break;
    case APP_ID_LISSAJ:  sid = SID_REQ_WAV_WAV | save_request; break;
    case APP_ID_SPECTRO: sid = SID_REQ_SPECTRO | save_request; break;
  }
  pthread_mutex_lock(&mtx);
#ifdef SCR_DEBUG
  MPLog("save_request:%d\n", save_request);
#endif
  save_request = SAVE_INIT;
  pthread_mutex_unlock(&mtx);
  if (sid & SID_REQ_SAVEDAT) MPLog("Save the data\n");
      
#ifdef MP_DEBUG
    MPLog("Request data to Maincore: 0x%02x\n", sid);
#endif

    ret = MP.Send(sid, data); /* data is dummy */
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    requestedData();
  }
    

  /* data arrived from MainCore */
  float dummy = 0.0;
  if (MP.Recv(&sid, &data) < 0) {
    return;
  }

#ifdef MP_DEBUG
  MPLog("sid: 0x%02x\n", sid);
#endif

  /* check whether sid is sdcard error */
  /*     tentative implementation      */    
  if (sid == SID_REQ_ESDCARD) {
    buildSDCardErrorPage();
    return;
  }

  /* the result of saving the data: it comes before the data */
  if (sid == SID_REQ_SAVED) {
    if (!bPageRequested && isAppRunning() != 0) appSaveStatus((struct SaveStatus*)data);
    return;
  }

  /* reply to REC/STOP: the status is updated by Maincore while recording */
  if (sid == SID_REQ_RECSTRT || sid == SID_REQ_RECSTOP) {
    if (scrType != SCR_TYPE_RECM) return; /* the page already changed */
    recStat = (struct RecStatus*)data;
    if (recStat->state == REC_STATE_ERROR && sid == SID_REQ_RECSTRT) {
      bRecording = false; /* failed to start */
    }
    appRecStatus(true);
    return;
  }

  /* check if the data is json documents */
  if (sid == SID_REQ_JSONDOC) {
#ifdef MP_DEBUG
    MPLog("Update screen\n");
#endif
    bPageRequested = false;
    json = (DynamicJsonDocument*)data;
    ClearScreen(json);
    char* title = (*json)["title"];
#ifdef MP_DEBUG
    MPLog("Build Screen %s\n", title);
#endif
    BuildScreen(json);
    return;  
  }
  
  if (sid == SID_REQ_MONDATA) {
#ifdef MP_DEBUG
    MPLog("Sensor Data arrived\n");
#endif
    /* check if the data is for sensor monitor application */
    struct SensorData *sd = (struct SensorData*)data;
    appSensorValue(sd->acc, sd->vel, sd->dis);
    receivedData();
    delay(100); /* delay for avoiding data flicker on LCD */
    
    return;
  } 

  if (sid == SID_REQ_WAV_FFT) {
#ifdef MP_DEBUG
    MPLog("FFT-WAV Data arrived\n");
#endif  
    /* check if th data is for FFT monitor application */
    struct FftWavData *fft = (struct FftWavData*)data;
    if (fft->len == 0) { /* sometimes Maincore send a null data, sp check it */
      MPLog("fft->len(%d)\n", fft->len);
      receivedData();
      return;
    }
#ifdef MP_DEBUG
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
#endif  
    appDraw2WayGraph(fft->pWav, fft->len, fft->pFft, fft->len/2, fft->df);
    if (scrType == SCR_TYPE_RECM) appRecStatus(false);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
#ifdef MP_DEBUG
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time);
    return;
  } 

  if (sid == SID_REQ_FFT_FFT) {
#ifdef MP_DEBUG
    MPLog("FFT-FFT Data arrived\n");
#endif
    /* check if th data is for FFT monitor application */
    struct FftFftData *fft = (struct FftFftData*)data;
    if (fft->len == 0) { /* sometimes MainCore send a null data */
#ifdef MP_DEBUG
      MPLog("fft->len(%d)\n", fft->len);
#endif
      receivedData();
      return;
    }
#ifdef MP_DEBUG    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
#endif
    appDraw2FftGraph(fft->pFft, fft->pSubFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
#ifdef MP_DEBUG    
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time); // is this call really needed?
    return;
  } 

  if (sid == SID_REQ_WAV_WAV) {
#ifdef MP_DEBUG
    MPLog("WAV-WAV Data arrived\n");
#endif
    /* check if th data is for FFT monitor application */
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { /* sometimes MainCore send a null data */
      MPLog("wav->len(%d)\n", wav->len);
      receivedData();
      return;
    }
#ifdef MP_DEBUG
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
#endif
    /* the reply may arrive after BACK stopped the application: choose by the screen, not by the app id */
    if (scrType == SCR_TYPE_LSJU) appDrawLissajous(wav->pWav, wav->pSubWav, wav->len);
    else appDraw2WavGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
#ifdef MP_DEBUG
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time); // is this call really needed?
    return;
  }
  
  if (sid == SID_REQ_RAW_FIL) {
#ifdef MP_DEBUG
    MPLog("RAW-FILL Data arrived\n");
#endif
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { 
#ifdef MP_DEBUG
      MPLog("wav->len(%d)\n", wav->len);
#endif
      receivedData();
      return;
    }
#ifdef MP_DEBUG
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
#endif
    appDraw2WavGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
#ifdef MP_DEBUG
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time); // is this call really needed?
    return;
  }  



  if (sid == SID_REQ_SPECTRO) {
#ifdef MP_DEBUG
    MPLog("Spectrogram Data arrived\n");
#endif  
    /* check if th data is for FFT monitor application */
    struct FftWavData *fft = (struct FftWavData*)data;
    if (fft->len == 0) { /* sometimes Maincore send a null data, sp check it */
      MPLog("fft->len(%d)\n", fft->len);
      receivedData();
      return;
    }
#ifdef MP_DEBUG
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
#endif  
    appDrawSpectroGraph(fft->pFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
#ifdef MP_DEBUG
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time);
    return;
  }   
  return;
}
