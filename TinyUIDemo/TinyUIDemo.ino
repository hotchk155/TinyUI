#include "TinyUI.h"

void setup(void)
{
  TUI.init();
}

int i=0;
void loop(void)
{
  TUI.showNumber(i++);
  delay(100);
}

