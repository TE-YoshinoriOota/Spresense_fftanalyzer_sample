#ifndef __APP_SCREEN_HEADER_GURAD__
#define __APP_SCREEN_HEADER_GURAD__

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ArduinoJson.h>
#include <MP.h>

// #define MP_DEBUG
// #define SCR_DEBUG

/* LCD SCREEN SIZE */
#define SCREEN_WIDTH  (320)
#define SCREEN_HEIGHT (240)


/* LED MASK FOR ERROR HANDLING */
#define LED0_MASK (0x01)
#define LED1_MASK (0x02)
#define LED2_MASK (0x04)
#define LED3_MASK (0x08)


/* ERROR DIFINITIONS */
/* Blinking LED3 means Subcore error */
#define MP_ERROR  (0x09)  /* LED2 and LED3 blink */


/* Screen type definitions */
#define SCR_TYPE_PAGE  (0)
#define SCR_TYPE_MENU  (1)
#define SCR_TYPE_INPT  (2)
#define SCR_TYPE_DMNU  (3)
#define SCR_TYPE_MNTR  (4)
#define SCR_TYPE_WVFT  (5)
#define SCR_TYPE_FFT2  (6)
#define SCR_TYPE_ORBT  (7)
#define SCR_TYPE_FLTR  (8)
#define SCR_TYPE_WVWV  (9)
#define SCR_TYPE_WFLG  (10)
#define SCR_TYPE_FTLG  (11)
#define SCR_TYPE_SCLE  (12)
#define SCR_TYPE_WFDB  (13)
#define SCR_TYPE_FFDB  (14)


/* TITLE COORDINATIONS */
#define TITLE_DECO_LINE (35)
#define TITLE_SIDE (10)
#define TITLE_HEAD (10)


/* BUTTON COORDINATIONS */
#define BUTTON_DECO_LINE (SCREEN_HEIGHT-30)
#define BUTTON_HEAD    (220)
#define BUTTON0_SIDE   (20)
#define BUTTON1_SIDE   (80)
#define BUTTON2_SIDE   (160)
#define BUTTON3_SIDE   (250)


/* MENU SCREEN COORDINATIONS */
#define MENU_ITEM_SIDE  (40)
#define MENU_ITEM0_HEAD (55)
#define MENU_CUR_SIDE    TITLE_SIDE
#define MENU_CUR_SIZE   (15)
#define MENU_ITEM1_HEAD (MENU_ITEM0_HEAD+40)
#define MENU_ITEM2_HEAD (MENU_ITEM1_HEAD+40)
#define MENU_ITEM3_HEAD (MENU_ITEM2_HEAD+40)


/* CH STATUS COORDINATIONS */
#define DISP_CH_SIDE   (270)
#define DISP_CH0_HEAD   MENU_ITEM0_HEAD
#define DISP_CH1_HEAD   MENU_ITEM1_HEAD
#define DISP_CH2_HEAD   MENU_ITEM2_HEAD
#define DISP_CH3_HEAD   MENU_ITEM3_HEAD


/* INPUT SCREEN COORDINATIONS */
#define INPUT_MARGIN (5)
#define INPUT_LABEL_HEAD  MENU_ITEM0_HEAD
#define INPUT_LABEL_SIDE  TITLE_SIDE
#define INPUT_BOX_SIDE   (MENU_ITEM_SIDE-INPUT_MARGIN)  
#define INPUT_BOX_HEAD   (MENU_ITEM1_HEAD-INPUT_MARGIN) 
#define INPUT_ELEM_SIDE   MENU_ITEM_SIDE
#define INPUT_ELEM_HEAD   MENU_ITEM1_HEAD
#define INPUT_BOX_WIDTH  (100)
#define INPUT_BOX_HEIGHT (30)
#define INPUT_UNIT_SIDE  (150)
#define INPUT_UNIT_HEAD   MENU_ITEM1_HEAD


/* MENU SCREEN COORDINATIONS */
#define DMENU_CUR_SIDE0   MENU_CUR_SIDE
#define DMENU_CUR_SIDE1  (MENU_CUR_SIDE+130)
#define DMENU_CUR_SIZE    MENU_CUR_SIZE
#define DMENU_ITEM_SIDE0  MENU_ITEM_SIDE
#define DMENU_ITEM_SIDE1 (DMENU_CUR_SIDE1+30)
#define DMENU_ITEM0_HEAD  MENU_ITEM0_HEAD
#define DMENU_ITEM1_HEAD  MENU_ITEM1_HEAD
#define DMENU_ITEM2_HEAD  MENU_ITEM2_HEAD
#define DMENU_ITEM3_HEAD  MENU_ITEM3_HEAD


/* SENSOR SCREEN COORDINATIONS */
#define MON_MARGIN (5)
#define MON_ELEM_SIDE   MENU_ITEM_SIDE
#define MON_ELEM0_HEAD (TITLE_HEAD+50)
#define MON_ELEM1_HEAD (MON_ELEM0_HEAD+50)
#define MON_ELEM2_HEAD (MON_ELEM1_HEAD+50)
#define MON_BOX_SIDE   (MON_ELEM_SIDE-MON_MARGIN)  
#define MON_BOX0_HEAD  (MON_ELEM0_HEAD-MON_MARGIN) 
#define MON_BOX1_HEAD  (MON_ELEM1_HEAD-MON_MARGIN) 
#define MON_BOX2_HEAD  (MON_ELEM2_HEAD-MON_MARGIN) 
#define MON_BOX_WIDTH  (160)
#define MON_BOX_HEIGHT (30)
#define MON_UNIT_SIDE  (200)
#define MON_UNIT0_HEAD  MON_BOX0_HEAD+MON_BOX_HEIGHT/2
#define MON_UNIT1_HEAD  MON_BOX1_HEAD+MON_BOX_HEIGHT/2
#define MON_UNIT2_HEAD  MON_BOX2_HEAD+MON_BOX_HEIGHT/2


/* FFT 2-WAY SCREEN COORDINATION */
#define FFT_MARGIN (1)
#define FFT_GRAPH_WIDTH  (256)
#define FFT_GRAPH_HEIGHT (64)
#define FFT_GRAPH_SIDE    FFT_MARGIN 
#define FFT_GRAPH0_HEAD  (45 + FFT_MARGIN)
#define FFT_GRAPH1_HEAD  (FFT_GRAPH0_HEAD + FFT_GRAPH_HEIGHT + 18)
#define FFT_BOX_SIDE     (FFT_GRAPH_SIDE  - FFT_MARGIN)  
#define FFT_BOX0_HEAD    (FFT_GRAPH0_HEAD - FFT_MARGIN) 
#define FFT_BOX1_HEAD    (FFT_GRAPH1_HEAD - FFT_MARGIN) 
#define FFT_UNIT0_SIDE    TITLE_SIDE
#define FFT_UNIT1_SIDE    FFT_UNIT0_SIDE + 32
#define FFT_UNIT2_SIDE    FFT_UNIT1_SIDE + 32
#define FFT_UNIT3_SIDE    FFT_UNIT2_SIDE + 32
#define FFT_UNIT4_SIDE    FFT_UNIT3_SIDE + 32
#define FFT_UNIT5_SIDE    FFT_UNIT4_SIDE + 32
#define FFT_UNIT6_SIDE    FFT_UNIT5_SIDE + 32
#define FFT_UNIT7_SIDE    FFT_UNIT6_SIDE + 32
#define FFT_UNIT0_HEAD    FFT_GRAPH0_HEAD + FFT_BOX_HEIGHT + 2
#define FFT_UNIT1_HEAD    FFT_GRAPH1_HEAD + FFT_BOX_HEIGHT + 2
#define FFT_VTEXT_MARGIN  (4)

/* FFT and WAV graph realted parameters */  
#define WAV_MAX_AMP       (100)
#define WAV_MIN_AMP       (1)
#define WAV_AMP_STEP      (2)
#define FFT_MAX_AMP       (100)
#define FFT_MIN_AMP       (1)
#define FFT_AMP_STEP      (2)
#define FFT_DBV_RANGE_MAX (12)
#define FFT_DBV_INIT      (0)


/*
#define FFT_MODE_WAV_FFT  (1)
#define FFT_MODE_FFT_FFT  (2)
#define FFT_MODE_WAV_WAV  (3)
*/

#define FRAME_WIDTH  FFT_GRAPH_HEIGHT
#define FRAME_HEIGHT FFT_GRAPH_WIDTH


/* ORBIT SCREEN CORDINATION */
#define ORBIT_MARGIN         (2)
#define ORBIT_GRAPH_XCENTER  (160)
#define ORBIT_GRAPH_YCENTER  (120)
#define ORBIT_GRAPH_RADIUS   (64)
#define ORBIT_GRAPH_SIZE  ORBIT_GRAPH_RADIUS*2
#define ORBIT_SIZE  (ORBIT_GRAPH_RADIUS+ORBIT_MARGIN)*2
#define ORBIT_LIMIT_UM       (10142)
#define ORBIT_TEXT_BOXW      (60)
#define ORBIT_TEXT_BOXH      (10)
#define ORBIT_UNIT_R_SIDE  ORBIT_GRAPH_XCENTER+ORBIT_GRAPH_RADIUS+ORBIT_MARGIN
#define ORBIT_UNIT_L_SIDE  ORBIT_GRAPH_XCENTER-ORBIT_GRAPH_RADIUS-ORBIT_MARGIN-ORBIT_TEXT_BOXW
#define ORBIT_UNIT_U_HEAD  ORBIT_GRAPH_YCENTER-ORBIT_GRAPH_RADIUS-ORBIT_MARGIN-ORBIT_TEXT_BOXH
#define ORBIT_UNIT_L_HEAD  ORBIT_GRAPH_YCENTER+ORBIT_GRAPH_RADIUS+ORBIT_MARGIN
#define ORBIT_MIN_AMP        (1)
#define ORBIT_MAX_AMP        (100)
#define ORBIT_AMP_STEP       (10)

/* MULTICORE MESSAGE ID */
/* 0x10 - 0x70 is reserved for FFT applications */
#define SID_REQ_JSONDOC  (0x01)
#define SID_REQ_MONDATA  (0x02)
#define SID_REQ_WAV_WAV  (0x03)
#define SID_REQ_RAW_FIL  (0x04)
#define SID_REQ_WAV_FFT  (0x10)
#define SID_REQ_FFT_FFT  (0x20)
#define SID_REQ_ORBITDT  (0x30)
#define SID_REQ_ORBITDT  (0x30)

#define APP_ID_MONDATA  SID_REQ_MONDATA
#define APP_ID_WAV_FFT  SID_REQ_WAV_FFT
#define APP_ID_FFT_FFT  SID_REQ_FFT_FFT
#define APP_ID_WAV_WAV  SID_REQ_WAV_WAV
#define APP_ID_RAW_FIL  SID_REQ_RAW_FIL
#define APP_ID_ORBITDT  SID_REQ_ORBITDT


/* FFT WINDOW FUNCTION */
#define FFT_WINDOW_RECTANGULAR  (0x00)
#define FFT_WINDOW_HAMMING      (0x01)
#define FFT_WINDOW_HANNING      (0x02)
#define FFT_WINDOW_FLATTOP      (0x03)


/* PHYSICAL VALUES */
#define WAV_MAX_VOL             (455.)
#define PEAK_TO_PEAK_VOL        (910.)



/* BUTTON PINS */
#define B3 (4)
#define B2 (5)
#define B1 (6)
#define B0 (7)


/* LCD SETTINGS */
#define TFT_DC  (9)
#define TFT_CS (-1)
#define TFT_RST (8)


static Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST); 


struct Response {
  char label[5];
  int value;
  int cur0;
  int cur1;
  int next_id;
};

struct SensorData {
  float acc;
  float vel;
  float dis;
};

struct FftWavData {
  float* pWav;
  float* pFft;
  int len;
  float df;
};

struct FftFftData {
  float* pFft;
  float* pSubFft;
  int len;
  float df;
};

struct WavWavData {
  float* pWav;
  float* pSubWav;
  int len;
  float df;
};

struct OrbitData {
  float acc0;
  float vel0;
  float dis0;
  float acc1;
  float vel1;
  float dis1;
};

/* static memories: needs to be reduced */
static uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT]; 
static uint16_t orbitBuf[ORBIT_SIZE][ORBIT_SIZE]; 
static float    graphDataBuf[FFT_GRAPH_WIDTH];

static int8_t scrType = 0;
static bool   scrChange = false;

static int nextScreen[5] = {-1,-1,-1,-1,-1};
static int backScreen = -1;

static uint8_t  item0_num = 0;
static uint8_t  item1_num = 0;
static uint8_t  inp_num = 0;
static int8_t   cur0 = 0;
static int8_t   cur1 = 0;
static uint16_t inpsel[100];
static bool     plotscale0_done = false;
static bool     plotscale1_done = false;
static int      fftamp0 = FFT_MIN_AMP;
static int      fftamp1 = FFT_MIN_AMP;
static int      wavamp0 = WAV_MIN_AMP;
static int      wavamp1 = WAV_MIN_AMP;
static int      orbitamp = ORBIT_MIN_AMP;
static bool     bLogDisplay = false;
static bool     bdBVDisplay = false;
static int      fmaxdisp = 10000; /* Hz */

static int      dbvdisp0  = 0;
static int      dbvdisp1  = 0;
static int      dbvrange[FFT_DBV_RANGE_MAX][2] = 
                  {{   0, -200}, {   0, -100}, { -50, -150}, {-100, -200}
                 , {   0,  -50}, { -25,  -75}, { -50, -100}, { -75, -125}
                 , {-100, -150}, {-125, -175}, {-150, -200}, {   0, -300}};



/* to avoid conflict between the main loop and interrupt function */
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; 

void error_notifier(int n);

void hardwareSetup();
bool updateScreen();
void stopApplication();
void getResponse(struct Response* res);
void updateResponse(char* label, int value, int cur0, int cur1, int next_id);

int  isAppRunning();
bool isDataReceived();
bool receivedData();
void requestData();

void clearScreen(DynamicJsonDocument* jdoc);
void BuildScreen(DynamicJsonDocument* doc);
void buildTitle(DynamicJsonDocument* doc);
void buildAppSign(DynamicJsonDocument* doc);
void buildInput(DynamicJsonDocument* doc);
void buildMenu(DynamicJsonDocument* doc);
void buildDMenu(DynamicJsonDocument* doc);
void buildMonitor(DynamicJsonDocument* doc);
void build2WayGraph(DynamicJsonDocument* doc);
void buildOrbitGraph(DynamicJsonDocument* doc);
void buildChStatus(DynamicJsonDocument* doc);
void buildButton(DynamicJsonDocument* doc);
void buildNextBackConnection(DynamicJsonDocument* doc);

void appSensorValue(float acc, float vel, float dis);
void appDraw2WayGraph(float* pWav, int len0, float* pFft, int len1, float df);
void appDraw2FftGraph(float* pFft, float* pSubFft, int len, float df);
void appDraw2WavGraph(float* pWav0, float*pSubWav, int len1, float df);
void appDrawOrbitGraph(struct OrbitData* odata);
float get_peak_to_peak(float* pData, int len, float* minValue, float* maxValue);
float get_peak_frequency(float* pData, int len, float delta_f, float* maxValue);

/* utility functions */
void drawWavGraph(float* pWav, int len, float df, int amp, int gskip, int dskip
                , int head, bool scale_update, int color = ILI9341_CYAN);
void drawLinearFftGraph(float* pFft, int len, float df, int amp, int gskip, int dskip
                , int head, bool scale_update, int color = ILI9341_MAGENTA
                , bool peakdisp = true, bool clear = true, bool display = true);
void drawLogFftGraph(float* pFft, int len, float df, int amp, int gskip, int dskip
                , int interval, float f_min_log
                , int head, bool scale_update, int color = ILI9341_MAGENTA
                , bool peakdisp = true, bool clear = true, bool display = true);
void drawDbvFftGraph(float* pFft, int len, float df, int dbvdisp, int gskip, int dskip
                , int interval, float f_min_log
                , int head, bool scale_update, int color = ILI9341_MAGENTA
                , bool peakdisp = true, bool clear = true, bool display = true);




/* helper functions */
void putHorizonLine(int h, int color);
void putItemCursor(int x, int y, int color);
bool putText(int x, int y, String str, int color, int tsize);

void putBufLinearGraph(uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT], float graph[]
                     , float max_vol, int gskip, int x, int y, int w, int h
                     , uint16_t color, float df, int offset = 0
                     , bool clr = true, bool draw = true);
void putBufLogGraph(uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT], float graph[]
                  , float max_vol, int len, int dskip, int x, int y, int w, int h
                  , uint16_t color, float df, int interval, double f_min_log
                  , bool clr = true, bool draw = true, int offset = 0);
void putBufdBVGraph(uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT], float graph[]
                  , int max_dbv, int min_dbv, int len, int dskip, int side, int head, int width, int height
                  , uint16_t color, float df, int interval, double f_min_log
                  , bool clr = true, bool draw = true, int offset = 0);
                  
void plotvirticalscale(int head, int mag, bool ac);
void plotvirticalscale_dbv(int head, int maxdbv, int mindbv);
void plottimescale(float df, int len, int head, bool redraw);
void plotlinearscale(float df, int gskip, int dskip, int head, bool redraw = false);
void plotlogscale(int interval, float df, double f_min_log, int head, bool redraw = false);

void writeLineToBuf(uint16_t fBuf[FRAME_WIDTH][FRAME_HEIGHT], int16_t x0, int16_t y0
                  , int16_t x1, int16_t y1, int16_t color);
void writeOrBitGraphToBuf(uint16_t orbitBuf[ORBIT_SIZE][ORBIT_SIZE]
                    , int16_t x0, int16_t y0, int16_t r, uint16_t color);
#endif /* __APP_SCREEN_HEADER_GURAD__ */
