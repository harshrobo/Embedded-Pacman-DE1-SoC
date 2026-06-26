#include "Util/watchdog.h"
#include "FPGA_PIO/FPGA_PIO.h"
#include "DE1SoC_Addresses/DE1SoC_Addresses.h"
#include "DE1SoC_LT24/DE1SoC_LT24.h"

#include "menu_image.h"
#include "controls_image.h"
#include "video_frames.h"

#include <stdlib.h>

void exitOnFail(HpsErr_t status)
{
    if (ERR_IS_ERROR(status))
    {
        exit((int)status);
    }
}

void delay(int ms)
{
    for (volatile int i = 0; i < ms * 1000; i++);
}

const unsigned short* videoFrames[TOTAL_FRAMES] =
{
    EmbPac_0001,
    EmbPac_0002,
    EmbPac_0003,
    EmbPac_0004
};

int main(void)
{
    FPGAPIOCtx_t* gpio1;
    FPGAPIOCtx_t* switches;
    LT24Ctx_t* lt24;

    exitOnFail(
        FPGA_PIO_initialise(
            LSC_BASE_GPIO_JP1,
            LSC_CONFIG_GPIO,
            &gpio1));

    exitOnFail(
        FPGA_PIO_initialise(
            LSC_BASE_SLIDE_SWITCH,
            LSC_CONFIG_SLIDE_SWITCH,
            &switches));

    exitOnFail(
        LT24_initialise(
            &gpio1->gpio,
            LSC_BASE_LT24HWDATA,
            &lt24));

    ResetWDT();

    LT24_copyFrameBuffer(
        lt24,
        Menu_img,
        0,
        0,
        240,
        320);

    while (1)
    {
        unsigned int switchState;

        FPGA_PIO_getInput(
            switches,
            &switchState,
            LSC_SLIDE_SWITCH_MASK);

        if (switchState & 0x1)
        {
            for (int i = 0; i < TOTAL_FRAMES; i++)
            {
                LT24_copyFrameBuffer(
                    lt24,
                    videoFrames[i],
                    0,
                    0,
                    240,
                    320);

                delay(100);

                ResetWDT();
            }
        }

        ResetWDT();
    }

    return 0;
}
