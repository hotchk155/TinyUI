////////////////////////////////////////////////////////
//
// T I N Y U I 
//
// Drive a user interface consisting of 
//
// - 4 digit, 7 segment, Common Cathode display
// - 8 LEDs
// - 5 switches
//
// Using seven IO pins
//
////////////////////////////////////////////////////////

#include "Arduino.h"

#define TUI_P_DIGIT0  17
#define TUI_P_DIGIT1  16
#define TUI_P_DIGIT2  15
#define TUI_P_DIGIT3  18
#define TUI_P_DIGIT4  14
#define TUI_P_SHCLK  6
#define TUI_P_SHDAT  7
#define TUI_P_SWREAD 19

enum 
{
  TUI_KEY_0      = 0x0001,
  TUI_KEY_1      = 0x0002,
  TUI_KEY_2      = 0x0004,
  TUI_KEY_3      = 0x0008,
  TUI_KEY_4      = 0x0010,
  TUI_KEY_A      = 0x0020,
  TUI_KEY_B      = 0x0040,
  TUI_KEY_C      = 0x0080,

  TUI_PRESS  = 0x0100,
  TUI_HOLD   = 0x0200,
  TUI_AUTO   = 0x0400,
  TUI_DOUBLE = 0x0800  
};

enum 
{
   TUI_LED_0 = 0x01,
   TUI_LED_1 = 0x02,
   TUI_LED_2 = 0x04,
   TUI_LED_3 = 0x08,
   TUI_LED_4 = 0x10,
   TUI_LED_5 = 0x20,
   TUI_LED_6 = 0x40,
   TUI_LED_7 = 0x80
};

typedef void (*KeypressHandlerFunc)(unsigned int keyStatus);

class CTinyUI
{
public:
  static void init();
  static void setKeypressHandler(KeypressHandlerFunc f); 
  static void setExtraKey(byte key, byte pin);
  static void show(byte seg0=0, byte seg1=0, byte seg2=0, byte seg3=0);
  static void cls() {
    show();
  } 
  static void showNumber(int n, int start=0);
  static void setLEDs(byte which, byte mask=0xff);
  static void clearLEDs(byte which=0xff);
  static void run() { 
    run(millis()); 
  }
  static void run(unsigned long milliseconds);

};



////////////////////////////////////////////////////////
//
// SEVEN SEGMENT LED DISPLAY DEFINITIONS
//
////////////////////////////////////////////////////////

// Map LED segments to shift register bits
//
//  aaa
// f   b
//  ggg
// e   c
//  ddd
#define SEG_A    0x80
#define SEG_B    0x20
#define SEG_C    0x08
#define SEG_D    0x02
#define SEG_E    0x01
#define SEG_F    0x40
#define SEG_G    0x10
#define SEG_DP   0x04



// Define the LED segment patterns for digits
#define DGT_0 (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F)
#define DGT_1 (SEG_B|SEG_C)
#define DGT_2 (SEG_A|SEG_B|SEG_D|SEG_E|SEG_G)
#define DGT_3 (SEG_A|SEG_B|SEG_C|SEG_D|SEG_G)
#define DGT_4 (SEG_B|SEG_C|SEG_F|SEG_G)
#define DGT_5 (SEG_A|SEG_C|SEG_D|SEG_F|SEG_G)
#define DGT_6 (SEG_A|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_7 (SEG_A|SEG_B|SEG_C)
#define DGT_8 (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_9 (SEG_A|SEG_B|SEG_C|SEG_F|SEG_G)
#define DGT_A (SEG_A|SEG_B|SEG_C|SEG_E|SEG_F|SEG_G)
#define DGT_B (SEG_C|SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_C (SEG_A|SEG_D|SEG_E|SEG_F)
#define DGT_D (SEG_B|SEG_C|SEG_D|SEG_E|SEG_G)
#define DGT_E (SEG_A|SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_F (SEG_A|SEG_E|SEG_F|SEG_G)
#define DGT_G (SEG_A|SEG_C|SEG_D|SEG_E|SEG_F)
#define DGT_H (SEG_B|SEG_C|SEG_E|SEG_F|SEG_G)
#define DGT_I (SEG_B|SEG_C)
#define DGT_J (SEG_B|SEG_C|SEG_D)
#define DGT_K (SEG_A|SEG_B|SEG_E|SEG_F|SEG_G)
#define DGT_L (SEG_D|SEG_E|SEG_F)
#define DGT_M (SEG_A|SEG_B|SEG_C|SEG_E|SEG_F)
#define DGT_N (SEG_C|SEG_E|SEG_G)
#define DGT_O (SEG_C|SEG_D|SEG_E|SEG_G)
#define DGT_P (SEG_A|SEG_B|SEG_E|SEG_F|SEG_G)
#define DGT_Q (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E) 
#define DGT_R (SEG_E|SEG_G)
#define DGT_S (SEG_A|SEG_C|SEG_D|SEG_F|SEG_G)
#define DGT_T (SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_U (SEG_C|SEG_D|SEG_E)
#define DGT_V (SEG_C|SEG_D|SEG_E)
#define DGT_W (SEG_A|SEG_C|SEG_D|SEG_E)
#define DGT_X (SEG_D|SEG_G)
#define DGT_Y (SEG_B|SEG_C|SEG_D|SEG_F|SEG_G)
#define DGT_Z (SEG_A|SEG_D|SEG_E|SEG_G)
#define DGT_DASH (SEG_G)



extern CTinyUI TUI;



