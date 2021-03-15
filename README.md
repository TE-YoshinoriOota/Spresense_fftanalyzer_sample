# Spresense FFT Analyzer (SPREFTA) sample

# Software Structure of SPREFTA
SPREFTA uses 2 core. Maincore takes in charge of Signal Processing, Subcore-1 takes in charge of Graphics. To make the software understand easily and simply, these programs are made by Spresense Arduino Library. So you can remake them very easily. 


# Singnal Processing on Maincore
Signal Processing contains tasks of signal capturing and signal processing. Sensor signal is aquired by Spresense micropone interface supporting sampling rate of 16kHz, 48kHz and 192 kHz. SPREFTA supports digital fiters like a high pass and a low pass filter and the fast fourie transformation and so on. Here is the major speification of SPREFTA.

## Major Specification
||Specification|note|
|---:|-------|------|
|Input Channel|up to 4 channel||
|Sampling Rate|16kHz, 48kHz, 192kHz||
|Lines for Singal Processing |100,200,400,800,1600|256,512,1024,2046,4096 samples|
|Low pass filter|1kHz,2kHz,5kHz,10kHz|Programable, see the source code for the detail|
|High pass filter|10Hz, 15Hz, ..|Programable, see the source code for the detail|
|Fast Fourie Transform|Rectangular, Hanning, Hamming window|Flat-top will support soon|
|Dual signal analysis|Display dual signals in frequecny graphs to compare the signal characteristics|
|Orbit analysis|Plot the dual signal in the graph to analyze colleration of two signals||

## Signal capturing
Signals captured by Spresense microphone interface are stored in Ringbuffer prepared for each channel with no conditions. This routine should not be obstucled by any tasks. Imagine that capturing signal of 192kHz in 256 samples, the allowed time for capturing is only 1.3 msec (1/192000 x 256 = 1.3msec). So this routine is implemented on the independent thread with high priority. The structure of the signal capturing is like this.

'''
while(bProcessing) { 

    err = theAudio->readFrames(pRaw, buffer_size, &read_size);
    if (err != AUDIOLIB_ECODE_OK && err != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
      Serial.println("[Main] Error err = " + String(err));
      bProcessing = false;
      break;
    }
    
    /* check if the read data is zero, just rest and continue */ 
    if (read_size == 0) {
      usleep(1); // yield the process to the main loop
      continue;
    }

    // printf("read_size: %d\n", read_size);
     
    /* push the captured data to the ringbuffers */
    int captured_sample = read_size/(g_chnm*sizeof(int16_t));
    pthread_mutex_lock(&m);
    for (int i = 0; i < g_chnm; i++) {
      ringbuff[i].put((q15_t*)pRaw, captured_sample, g_chnm, i);
    }
    pthread_mutex_unlock(&m);  
    usleep(1); // yield the process to main loop
  }
  bThreadStopped = true;  
}
'''

# Signal Processing
Signal processing is done by as per a request from subcore-1, and the communication between the maincore and the subcore-1 is done on the main loop. It means that signal processing is worked on the different thread from the signal capturing thread. The main loop on the maincore is very simple. Checking requests from the subcore-1 constantly, when a request arrived, check the sid 


'''
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
'''

Signal processing are different implemented manner as per each application on subcore-1. Generally, subcore applications request different data, so signal processing should be implemented differently. When taking signal from RingBuffer, you should take care to handle. Since the each process is running on the main loop, extracting signal data from the ringbuffer managed by independent thread should be protected by mutex to pick up safely.

'''
void calc_sensor_data(struct SensorData* sdata) {

  /* get the data from the ringbuffer */
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);   
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
  ringbuff[(g_ch0-1)].get(pWav, g_samp);       
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
  fft->fft_amp(pFft, pTmp);
  //mTime = millis() - mTime;
  //Serial.println("[Main] " + String(mTime));
   
  fdata->pWav = pWav;
  fdata->pFft = pFft;
  fdata->len  = g_samp;
  fdata->df   = (float)(g_rate)/g_samp;
}

void calc_fft2_data(struct FftFftData* fdata) {
  
  /* Copy data for FFT calculation */
  uint32_t mTime = millis();
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);       
  ringbuff[(g_ch1-1)].get(pSubWav, g_samp);       
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
  fft->fft_amp(pFft, pTmp);

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
  fft->fft_amp(pSubFft, pTmp);
  //mTime = millis() - mTime;
  //Serial.println("[Main] " + String(mTime));

  fdata->pFft = pFft;
  fdata->pSubFft = pSubFft;
  fdata->len  = g_samp;
  fdata->df   = (float)(g_rate)/g_samp;
 
}


void get_wav2_data(struct WavWavData* wdata) {
  
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);       
  ringbuff[(g_ch1-1)].get(pSubWav, g_samp);       
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
  ringbuff[(g_ch0-1)].get(pWav, g_samp);   
  pthread_mutex_unlock(&m); 

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  /* note that this process is slightly different from above for keeping the original raw data*/
  if (lpf != NULL) {
    Serial.println("[Main] applying low pass filter");
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  } else { 
    // in case of no lpf applied.
    memcpy(pSubWav, pWav, sizeof(float)*g_samp);    
  }
  
  if (hpf != NULL) {
    Serial.println("[Main] applying high pass filter");
    hpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);    
  }
  
  wdata->pWav = pWav;
  wdata->pSubWav = pSubWav;
  wdata->len  = g_samp;
  wdata->df   = (float)(g_rate)/g_samp;  
}
'''

# Commnication between Maincore and Subcore-1
Commnication between Maincore and Subore-1 is based on a master-slave model. The master is subcore-1, the slave is maincore. The main loop of the subcore-1 takes in charge of  managing communication to Maincore and reflect the receiving data to the display. The applicaion data is identified by sid. There is a rule to assigned the request id (application id), the id of applications requiring FFT should be numbered between 0x10 to 0x70. The reason is that the maincore prepares the resource for FFT by looking this id. 

The process of the main loop on subcore-1 is also simple. Checking a change of the page at first, secondly if an application requiring processed data is running, requests data to maincore. Thirdly checking the data arriaval from maincore and if it threre, reflecting it to the application. 

To say it simply, sending requests to maincore and receiving reqeusted data from maincore are done by the mainloop.

'''
void loop() {
  int ret;
  int8_t sid = 0x00;
  int appid = 0;
  struct Response *res = NULL;
  void *data = NULL;

  /* Send a request to MainCore */
  if (updateScreen()) {
    /* change page */
    //stopApplication();
    stopApplication();
    getResponse(res);
    sid = SID_REQ_JSONDOC;
    ret = MP.Send(sid, res);
    if (ret < 0) {
      Serial.println("MP.Send error: " + String(ret));
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }   
  } 
  
  appid = isAppRunning();
  
  if ((appid > 0) && isDataReceived()) {
    switch(appid) {
    case APP_ID_MONDATA: sid = SID_REQ_MONDATA; break;
    case APP_ID_WAV_FFT: sid = SID_REQ_WAV_FFT; break;
    case APP_ID_FFT_FFT: sid = SID_REQ_FFT_FFT; break;
    case APP_ID_WAV_WAV: sid = SID_REQ_WAV_WAV; break;
    case APP_ID_RAW_FIL: sid = SID_REQ_RAW_FIL; break;
    }

    MPLog("Request data to Maincore: 0x%02x\n", sid);
    
    ret = MP.Send(sid, data); /* data is dummy */
    if (ret < 0) {
      Serial.println("MP.Send error: " + String(ret));
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
  MPLog("sid: 0x%02x\n", sid);

  /* check if the data is json documents */
  if (sid == SID_REQ_JSONDOC) {
    
    MPLog("Update screen\n");
    json = (DynamicJsonDocument*)data;
    clearScreen(json);
    char* title = (*json)["title"];
    MPLog("Build Screen %s\n", title);
    BuildScreen(json);
    
    return;  
  }
  
  if (sid == SID_REQ_MONDATA) {

    MPLog("Sensor Data arrived\n");
    
    /* check if the data is for sensor monitor application */
    struct SensorData *sd = (struct SensorData*)data;
    appSensorValue(sd->acc, sd->vel, sd->dis);
    receivedData();
    delay(100); /* delay for avoiding data flicker on LCD */
    
    return;
  } 

  if (sid == SID_REQ_WAV_FFT) {

    MPLog("FFT-WAV Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct FftWavData *fft = (struct FftWavData*)data;
    if (fft->len == 0) { /* sometimes Maincore send a null data, sp check it */
      MPLog("fft->len(%d)\n", fft->len);
      receivedData();
      return;
    }
    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
    appDraw2WayGraph(fft->pWav, fft->len, fft->pFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time);

    return;
  } 

  if (sid == SID_REQ_FFT_FFT) {

    MPLog("FFT-FFT Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct FftFftData *fft = (struct FftFftData*)data;
    if (fft->len == 0) { /* sometimes MainCore send a null data */
      MPLog("fft->len(%d)\n", fft->len);
      receivedData();
      return;
    }
    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
    appDraw2FftGraph(fft->pFft, fft->pSubFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time); // is this call really needed?

    return;
  } 

  if (sid == SID_REQ_WAV_WAV) {

    MPLog("WAV-WAV Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { /* sometimes MainCore send a null data */
      MPLog("wav->len(%d)\n", wav->len);
      receivedData();
      return;
    }
    
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
    appDrawOrbitGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time); // is this call really needed?
    
    return;
  }
  
  if (sid == SID_REQ_RAW_FIL) {

    MPLog("RAW-FILL Data arrived\n");
    
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { 
      MPLog("wav->len(%d)\n", wav->len);
      receivedData();
      return;
    }
    
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
    appDrawFilterGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time); // is this call really needed?

    return;
  }  
  
  return;
}
'''

# the construction process of applications on subcore
The application framework of SPREFTA is relatively complicated. The application model is based on Model-View-Controller. The main loop is the controller needless to say. The model is functions of appDrawxxx implemented in ScreenApps.ino. The view is functions in ScreenElements.ino. Addition to that, The control buttons are handled by ScreenElements. The view is made by the builder functions in BuildScreen.ino. The construction process of an application is as follows. 

(1) A user changes an application page by pressing button "NEXT" or "BACK".
(2) The main loop detects the change request and send the request to get JSON data of the next page.
(3) Maincore gets the request, sends the JSON data back to Subcore-1
(4) Subcore-1 gets the data in the main loop, then starts to build the view by using JSON doc. During the building the view, if the application needs the signal processing data, builder sets a data-request by calling startApplication(). 
(5) The main loop detects the data request by calling isRunningApplication(), and send the request to Maincore.
(6) Maincore gets the request, sends the signal processed data back to Subcore-1.
(7) The main loop receives the data and plot the data on LCD by appDrawxxxx functions
(8) Loop from (5) to (7) until the end of the application by User instruction pressing "NEXT" or "BACK"

When a user presses "NEXT" or "BACK" button, the framework makes the applicatioon loop go back to (1) and stops the application at the (2) step.


