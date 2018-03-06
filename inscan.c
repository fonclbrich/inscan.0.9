#include <blinkLED.h>
#include <debug.h>
#include <timer.h>
#include <clock.h>
#include <usb.h>
#include <stm32f10x_tim.h>

int main(void)
{
	clockInit();

	blinkLEDinit();

	debugInit();

	timerInit();

	USBinit();

    while(1)
    {
    	waitSyncTimer();
    	blinkLEDset();

    	waitSyncTimer();
    	blinkLEDreset();
    }
}
