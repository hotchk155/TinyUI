////////////////////////////////////////////////////////
#include "PortIO.h"
#include "TinyUI.h"

#define UI_MAXLEDARRAY  5         // number of LED arrays
#define UI_NUM_SWITCHES 6
#define UI_DEBOUNCE_MS 20         // key debounce
#define UI_AUTO_REPEAT_DELAY 500  // delay before key auto repeats
#define UI_AUTO_REPEAT_PERIOD 50  // delay between auto repeats
#define UI_DOUBLE_CLICK_TIME 200  // double click threshold


static byte uiLEDState[UI_MAXLEDARRAY];       // the output bit patterns for the LEDs
static byte uiDebounceCount[UI_NUM_SWITCHES];  // debounce registers for switches
static byte uiKeyStatus;              // input status for switches (The one other modules should read)
static byte uiLastKeyStatus;          
static byte uiLastKeypress;           
static unsigned long uiDoubleClickTime; 
static unsigned long uiAutoRepeatTime; 
static byte uiLongPress;
static byte uiLEDIndex;                   
static byte uiSwitchStates;
static byte uiLastSwitchStates;              
static byte uiKeyAPin;
static byte uiKeyBPin;
static byte uiKeyCPin;
static KeypressHandlerFunc uiKeypressHandler; 

////////////////////////////////////////////////////////
// Initialise variables
void CTinyUI::init()
{  
  pinMode(TUI_P_DIGIT0, OUTPUT);
  pinMode(TUI_P_DIGIT1, OUTPUT);
  pinMode(TUI_P_DIGIT2, OUTPUT);
  pinMode(TUI_P_DIGIT3, OUTPUT);
  pinMode(TUI_P_DIGIT4, OUTPUT);
  pinMode(TUI_P_SHCLK, OUTPUT);
  pinMode(TUI_P_SHDAT, OUTPUT);
  pinMode(TUI_P_SWREAD, INPUT);

  digitalWrite(TUI_P_DIGIT0, LOW);
  digitalWrite(TUI_P_DIGIT1, LOW);
  digitalWrite(TUI_P_DIGIT2, LOW);
  digitalWrite(TUI_P_DIGIT3, LOW);
  digitalWrite(TUI_P_DIGIT4, LOW);
  digitalWrite(TUI_P_SHCLK, LOW);
  digitalWrite(TUI_P_SHDAT, LOW);
  
  memset(uiLEDState, 0, sizeof(uiLEDState));
  memset(uiDebounceCount, 0, sizeof(uiDebounceCount));
  uiKeyStatus = 0;
  uiLEDIndex = 0;
  uiSwitchStates = 0;
  uiLastSwitchStates = 0;
  uiAutoRepeatTime = 0;
  uiKeyAPin = 0;
  uiKeyBPin = 0;
  uiKeyCPin = 0;
  uiKeypressHandler = NULL;
  uiLongPress = 0;
  
  // start the interrupt to service the UI   
  TCCR2A = 0;
  TCCR2B = 1<<CS22 | 0<<CS21 | 1<<CS20; // control refresh rate of display
  TIMSK2 = 1<<TOIE2;
  TCNT2 = 0; 
}

////////////////////////////////////////////////////////
void CTinyUI::setKeypressHandler(KeypressHandlerFunc f)
{
  uiKeypressHandler = f;
}

////////////////////////////////////////////////////////
void CTinyUI::setLEDs(byte which, byte mask)
{
  uiLEDState[4] &= ~mask;
  uiLEDState[4] |= which;
}

////////////////////////////////////////////////////////
void CTinyUI::clearLEDs(byte which)
{
  uiLEDState[4] &= ~which;
}

////////////////////////////////////////////////////////
// Configure the LEDs to show a decimal number
void CTinyUI::showNumber(int n, int start)
{  
  byte xlatDigit[10]  = {  
    DGT_0,  DGT_1,  DGT_2,  DGT_3,  DGT_4,  DGT_5,  DGT_6,  DGT_7,  DGT_8,  DGT_9   };
  int div[4] = {
    1000, 100, 10, 1  };
  if(start < 0 || start >= 4)
  {
    show(SEG_DP, SEG_DP, SEG_DP, SEG_DP);
  }
  else
  {
    while(start < 4)
    {
      int divider = div[start];
      uiLEDState[start] = xlatDigit[n/divider]; 
      n%=divider;
      ++start;
    }
  }
}

////////////////////////////////////////////////////////
void checKeyPin(byte pin, byte key)
{
    if(!digitalRead(pin))
      uiSwitchStates |= key;
    else  
      uiSwitchStates &= ~key;
}

////////////////////////////////////////////////////////
// Manage UI functions
void CTinyUI::run(unsigned long milliseconds)
{
  if(uiKeyAPin) checKeyPin(uiKeyAPin, TUI_KEY_A);
  if(uiKeyBPin) checKeyPin(uiKeyBPin, TUI_KEY_B);
  if(uiKeyCPin) checKeyPin(uiKeyCPin, TUI_KEY_C);

  // Manage switch debouncing...
  byte newKeyPress = 0;
  for(int i=0; i<UI_NUM_SWITCHES; ++i)
  {    
    byte mask = 1<<i;

    // are we debouncing?
    if(uiDebounceCount[i])
    {
      --uiDebounceCount[i];
    }
    else
    {      
      if((uiSwitchStates & mask) && !(uiKeyStatus & mask)) // key pressed now, was not before
      {
        uiKeyStatus |= mask;
        uiDebounceCount[i] = UI_DEBOUNCE_MS;        
        newKeyPress = 1;
      }
      else if(!(uiSwitchStates & mask) && (uiKeyStatus & mask)) // key pressed before, not now
      {
        uiKeyStatus &= ~mask;
        uiDebounceCount[i] = UI_DEBOUNCE_MS;
      }
    }
  }

  unsigned int flags = 0;
  
  // Manage auto repeat etc
  if(!uiKeyStatus) // no keys pressed
  {
    uiLongPress = 0;
    uiKeyStatus = 0;
    uiAutoRepeatTime = 0; 
  }
  else if(uiKeyStatus != uiLastKeyStatus) // change in keypress
  {
    uiLongPress = 0;
    if(newKeyPress)
    {
      if(uiKeyStatus == uiLastKeypress && milliseconds < uiDoubleClickTime)
        flags = TUI_DOUBLE;
      else
        flags = TUI_PRESS;
      uiLastKeypress = uiKeyStatus;
      uiDoubleClickTime = milliseconds + UI_DOUBLE_CLICK_TIME;
    }
    uiAutoRepeatTime = milliseconds + UI_AUTO_REPEAT_DELAY;
  }
  else 
  {
    // keys held - not a new press
    if(uiAutoRepeatTime < milliseconds)
    {
      if(uiLongPress)  // key flagged as held?
      {
        flags = TUI_AUTO;  // now it is an auto repeat
      }
      else
      {
        uiLongPress = 1;  // otherwise flag as held
        flags = TUI_HOLD;
      }
      uiAutoRepeatTime = milliseconds + UI_AUTO_REPEAT_PERIOD;
    }
  }
  if(flags && uiKeypressHandler) 
      uiKeypressHandler(flags | uiKeyStatus);
  uiLastKeyStatus = uiKeyStatus;  
}






////////////////////////////////////////////////////////
void CTinyUI::setExtraKey(byte key, byte pin)
{
  switch(key)
  {
    case TUI_KEY_A: uiKeyAPin = pin; break;
    case TUI_KEY_B: uiKeyBPin = pin; break;
    case TUI_KEY_C: uiKeyCPin = pin; break;    
  }
  
}

////////////////////////////////////////////////////////
void CTinyUI::show(byte seg0, byte seg1, byte seg2, byte seg3)
{
  uiLEDState[0] = seg0;
  uiLEDState[1] = seg1;
  uiLEDState[2] = seg2;
  uiLEDState[3] = seg3;
}

////////////////////////////////////////////////////////
// Interrupt service routing that refreshes the LEDs
ISR(TIMER2_OVF_vect) 
{
  // Read the switch status (do it now, rather than on previous
  // tick so we can ensure adequate setting time)
  if(_DIGITAL_READ(TUI_P_SWREAD))
    uiSwitchStates |= (1<<uiLEDIndex);
  else
    uiSwitchStates &= ~(1<<uiLEDIndex);

  // Turn off all displays  
  _DIGITAL_WRITE(TUI_P_DIGIT0,0);
  _DIGITAL_WRITE(TUI_P_DIGIT1,0);
  _DIGITAL_WRITE(TUI_P_DIGIT2,0);
  _DIGITAL_WRITE(TUI_P_DIGIT3,0);
  _DIGITAL_WRITE(TUI_P_DIGIT4,0);

  // 0x80 A
  // 0x20 B
  // 0x08 C
  // 0x02 D
  // 0x01 E
  // 0x40 F
  // 0x10 G
  // 0x04 DP

  // Unrolled loop to load shift register
  byte d = uiLEDState[uiLEDIndex];
  // bit 7    
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHDAT,(d & 0x80));
  _DIGITAL_WRITE(TUI_P_SHCLK,1);
  // bit 6
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHDAT,(d & 0x40));
  _DIGITAL_WRITE(TUI_P_SHCLK,1);
  // bit 5    
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHDAT,(d & 0x20));
  _DIGITAL_WRITE(TUI_P_SHCLK,1);
  // bit 4    
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHDAT,(d & 0x10));
  _DIGITAL_WRITE(TUI_P_SHCLK,1);
  // bit 3    
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHDAT,(d & 0x08));
  _DIGITAL_WRITE(TUI_P_SHCLK,1);
  // bit 2    
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHDAT,(d & 0x04));
  _DIGITAL_WRITE(TUI_P_SHCLK,1);
  // bit 1    
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHDAT,(d & 0x02));
  _DIGITAL_WRITE(TUI_P_SHCLK,1);
  // bit 0    
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHDAT,(d & 0x01));
  _DIGITAL_WRITE(TUI_P_SHCLK,1);
  // flush    
  _DIGITAL_WRITE(TUI_P_SHCLK,0);
  _DIGITAL_WRITE(TUI_P_SHCLK,1);

  // Turn on the appropriate display
  switch(uiLEDIndex)
  {
  case 0: 
    _DIGITAL_WRITE(TUI_P_DIGIT0,1);
    break;
  case 1: 
    _DIGITAL_WRITE(TUI_P_DIGIT1,1);
    break;
  case 2: 
    _DIGITAL_WRITE(TUI_P_DIGIT2,1);
    break;
  case 3: 
    _DIGITAL_WRITE(TUI_P_DIGIT3,1);
    break;
  case 4: 
    _DIGITAL_WRITE(TUI_P_DIGIT4,1);
    break;
  }

  // Next pass we'll check the next display
  if(++uiLEDIndex >= UI_MAXLEDARRAY)
    uiLEDIndex = 0;
}

////////////////////////////////////////////////////////
CTinyUI TUI;

