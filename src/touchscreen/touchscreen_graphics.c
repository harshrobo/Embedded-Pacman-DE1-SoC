PART 1

#include "LT24_graphics.h" 
#include "DE1SoC_LT24/DE1SoC_LT24.h" 
#include "BasicFont/BasicFont.h" 
#include <string.h> 
 
HpsErr_t LT24_clearScreen(LT24Ctx_t* ctx)  
{ return LT24_clearDisplay(ctx, LT24_BLACK); // Clear to black } 
void LT24_drawChar(LT24Ctx_t* ctx, uint16_t x, uint16_t y, char c, uint16_t fgColour, uint16_t 
bgColour) { 
if (c < ' ' || c > '~') return; // Skip unsupported characters const uint8_t* charData = 
(uint8_t*)BF_fontMap[c - ' ']; 
for (int col = 0; col < 5; col++) { 
    uint8_t bits = charData[col]; 
    for (int row = 0; row < 8; row++) { 
        uint16_t colour = (bits & (1 << row)) ? fgColour : bgColour; 
        LT24_drawPixel(ctx, colour, x + col, y + row); 
    } 
} 
 
// Add 1-pixel spacing 
for (int row = 0; row < 8; row++) { 
    LT24_drawPixel(ctx, bgColour, x + 5, y + row); 
 
35 
 
} 
  
} 
void LT24_drawString(LT24Ctx_t* ctx, uint16_t x, uint16_t y, const char* str, uint16_t fgColour, 
uint16_t bgColour) { 
while (*str) { 
LT24_drawChar(ctx, x, y, *str, fgColour, bgColour); x += 6; // 5 pixel wide character + 1 spacing 
str++; 
} 
} 
void LT24_drawRect(LT24Ctx_t* ctx, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t 
colour) { 
for (uint16_t i = 0; i < w; i++) { 
LT24_drawPixel(ctx, colour, x + i, y); 
LT24_drawPixel(ctx, colour, x + i, y + h - 1); 
} for (uint16_t j = 0; j < h; j++) { 
LT24_drawPixel(ctx, colour, x, y + j); 
LT24_drawPixel(ctx, colour, x + w - 1, y + j); 
} 
} 
void LT24_fillRect(LT24Ctx_t* ctx, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t 
colour) { 
for (uint16_t j = 0; j < h; j++) { 
for (uint16_t i = 0; i < w; i++) { 
LT24_drawPixel(ctx, colour, x + i, y + j); 
} 
} 
} 


PART 2 

#include "Touchscreen/Touchscreen.h" 
#include "DE1SoC_Addresses/DE1SoC_Addresses.h" 
#include "Util/watchdog.h" 
#include "Util/delay.h" 
#include "Util/bit_helpers.h" 
#include <stdio.h> 
#include <stdint.h> 
#include <stdbool.h> 
// Debugging function for error checking 
void exitOnFail(HpsErr_t status, const char* msg) { 
if (ERR_IS_ERROR(status)) { 
printf("Error: %s (Code: %d)\n", msg, status); 
while (1); // Halt execution for debugging 
} 
} 
// Main function 
int main(void) { 
// Variables 
FPGAPIOCtx_t* pioContext = NULL; 
TouchscreenCtx_t tsCtx = {0}; 
uint16_t x, y; 
int touch_count = 0; 
// Print base addresses for debugging 
printf("Starting program...\n"); 
printf("GPIO Base Address: %p\n", (void *)LSC_BASE_GPIO_JP1); 
 
// Initialise GPIO 
printf("Initializing GPIO...\n"); 
HpsErr_t gpioStatus = FPGA_PIO_initialise((void *)LSC_BASE_GPIO_JP1, LSC_CONFIG_GPIO, 
&pioContext); 
if (ERR_IS_ERROR(gpioStatus) || pioContext == NULL) { 
    printf("GPIO initialisation failed (Code: %d, Context: %p)\n", gpioStatus, (void 
*)pioContext); 
    while (1); 
} 
printf("GPIO initialised successfully. Context: %p\n", (void *)pioContext); 
 
// Check initial GPIO states repeatedly 
unsigned int pins; 
printf("[DEBUG] Checking initial GPIO states...\n"); 
for (int i = 0; i < 5; i++) { 
    FPGA_PIO_getInput(pioContext, &pins, ADC_DOUT | ADC_PENIRQ); 
    printf("[DEBUG] Initial GPIO %d: DOUT=0x%08X, PENIRQ=0x%08X\n", i, pins & ADC_DOUT, pins & 
ADC_PENIRQ); 
    usleep(100000); 
} 
 
// Initialise Touchscreen 
printf("Initializing Touchscreen ADC...\n"); 
if (!TouchscreenADC_initialise(&tsCtx, pioContext)) { 
    printf("Touchscreen ADC initialisation failed\n"); 
    while (1); 
} 
printf("Touchscreen ADC initialised successfully\n"); 
 
// Test touchscreen 
printf("Testing touchscreen (console output)...\n"); 
while (touch_count < 20) { // Limit to 20 iterations 
    FPGA_PIO_getInput(pioContext, &pins, ADC_PENIRQ); 
    printf("[DEBUG] Raw PENIRQ pin state: 0x%08X\n", pins); 
 
    if (TouchscreenADC_readXY(&tsCtx, &x, &y)) { 
        uint16_t screenX = (x * 320) / 4095; 
        uint16_t screenY = (y * 240) / 4095; 
        printf("Touch %d detected - Raw X:%u, Y:%u | Screen X:%u, Y:%u\n", touch_count + 1, x, y, 
screenX, screenY); 
        touch_count++; 
    } else { 
        printf("No touch detected\n"); 
    } 
    usleep(1000000); // 1s delay 
} 
 
printf("Test complete. Total touches detected: %d\n", touch_count); 
return 0; 
  
} 
