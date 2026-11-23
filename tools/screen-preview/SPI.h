#pragma once
#include "Arduino.h"
#define FSPI 0
#define HSPI 2
#define VSPI 3
class SPIClass { public: SPIClass(int=0){} void begin(int=-1,int=-1,int=-1,int=-1){} };
inline SPIClass SPI(0);
