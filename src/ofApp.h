#pragma once

#include <regex>
#include "ofMain.h"
#include "ofxTrueTypeFontUC.h"
#include "ofxOsc.h"
#include "ofxAruco.h"
#include "highlyreliablemarkers.h"
#include "ofxZxing.h"
#include "ofxJoystick.h"
#include "ofxXmlSettings.h"

/* ---------- definitions ---------- */

// system
#define APP_VER         "v0.0.2 alpha1"

#define DEBUG_ENABLED   false
#define HELP_LINES      35  // must be <= OVLTXT_LINES
#define SCENE_INIT      0
#define SCENE_CAMS      1
#define SCENE_MAIN      2
#ifdef TARGET_OSX
#define TVP_KEY_ALT     OF_KEY_COMMAND
#define TVP_STR_ALT     "command"
#else /* TARGET_OSX */
#define TVP_KEY_ALT     OF_KEY_ALT
#define TVP_STR_ALT     "Alt"
#endif /* TARGET_WIN32 TARGET_LINUX */
#define DFLT_SYS_STAT   false
#define TVP_VAL_PLUS    1
#define TVP_VAL_MINUS   -1

// settings
#define SETTINGS_FILE   "settings.xml"
#define SNM_SYS_STAT    "system:sysStat"
#define SNM_DTCTALL_FRM "system:allFrameDetect"
#define TVP_COMPORT     "system:tvpComport"
#define SNM_VIEW_FLLSCR "view:fullscreen"
#define SNM_VIEW_CAMTRM "view:camTrim"
#define SNM_VIEW_CAMFRM "view:camFrame"

// pilots
#define PILOTS_FILE     "pilots/pilots.xml"
#define PLT_PILOT_LABEL "pilot:label_"

// camera profile
#define CAM_FPV_FILE    "camera/fpv.xml"
#define CFNM_NAME       "camera:name"
#define CFNM_CAMNUM     "camera:camnum"
#define CFNM_GRAB_W     "camera:grab:width"
#define CFNM_GRAB_H     "camera:grab:height"
#define CFNM_CROP_X     "camera:crop:x"
#define CFNM_CROP_Y     "camera:crop:y"
#define CFNM_CROP_W     "camera:crop:width"
#define CFNM_CROP_H     "camera:crop:height"
#define CFNM_DRAW_ASPR  "camera:draw:aspectRatio"

// color
#define COLOR_YELLOW    255,215,0
#define COLOR_WHITE     255,255,255
#define COLOR_LGRAY     127,127,127
#define COLOR_DGRAY     15
#define COLOR_BG_DARK   0,0,0,223
#define COLOR_BG_MIDDLE 0,0,0,127
#define COLOR_BG_LIGHT  0,0,0,31
#define COLOR_ALERT     255,0,0
// view
#define FRAME_RATE      60
#define MOVE_STEPS      10
#define VERTICAL_SYNC   true
#define LOGO_LARGE_FILE "system/logo_large.png"
#define LOGO_SMALL_FILE "system/logo_small.png"
#define DFLT_WALL_FILE  "system/background.png"
#define DFLT_ICON_FILE  "system/pilot_icon.png"
#define CAMERA_MAXNUM   4
#define CAMERA_WIDTH    640
#define CAMERA_HEIGHT   480
#define CAMERA_RATIO    1.3333
#define FONT_P_FILE     "system/GenShinGothic-P-Bold.ttf"
#define FONT_M_FILE     "system/GenShinGothic-Monospace-Bold.ttf"
#define ICON_DIR        "pilots/"
#define ICON_WIDTH      50
#define ICON_HEIGHT     50
#define ICON_MARGIN_X   20
#define ICON_MARGIN_Y   0
#define NUMBER_HEIGHT   20
#define NUMBER_MARGIN_X 1
#define NUMBER_MARGIN_Y 35
#define LABEL_HEIGHT    30
#define LABEL_MARGIN_X  75
#define LABEL_MARGIN_Y  40
#define BASE_MARGIN_X   0
#define BASE_MARGIN_Y   0
#define BASE_WIDTH      20
#define BASE_HEIGHT     50
#define BASE_1_COLOR    201,58,64
#define BASE_2_COLOR    160,194,56
#define BASE_3_COLOR    0,116,191
#define BASE_4_COLOR    248,128,23
#define LAP_HEIGHT      20
#define LAP_MARGIN_X    20
#define LAP_MARGIN_Y    80
#define LAPHIST_MD_OFF  0
#define LAPHIST_MD_IN   1
#define LAPHIST_MD_OUT  2
#define LAPHIST_HEIGHT  15
#define LAPHIST_MARGIN  8
#define FRAME_LINEWIDTH 8
#define DFLT_FSCR_ENBLD false
#define DFLT_CAM_TRIM   false
#define DFLT_CAM_LAPHST LAPHIST_MD_OFF
#define DFLT_CAM_FRAMED false
#define ALIGN_LEFT      0
#define ALIGN_CENTER    1
#define ALIGN_RIGHT     2
#define HIDECUR_TIME    FRAME_RATE
// overlay
#define OVLMODE_NONE    0
#define OVLMODE_HELP    1
#define OVLMODE_MSG     2
#define OVLMODE_RCRSLT  3
#define OVLTXT_BLKS     13
#define OVLTXT_LINES    35
#define OVLTXT_LAPS     25
#define OVLTXT_MARG     10
#define OLVMSG_TIME     FRAME_RATE
#define INFO_HEIGHT     10
#define WATCH_HEIGHT    40
#define WATCH_OFFSET_Y  10
// sound
#define SND_BEEP_FILE   "system/beep.wav"
#define SND_BEEP3_FILE  "system/beep3.wav"
#define SND_COUNT_FILE  "system/count.wav"
#define SND_FINISH_FILE "system/finish.wav"
#define SND_NOTIFY_FILE "system/notify.wav"
#define SND_CANCEL_FILE "system/cancel.wav"
// AR lap timer
#define ARAP_MODE_NORM  0
#define ARAP_MODE_LOOSE 1
#define ARAP_MODE_OFF   2
#define DFLT_ARAP_MODE  ARAP_MODE_NORM
#define DFLT_ARAP_RLAPS 10
#define DFLT_ARAP_RSECS 0
#define DFLT_ARAP_MNLAP 3
#define DFLT_ARAP_SGATE false
#define DFLT_ARAP_LAPTO false
#define ARAP_MKR_FILE   "system/marker.xml"
#define ARAP_RESULT_DIR "results/"
#define ARAP_MNUM_THR   2
#define ARAP_MAX_RLAPS  10000
#define ARAP_MAX_MNLAP  100
#define ARAP_MAX_RSECS  36000
#define ARAP_RSLT_SCRN  0
#define ARAP_RSLT_FILE  1
#define ARAP_RSLT_DELAY (FRAME_RATE * 3)
#define ARAP_RECT_LINEW 5
#define WATCH_COUNT_SEC 5
#define DTCT_ALL_FRAME  false

/* ---------- classes ---------- */

class tvpCamView {
public:
    // camera
    int moveSteps;
    int width;
    int height;
    int heightWide;
    int widthTarget;
    int heightTarget;
    int heightWideTarget;
    int posX;
    int posY;
    int posYWide;
    int posXTarget;
    int posYTarget;
    int posYWideTarget;
    int grabW;
    int grabH;
    int cropX;
    int cropY;
    int cropW;
    int cropH;
    float imageScale;
    bool needCrop;
    bool needResize;
    bool isWide;
    ofPixels resizedPixels;
    ofImage resizedImage;

    // lap
    int lapPosX;
    int lapPosY;
    int lapPosXTarget;
    int lapPosYTarget;
    // AR lap timer
    ofxAruco aruco;
    int foundMarkerNum;
    int foundValidMarkerNum;
    bool enoughMarkers;
    int flickerCount;
    int flickerValidCount;
    //For RotorHazard
    uint8_t markerDetectStrengthLast;
    uint8_t markerOutput;
    bool rssiOutput;
    float markerEndTime;
};


class tvpCamProf {
public:
    bool enabled;
    int camnum;
    string name;
    int grabW;
    int grabH;
    int cropX;
    int cropY;
    int cropW;
    int cropH;
    string drawAspr;
    bool needCrop;
    bool needResize;
    bool isWide;
};

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void exit();
};

// -- splash --
void setupInit();
void loadSettingsFile();
void saveSettingsFile();
void loadCameraProfileFile();
void updateInit();
void drawInit();
// -- camera setup --
void setupCamCheck();
void updateCamCheck();
void drawCamCheck();
void keyPressedCamCheck(int);
void reloadCameras();
// -- main --
// common
void initConfig();
void toggleSysStat();
// view
void grabberUpdateResize(int);
void grabberUpdateResizeMulti();
void toggleFullscreen();
void setupColors();
void setViewParams();
int calcViewParam(int, int, int);
void updateViewParams();

// draw
void drawCameraImage(int);
void drawCameraARMarker(int, bool);
void drawCamera(int);
void drawInfo();
void drawStringWithShadow(ofxTrueTypeFontUC*, ofColor, ofColor, string, int, int);
// input
void keyPressedOverlayHelp(int);
void keyPressedOverlayHelp(int);
void keyPressedOverlayNone(int);
// race
void initRaceVars();
void startRace();

// overlay - common
void setOverlayMode(int);
void loadOverlayFont();
void drawStringBlock(ofxTrueTypeFontUC*, string, int , int, int, int, int);
void drawLineBlock(int, int, int, int, int);
void drawULineBlock(int, int, int, int, int);
// overlay - help
void drawHelp();
void drawHelpBody(int);
// overlay - message
void initOverlayMessage();
void setOverlayMessage(string);
void drawOverlayMessageCore(ofxTrueTypeFontUC*, string);
void drawOverlayMessage();

// others
void activateCursor();
