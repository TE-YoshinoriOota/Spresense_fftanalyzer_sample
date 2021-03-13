#include "AppSystem.h"


/* setup function starts from here */
void setup() {
  int ret;
  
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial) {}

  while (!theSD.begin()) {
    Serial.println("[Main] Insert SD card");
    error_notifier(FILE_ERROR);
  }
  
  ret = MP.begin(SUBCORE);
  if (ret < 0) {
    Serial.println("[Main] MP.begin error: " + String(ret));
    while(true) { 
      error_notifier(MP_ERROR); 
    }
  }
  
  djson = updateJson(djson, NULL);
  int8_t sid = SID_REQ_JSONDOC;
  ret = MP.Send(sid, djson, SUBCORE);
  if (ret < 0) {
    Serial.println("[Main] MP.Send error: " + String(ret));
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
  Serial.println("[Main] sid = " + String(sid));

  if (sid == SID_REQ_JSONDOC) {
  /* request to change page from Subcore */

    /* stop the signal processing */
    bProcessing = false;
    Serial.println("[Main] Stop Thread");
    
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
      Serial.println("[Main] MP.Send error: " + String(ret));
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
    Serial.println("[Main] init_processing");
    readSysprop(sid, sjson);     
    bProcessing = true;
    init_processing(sid, sjson);
    bSignalProcessingStarted = true;
    usleep(MAX_USTIME_FOR_CAPTURING); // yield the process to SignalProcessing
  }
    
  /* request data to monitor sensors */
  if (sid == SID_REQ_MONDATA) {

    Serial.println("[Main] Calculating sensor data");
    calc_sensor_data(&sData);

    struct SensorData sdata;
    memcpy(&sdata, &sData, sizeof(sData));

    Serial.println("[Main] Send monitor data to Subcore");

    /* send the data to update the number on LCD display */
    ret = MP.Send(sid, &sdata, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while(true) {
        error_notifier(MP_ERROR);
      }    
    }
    usleep(1);
    return;      
  } 
  
  /* request calculate fft data */  
  if (sid == SID_REQ_WAV_FFT) {

    Serial.println("[Main] Calculating fft-wav data");
    calc_fft_data(&fftWavData);

    Serial.println("[Main] Send fft-wav data to SUBCORE");

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &fftWavData, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while (true) { 
        error_notifier(MP_ERROR); 
      }    
    } 
    usleep(1);
    return; 
  } 
  
  /* request calculate double fft data */
  if (sid == SID_REQ_FFT_FFT) {

    Serial.println("[Main] Calculating fft-fft data");
    calc_fft2_data(&fft2Data);

    Serial.println("[Main] Send fft-fft data to SUBCORE");

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &fft2Data, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while (true) {
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1);
    return;        
  } 
  
  /* request calculate double wav data */   
  if (sid == SID_REQ_WAV_WAV) {

    Serial.println("[Main] Collecting wav-wav data");
    get_wav2_data(&wav2Data);

    Serial.println("[Main] Send wav-wav data to SUBCORE");
    
    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &wav2Data, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1);
    return;        
  } 
  
  /* request calculate double wav data */   
  if (sid == SID_REQ_RAW_FIL) {

    Serial.println("[Main] Collecting filtered wav data");
    get_rawfil_data(&wav2Data);

    Serial.println("[Main] Send raw-filtered data to SUBCORE");
    
    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &wav2Data, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1);
    return;      
  }
  
  usleep(1); // yield the process to SignalProcessing
}
