#ifndef __APP_SCREEN_HEADER_GURAD__
#define __APP_SCREEN_HEADER_GURAD__

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ArduinoJson.h>
#include <MP.h>


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
#define MON_BOX_WIDTH  (100)
#define MON_BOX_HEIGHT (30)
#define MON_UNIT_SIDE  (150)
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


/* FFT and WAV graph realted parameters */  
#define WAV_MAX_AMP   2000
#define WAV_MIN_AMP   100
#define WAV_AMP_STEP  100
#define FFT_MAX_AMP   1000
#define FFT_MIN_AMP   1
#define FFT_AMP_STEP  1

#define FFT_MODE_WAV_FFT  (1)
#define FFT_MODE_FFT_FFT  (2)
#define FFT_MODE_WAV_WAV  (3)

#define FRAME_WIDTH  FFT_GRAPH_HEIGHT
#define FRAME_HEIGHT FFT_GRAPH_WIDTH


/* ORBIT SCREEN CORDINATION */
#define ORBIT_MARGIN         (2)
#define ORBIT_GRAPH_XCENTER  (160)
#define ORBIT_GRAPH_YCENTER  (120)
#define ORBIT_GRAPH_RADIUS   (64)
#define ORBIT_GRAPH_SIZE  ORBIT_GRAPH_RADIUS*2
#define ORBIT_SIZE  (ORBIT_GRAPH_RADIUS+ORBIT_MARGIN)*2


/* MULTICORE MESSAGE ID */
#define SID_DOC_MASK (0x01)
#define SID_DAT_MASK (0x02)
#define SID_WAV_MASK (0x04)
#define SID_FFT_MASK (0x08)


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

static uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT]; 
static uint16_t orbitBuf[ORBIT_SIZE][ORBIT_SIZE]; 

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
static int      graphDataBuf[FFT_GRAPH_WIDTH];
static bool     plotscale0_done = false;
static bool     plotscale1_done = false;
static int      fftamp0 = FFT_MIN_AMP;
static int      fftamp1 = FFT_MIN_AMP;
static int      wavamp0 = WAV_MIN_AMP;
static int      wavamp1 = WAV_MIN_AMP;
static bool     bLogDisplay = true;


void error_notifier(int n);

void hardwareSetup();
bool updateScreen();
void stopApplication();
void getResponse(struct Response *res);
void updateResponse(char* label, int value, int cur0, int cur1, int next_id);

bool isSensorAppRunning();
bool isSensorDataReceived();
bool receivedSensorData();
void requestSensorData();

bool isFftWavAppRunning();
bool isFftWavDataReceived();
bool receivedFftWavData();
void requestFftWavData();

bool isFftFftAppRunning();
bool isFftFftDataReceived();
bool receivedFftFftData();
void requestFftFftData();

bool isOrbitAppRunning();
bool isOrbitDataReceived();
bool receivedOrbitData();
void requestOrbitData();


void clearScreen(DynamicJsonDocument *jdoc);
void BuildScreen(DynamicJsonDocument *doc);
void buildTitle(DynamicJsonDocument *doc);
void buildAppSign(DynamicJsonDocument *doc);
void buildInput(DynamicJsonDocument *doc);
void buildMenu(DynamicJsonDocument *doc);
void buildDMenu(DynamicJsonDocument *doc);
void buildMonitor(DynamicJsonDocument *doc);
void build2WayGraph(DynamicJsonDocument *doc);
void buildOrbitGraph(DynamicJsonDocument *doc);
void buildChStatus(DynamicJsonDocument *doc);
void buildButton(DynamicJsonDocument *doc);
void buildNextBackConnection(DynamicJsonDocument *doc);

void putSensorValue(float acc, float vel, float dis);
void putDraw2WayGraph(float *pWav0, int len0, float *pFft1, int len1, float df);
void putDraw2FftGraph(float *pFft, float* pSubFft, int len, float df);
void putDrawOrbitGraph(float *pWav, float *pSubWav, int len, float df);
void putBufLinearGraph(uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT], int graph[]
                     , int gskip, int x, int y, int w, int h
                     , uint16_t color, float df, int offset = 0
                     , bool clr = true, bool draw = true);
void putBufLogGraph(uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT], int graph[]
                  , int len, int dskip, int x, int y, int w, int h
                  , uint16_t color, float df, int interval, double f_min_log
                  , bool clr = true, bool draw = true, int offset = 0);
void plottimescale(float df, int len, int head, bool redraw);
void plotlinearscale(float df, int gskip, int dskip, int head, bool redraw = false);
void plotlogscale(int interval, float df, double f_min_log, int head, bool redraw = false);

void putHorizonLine(int h, int color);
void putItemCursor(int x, int y, int color);
bool putText(int x, int y, String str, int color, int tsize);

void writeLineToBuf(uint16_t fBuf[FRAME_WIDTH][FRAME_HEIGHT], int16_t x0, int16_t y0
                  , int16_t x1, int16_t y1, int16_t color);
void writeOrBitGraphToBuf(uint16_t orbitBuf[ORBIT_SIZE][ORBIT_SIZE]
                    , int16_t x0, int16_t y0, int16_t r, uint16_t color);
#endif /* __APP_SCREEN_HEADER_GURAD__ */
