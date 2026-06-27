#include "Touchscreen.h" #include "Util/watchdog.h" #include "Util/delay.h" #include 
"Util/bit_helpers.h" #include "DE1SoC_Addresses/DE1SoC_Addresses.h" #include <stdio.h> #include 
<stdarg.h> 
// GPIO pin definitions for ADC communication #define ADC_CS (1 << 24) #define ADC_CLK (1 << 23) 
#define ADC_DIN (1 << 25) #define ADC_DOUT (1 << 27) #define ADC_PENIRQ (1 << 28) 
static void pulseClock(FPGAPIOCtx_t* pio) { FPGA_PIO_setOutput(pio, ADC_CLK, ADC_CLK); // Set CLK 
High usleep(5); // 5us for ~100 kHz clock FPGA_PIO_setOutput(pio, 0, ADC_CLK); // Set CLK Low 
usleep(5); } 
static uint16_t readADC(FPGAPIOCtx_t* pio, uint8_t command) { uint16_t result = 0; 
FPGA_PIO_setOutput(pio, 0, ADC_CS); // CS low 
printf("[DEBUG] Sending ADC Command: 0x%02X\n", command); 
 
// Send command (8 bits, MSB first) 
for (int i = 7; i >= 0; i--) { 
    FPGA_PIO_setOutput(pio, (command >> i) & 1 ? ADC_DIN : 0, ADC_DIN); 
    pulseClock(pio); 
} 
 
// Dummy clock cycle 
pulseClock(pio); 
 
// Skip null bit 
pulseClock(pio); 
 
// Read 12 bits 
for (int i = 11; i >= 0; i--) { 
    pulseClock(pio); 
    unsigned int pins; 
    FPGA_PIO_getInput(pio, &pins, ADC_DOUT); 
    printf("[DEBUG] DOUT pins: 0x%08X (Bit %d: %d)\n", pins, i, (pins & ADC_DOUT) ? 1 : 0); 
    if (pins & ADC_DOUT) { 
        result |= (1 << i); 
    } 
} 
 
FPGA_PIO_setOutput(pio, ADC_CS, ADC_CS); // CS high 
 
printf("[DEBUG] ADC Result: 0x%03X (%d)\n", result, result); 
return result; 
  
} 
bool TouchscreenADC_initialise(TouchscreenCtx_t* ctx, FPGAPIOCtx_t* pio) { if 
(!FPGA_PIO_isInitialised(pio)) { printf("[DEBUG] FPGA PIO not initialised\n"); return false; } 
ctx->gpio = pio; 
 
// Set GPIO directions 
FPGA_PIO_setDirection(pio, ADC_CS | ADC_CLK | ADC_DIN, ADC_CS | ADC_CLK | ADC_DIN);  // Outputs 
FPGA_PIO_setDirection(pio, ADC_DOUT | ADC_PENIRQ, 0);   // Inputs 
 
// Initialise pin states 
FPGA_PIO_setOutput(pio, ADC_CS, ADC_CS);              // CS High 
FPGA_PIO_setOutput(pio, 0, ADC_CLK | ADC_DIN);        // CLK, DIN Low 
 
// Check initial pin states 
unsigned int pins; 
FPGA_PIO_getInput(pio, &pins, ADC_DOUT | ADC_PENIRQ); 
printf("[DEBUG] Initial pins: DOUT=0x%08X, PENIRQ=0x%08X\n", pins & ADC_DOUT, pins & ADC_PENIRQ);
 
// Test raw DOUT 
printf("[DEBUG] Testing raw DOUT...\n"); 
for (int i = 0; i < 5; i++) { 
    FPGA_PIO_getInput(pio, &pins, ADC_DOUT); 
    printf("[DEBUG] Raw DOUT test %d: 0x%08X\n", i, pins); 
    usleep(1000); 
} 
 
// Power-up sequence 
usleep(1000); 
for (int i = 0; i < 3; i++) { 
    readADC(pio, 0x00); 
    usleep(1000); 
} 
 
printf("[DEBUG] Touchscreen GPIO directions set. Output: CS/CLK/DIN, Input: DOUT/PENIRQ\n"); 
return true; 
  
} 
bool TouchscreenADC_readXY(TouchscreenCtx_t* ctx, uint16_t* x, uint16_t* y) { unsigned int pins; 
// Check PENIRQ multiple times 
bool penirq_low = true; 
for (int i = 0; i < 3; i++) { 
    FPGA_PIO_getInput(ctx->gpio, &pins, ADC_PENIRQ); 
    printf("[DEBUG] Raw PENIRQ pins: 0x%08X\n", pins); 
    if (pins & ADC_PENIRQ) { 
        penirq_low = false; 
        break; 
    } 
    usleep(50); 
} 
 
printf("[DEBUG] PENIRQ state: %s\n", penirq_low ? "LOW (touch detected)" : "HIGH (no touch)"); 
 
if (!penirq_low) { 
    return false; 
} 
 
usleep(500); // Increased debounce 
 
// Try single-ended and differential modes 
uint32_t x_sum = 0, y_sum = 0; 
int valid_reads = 0; 
for (int i = 0; i < 3; i++) { 
    // Single-ended 
    uint16_t x_val = readADC(ctx->gpio, 0xD0); 
    uint16_t y_val = readADC(ctx->gpio, 0x90); 
    if (x_val > 0 && y_val > 0) { 
        x_sum += x_val; 
        y_sum += y_val; 
 valid_reads++; 
    } 
    usleep(100); 
    // Differential 
    x_val = readADC(ctx->gpio, 0xD1); 
    y_val = readADC(ctx->gpio, 0x91); 
    if (x_val > 0 && y_val > 0) { 
        x_sum += x_val; 
        y_sum += y_val; 
        valid_reads++; 
    } 
    usleep(100); 
} 
 
if (valid_reads == 0) { 
    *x = 0; 
    *y = 0; 
    return true; 
} 
 
*x = x_sum / valid_reads; 
*y = y_sum / valid_reads; 
printf("Touch Detected at X: %u, Y: %u (Averaged from %d reads)\n", *x, *y, valid_reads); 
return true; 
  
}
