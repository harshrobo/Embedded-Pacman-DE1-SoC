@ -0,0 +1,72 @@ #include "Util/watchdog.h" #include "Util/macros.h" #include 
"DE1SoC_Addresses/DE1SoC_Addresses.h" #include "DE1SoC_WM8731/DE1SoC_WM8731.h" #include 
"HPS_GPIO/HPS_GPIO.h" #include "HPS_I2C/HPS_I2C.h" #include "FPGA_PIO/FPGA_PIO.h" #include 
<stdlib.h> #include <stdint.h> #include "audio_sample.h"  
void exitOnFail(HpsErr_t status) { if (ERR_IS_ERROR(status)) { exit((int)status); } } 
int main(void) { // Init hardware drivers FPGAPIOCtx_t* leds; WM8731Ctx_t* audio; HPSGPIOCtx_t* 
gpio; HPSI2CCtx_t* hpsi2c; 
// Initialise hardware interfaces 
exitOnFail(FPGA_PIO_initialise(LSC_BASE_RED_LEDS, LSC_CONFIG_RED_LEDS, &leds)); 
exitOnFail(HPS_GPIO_initialise(LSC_BASE_ARM_GPIO, ARM_GPIO_DIR, ARM_GPIO_I2C_GENERAL_MUX, 0, 
&gpio)); 
exitOnFail(HPS_I2C_initialise(LSC_BASE_I2C_GENERAL, I2C_SPEED_STANDARD, &hpsi2c)); 
exitOnFail(WM8731_initialise(LSC_BASE_AUDIOCODEC, &hpsi2c->i2c, &audio)); 
 
// Clear both ADC and DAC FIFOs 
WM8731_clearFIFO(audio, true, true); 
 
// Cast audio data 
const int32_t* samples = (const int32_t*)output_raw; 
 
31 
 
size_t total_samples = output_raw_len / sizeof(int32_t); 
size_t index = 0; 
 
while (1) { 
    unsigned int space = 0; 
    WM8731_getFIFOSpace(audio, &space); 
 
    // Show FIFO space on LEDs 
    FPGA_PIO_setOutput(leds, space, UINT32_MAX); 
 
    if ((index < total_samples) && (space > 0)) { 
        // Read the next interleaved 32-bit sample 
        int32_t current_sample = samples[index++]; 
 
        // Split the 32-bit sample into 2x 16-bit channels (interleaved LR) 
        int16_t left_sample  = (int16_t)(current_sample & 0xFFFF);             // Lower 16 bits 
        int16_t right_sample = (int16_t)((current_sample >> 16) & 0xFFFF);     // Upper 16 bits 
 
        // Shift 16-bit to 24-bit aligned (8-bit left shift) 
        unsigned int left  = ((int32_t)left_sample) << 8; 
        unsigned int right = ((int32_t)right_sample) << 8; 
 
        // Send to audio codec 
        WM8731_writeSample(audio, left, right); 
    } 
 
    // Loop the audio when it finishes 
    if (index >= total_samples) { 
        index = 0; 
    } 
 
    // Feed the watchdog 
    ResetWDT(); 
} 
 
return 0; 
} 
