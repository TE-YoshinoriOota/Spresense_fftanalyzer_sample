#include "AppSystem.h"

// #define SPECIFY_START_PAGE

/* setup function starts from here */
void setup() {
  int ret;
  
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial) {}

#ifdef USE_SD_CARD
  while (!theSD.begin()) {
    MPLog("Insert SD card\n");
    error_notifier(FILE_ERROR);
  }
#else
  /* Flash */
#endif

  ret = MP.begin(SUBCORE);
  if (ret < 0) {
    MPLog("MP.begin error: %d\n", ret);
    while(true) { 
      error_notifier(MP_ERROR); 
    }
  }

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
  MPLog("sid = %d\n", sid);

  if (sid == SID_REQ_JSONDOC) {
  /* request to change page from Subcore */

    /* stop the signal processing */
    bProcessing = false;
    MPLog("Stop Thread\n");
    
    /* wait for the thread to stopp */
    while (!bThreadStopped) { 
      usleep(1); 
    } 
    
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
    ret = MP.Send(sid, djson, SUBCORE);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1); // yield 
    return;
  } 
  
  /* need signal processing, starts the sub process */
  /* request to launch the process, if it not started */
  if (bProcessing == false) {
    MPLog("init_processing\n");
    readSysprop(sid, sjson);     
    bProcessing = true;
    init_processing(sid, sjson);
    bSignalProcessingStarted = true;
    //usleep(MAX_USTIME_FOR_CAPTURING); // yield the process to SignalProcessing
    usleep(1);
  }
    
  /* request data to monitor sensors */
  if (sid == SID_REQ_MONDATA) {

    MPLog("Calculating sensor data\n");
    calc_sensor_data(&sData);

    struct SensorData sdata;
    memcpy(&sdata, &sData, sizeof(sData));

    MPLog("Send monitor data to Subcore\n");

    /* send the data to update the number on LCD display */
    ret = MP.Send(sid, &sdata, SUBCORE);
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
  if (sid == SID_REQ_WAV_FFT) {

    MPLog("Calculating fft-wav data\n");
    calc_fft_data(&fftWavData);

    MPLog("Send fft-wav data to SUBCORE\n");

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &fftWavData, SUBCORE);
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
  if (sid == SID_REQ_FFT_FFT) {

    MPLog("Calculating fft-fft data\n");
    calc_fft2_data(&fft2Data);

    MPLog("Send fft-fft data to SUBCORE\n");

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &fft2Data, SUBCORE);
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
  if (sid == SID_REQ_WAV_WAV) {

    MPLog("Collecting wav-wav data\n");
    get_wav2_data(&wav2Data);

    MPLog("Send wav-wav data to SUBCORE\n");
    
    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &wav2Data, SUBCORE);
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
  if (sid == SID_REQ_ORBITDT) {

    MPLog("Collecting filtered wav data\n");
    calc_orbit_data(&orbitData);

    MPLog("Send orbit data to SUBCORE\n");
    
    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &orbitData, SUBCORE);
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
  if (sid == SID_REQ_RAW_FIL) {

    MPLog("Collecting filtered wav data\n");
    get_rawfil_data(&wav2Data);

    MPLog("Send raw-filtered data to SUBCORE\n");
    
    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &wav2Data, SUBCORE);
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
