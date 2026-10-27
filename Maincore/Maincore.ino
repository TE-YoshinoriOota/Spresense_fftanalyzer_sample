#include "AppSystem.h"

// #define SPECIFY_START_PAGE
// #define MP_DEBUG

/* setup function starts from here */
void setup() {
  int ret;
  
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial) {}


  ret = MP.begin(SUBCORE);
  if (ret < 0) {
    MPLog("MP.begin error: %d\n", ret);
    while(true) { 
      error_notifier(MP_ERROR); 
    }
  }

#if defined(USE_SD_CARD) || defined(ENABLE_SAVE_DATA)
  while (!theSD.begin()) {
    MPLog("Insert SD card\n");
    int8_t sid = SID_REQ_ESDCARD;
    uint32_t dummy;
    ret = MP.Send(sid, &dummy, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    error_notifier(FILE_ERROR);
  }
#endif
  


  /* the setting files are in the config folder: move them there if they are on the root */
  cfg_migrate();
#if defined(USE_SD_CARD) || defined(ENABLE_SAVE_DATA)
  /* the recordings, the CSV files and the captures are in their folders */
  data_migrate();
#endif

#ifdef SPECIFY_START_PAGE
  struct Response res;
  memset(&res, 0, sizeof(struct Response));
  res.next_id = 356; // WAV-FFT page for Example
  djson = updateJson(djson, &res);
#else
  djson = updateJson(djson, NULL);
#endif
  int8_t sid = SID_REQ_JSONDOC;
  ret = MP.Send(sid, djson, SUBCORE);
  if (ret < 0) {
    MPLog("MP.Send error: %d\n", ret);
    while(true) { 
      error_notifier(MP_ERROR); 
    }    
  }
  MP.RecvTimeout(MP_RECV_POLLING);
}


/* main loop function starts from here */
void loop() {
  int ret;
  int8_t sid = 0;
  if (MP.Recv(&sid, &res, SUBCORE) < 0) {
    usleep(1); // yield the process to SignalProcessing
    return;
  }

  uint8_t usid = (uint8_t)sid;
  uint8_t appid = usid & SID_UMSK_SAVEDAT;
  bool sflag = usid & SID_MSK_SAVEDAT ? true : false;
  
#ifdef MP_DEBUG
  MPLog("sid = %d\n", sid);
  MPLog("appid: %d sflag:%d\n", appid, sflag);
#endif

  if (appid == SID_REQ_JSONDOC) {
  /* request to change page from Subcore */

    /* finish the WAV file before stopping the capture thread */
    rec_stop(&recStatus);

    /* stop the signal processing */
    bProcessing = false;
    MPLog("Stop Thread\n");
    
    /* wait for the thread to stop */
    while (!bThreadStopped) { 
      usleep(1); 
    } 
    
    if (bSignalProcessingStarted) {
      finish_processing();
#ifdef ENABLE_SAVE_DATA
      closeStorage();
#endif
      bSignalProcessingStarted = false;
    }
    
    MPLog("Response: %.4s value %d cur %d,%d next %d\n", res->label, res->value, res->cur0, res->cur1, res->next_id);

    /* update system properties */
    updateSysprop(sid, sjson, res);
    /* a mono file of FILE: the next page is the file to compare (if any) */
    play_route_compare(res, sjson);

    /* get the new page resources and update the json document */
    djson = updateJson(djson, res);
    patchPage(djson, sjson);
    patchSource(djson, sjson);
    int ch = (*sjson)["ch"];
    int ch0 = ch & 0x00ff;
    int ch1 = ch >> 8;
    (*djson)["ch0"] = ch0;
    (*djson)["ch1"] = ch1;

    /* send the json document to change the page on LCD */
    ret = MP.Send((int8_t)appid, djson, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
   
    usleep(1); // yield
    return;
  }

  /* a strip of the screen capture: it works on every page, so handle it before init_processing() */
  if (appid == SID_REQ_SCRCAP) {
    cap_strip((struct CapStrip*)res, &capStatus);
    ret = MP.Send(SID_REQ_SCRCAP, &capStatus, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) {
        error_notifier(MP_ERROR);
      }
    }
    return;
  }

  /* request to start/stop recording from the REC MONITOR screen */
  /* handle it before init_processing() so that g_sid keeps the FFT application id */
  if (appid == SID_REQ_RECSTRT || appid == SID_REQ_RECSTOP) {
    if (appid == SID_REQ_RECSTRT) rec_start(&recStatus);
    else                          rec_stop(&recStatus);

    /* Subcore reads the status through this pointer while recording */
    ret = MP.Send((int8_t)appid, &recStatus, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) {
        error_notifier(MP_ERROR);
      }
    }
    usleep(1);
    return;
  }

  /* need signal processing, starts the sub process */
  /* request to launch the process, if it not started */
  if (bProcessing == false) {
    MPLog("init_processing\n");
    readSysprop(sid, sjson);     
    if ((*djson)["type"] == PAGE_TYPE_RECM && g_samp > REC_MAX_SAMP) {
      MPLog("REC MONITOR: samp %d -> %d\n", g_samp, REC_MAX_SAMP);
      g_samp = REC_MAX_SAMP;
    }
    bProcessing = true;
#ifdef ENABLE_SAVE_DATA
    openStorage(appid);
#endif
    init_processing(sid, sjson);
    bSignalProcessingStarted = true;
    usleep(1);
  }

  /* request data to monitor sensors */
  if (appid == SID_REQ_MONDATA) {

#ifdef MP_DEBUG
    MPLog("Calculating sensor data\n");
#endif

    calc_sensor_data(&sData);

    struct SensorData sdata;
    memcpy(&sdata, &sData, sizeof(sData));


#ifdef ENABLE_SAVE_DATA    
    if (sflag) { 
      pause_processing();
      saveData(&sdata);
      resume_processing();
    }
#endif

    /* send the data to update the number on LCD display */
    ret = MP.Send((int8_t)appid, &sdata, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) {
        error_notifier(MP_ERROR);
      }    
    }
    usleep(1);
    return;      
  } 
  
  /* request calculate fft data */  
  if (appid == SID_REQ_WAV_FFT || appid == SID_REQ_SPECTRO) {

#ifdef MP_DEBUG
    MPLog("Calculating fft-wav data\n");
#endif

    calc_fft_data(&fftWavData);


#ifdef ENABLE_SAVE_DATA
    /* pause_processing() breaks the recording, so no CSV while recording */
    if (sflag && !rec_is_active()) {
      pause_processing();
      saveData(&fftWavData);
      resume_processing();
      sendSaveStatus();
    }    
#endif  

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send((int8_t)appid, &fftWavData, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while (true) { 
        error_notifier(MP_ERROR); 
      }    
    } 
        
    usleep(1);
    return; 
  } 

  /* request calculate double fft data */
  if (appid == SID_REQ_FFT_FFT) {

#ifdef MP_DEBUG
    MPLog("Calculating fft-fft data\n");
#endif

    calc_fft2_data(&fft2Data);

#ifdef ENABLE_SAVE_DATA     
    if (sflag) { 
      pause_processing();
      saveData(&fft2Data);
      resume_processing();
      sendSaveStatus();
    }
#endif    

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send((int8_t)appid, &fft2Data, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while (true) {
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1);
    return;        
  } 
  
  /* request calculate double wav data */   
  if (appid == SID_REQ_WAV_WAV) {

#ifdef MP_DEBUG
    MPLog("Collecting wav-wav data\n");
#endif

    get_wav2_data(&wav2Data);
   
    
#ifdef ENABLE_SAVE_DATA     
    if (sflag) { 
      pause_processing();
      saveData(&wav2Data);
      resume_processing();
      sendSaveStatus();
    }
#endif
 
    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send((int8_t)appid, &wav2Data, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    
    usleep(1);
    return;        
  } 


  
  
  /* request calculate double wav data */   
  if (appid == SID_REQ_RAW_FIL) {

#ifdef MP_DEBUG
    MPLog("Collecting filtered wav data\n");
#endif

    get_rawfil_data(&wav2Data);
   
    
#ifdef ENABLE_SAVE_DATA    
    if (sflag) { 
      MPLog("save data!\n");
      pause_processing();
      saveData(&wav2Data);
      resume_processing();
      sendSaveStatus();
    }
#endif

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send((int8_t)appid, &wav2Data, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1);
    return;      
  }
  
  usleep(1); // yield the process to SignalProcessing
}
