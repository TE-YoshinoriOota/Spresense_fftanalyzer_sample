#include "AppSystem.h"


void update_system_properties(int8_t sid, DynamicJsonDocument* sys) {
  int ch, line;
  /* update System Properties */
  g_sid   = sid;              // MPLog("sid : %d\n", g_sid);
  g_sens  = (*sys)["sens"];   // MPLog("sens: %d\n", g_sens);
  g_gain  = (*sys)["gain"];   // MPLog("gain: %d\n", g_gain);
  g_chnm  = (*sys)["chnm"];   // MPLog("chnm: %d\n", g_chnm);
  ch      = (*sys)["ch"];     // 
  g_ch0   = ch & 0x00ff;      // MPLog("ch0 : %d\n", g_ch0);
  g_ch1   = ch >> 8;          // MPLog("ch1 : %d\n", g_ch1);
  g_rate  = (*sys)["rate"];   // MPLog("rate: %ld\n", g_rate);
  line    = (*sys)["line"];   //
  g_samp  = line*2.56;        // MPLog("samp: %d\n", g_samp);
  g_lpf   = (*sys)["lpf"];    // MPLog("lpf : %d\n", g_lpf);
  g_hpf   = (*sys)["hpf"];    // MPLog("hpf : %d\n", g_hpf);
  g_win   = (*sys)["win"];    MPLog("win : %d\n", g_win);  
}

void readSysprop(int8_t sid, DynamicJsonDocument* doc) {
  
  File mySysprop;
  sfile = "sys.txt";
  MPLog("Open system file : %s\n", sfile);
#ifdef USE_SD_CARD
  mySysprop = theSD.open(sfile, FILE_READ);
#else
  mySysprop = Flash.open(sfile, FILE_READ);
#endif
  if (!mySysprop) {
    MPLog("Cannot open %s\n", sfile.c_str());
    while (true) { 
      error_notifier(JSON_ERROR);
    }
  }
  String strSysprop;
  while (mySysprop.available()) strSysprop += char(mySysprop.read());
  DeserializationError error = deserializeJson(*doc, strSysprop);
  if (error) {
    MPLog("deserializeJson() failed: %s\n", error.f_str());
    while (true) {
      error_notifier(JSON_ERROR);
    }
  }
  mySysprop.close();
  update_system_properties(sid, doc);
}

void updateSysprop(int8_t sid, DynamicJsonDocument* doc, struct Response* res) {
  
  if (res == NULL) return false;
  if (res->value < 0) return false;

  /* Read the current system property at first */
  File mySysprop;
  sfile = "sys.txt";
  MPLog("Open system file : %s\n", sfile.c_str());
#ifdef USE_SD_CARD
  mySysprop = theSD.open(sfile, FILE_READ);
#else
  mySysprop = Flash.open(sfile, FILE_READ);
#endif
  if (!mySysprop) {
    MPLog("Cannot open %s\n", sfile.c_str());
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }
  
  String strSysprop;
  while (mySysprop.available()) strSysprop += char(mySysprop.read());
  DeserializationError error = deserializeJson(*doc, strSysprop);
  if (error) {
    MPLog("deserializeJson() failed: %s\n", error.f_str());
    while (true) error_notifier(JSON_ERROR);
  }
  mySysprop.close();

  /* update the system properties according to the response */
  char* label = res->label;
  int   value = res->value;
#ifdef USE_SD_CARD
  if (theSD.exists(sfile)) theSD.remove(sfile);
#else
  if (Flash.exists(sfile)) Flash.remove(sfile);
#endif
  MPLog("Update System Property File : %s\n", sfile.c_str());
  MPLog("%s : %d\n", label, value);

  int8_t appid = (*doc)["id"];
  appid /= 100;
  (*doc)["app"] = appid;
  (*doc)[label] = value;
#ifdef USE_SD_CARD
  mySysprop = theSD.open(sfile , FILE_WRITE);
#else
  mySysprop = Flash.open(sfile , FILE_WRITE);
#endif
  if (!mySysprop) {
    MPLog("Cannot open %s\n", sfile.c_str());
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }

  size_t wsize = serializeJson((*doc), mySysprop);
  if (wsize == 0) {
    MPLog("serializeJson() failed: ");
    while (true) {
      error_notifier(JSON_ERROR);
    }
  }
  mySysprop.close(); 
  
  update_system_properties(sid, doc);
}

DynamicJsonDocument* updateJson(DynamicJsonDocument* doc, struct Response* res) {
 
  File myJson;
  if (res == NULL) {
    // MPLog("Response is null. Open the home menu\n");
    dfile = "AA000.txt";
  } else {
    int cur0 = res->cur0;
    int cur1 = res->cur1;
    int id = res->next_id;

    /* update the document properties according to the response */
#ifdef USE_SD_CARD
    if (theSD.exists(dfile)) theSD.remove(dfile);
#else
    if (Flash.exists(dfile)) Flash.remove(dfile);
#endif
    MPLog("Update json file : %s", dfile.c_str());
    (*doc)["cur0"] = cur0;
    (*doc)["cur1"] = cur1;
#ifdef USE_SD_CARD
    myJson = theSD.open(dfile , FILE_WRITE);
#else
    myJson = Flash.open(dfile , FILE_WRITE);
#endif
    size_t wsize = serializeJson((*doc), myJson);
    if (wsize == 0) {
      MPLog("serializeJson() failed: ");
      while (true) {
        error_notifier(JSON_ERROR);
      }
    }
    myJson.close();    
    char fname[12] = {0};
    memset(fname, '\0', sizeof(char)*12);
    sprintf(fname, "AA%03d.txt", id);
    dfile = String(fname);
  }
    
  /* Open the new json file for the request to change page from Subcore */
  MPLog("Open json file : %s\n", dfile.c_str());
#ifdef USE_SD_CARD
  myJson = theSD.open(dfile, FILE_READ);
#else
  myJson = Flash.open(dfile, FILE_READ);
#endif
  if (!myJson) {
    MPLog("Cannot open %s\n", dfile.c_str());
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }
  
  String strJson;
  while (myJson.available()) strJson += char(myJson.read());
  DeserializationError error = deserializeJson(*doc, strJson);
  if (error) {
    MPLog("deserializeJson() failed: %s\n", error.f_str());
    while (true) {
      error_notifier(JSON_ERROR);
    }
  }
  myJson.close();
  return doc;
}
