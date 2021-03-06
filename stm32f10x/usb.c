/*
 * usb.c
 *
 *  Created on: 30 jan. 2018
 *      Author: erik.welander
 */
#include <stm32f10x.h>
#include "usb_def.h"
#include "usb.h"
#include "debug.h"
#include "../app_spec.h"
void printUSBstate()
{
	debugSendString(Dhex2str(USB->EP0R) );
	debugSendString("  ");
	debugSendString(Dhex2str(USB->EP1R) );
	debugSendString("  ");
	debugSendString(Dhex2str(USB->EP2R) );
	debugSendString("  ");
	debugSendString(Dhex2str(USB->ISTR) );
	debugSendString("  ");
	debugSendString(Dhex2str(USB->DADDR) );
}

void USBinit()
{
    /* Enable necessary USB clocks */
    /* USB pins are located on port A. Enable the port A clock */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    /* Set USB prescaler to 1.5 (72 MHz / 3 * 2 = 48 MHz */
    RCC->CFGR &= ~RCC_CFGR_USBPRE;
    /* Enable USB clock */
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;

    /* Configure pins as push-pull outputs */
    GPIOA->CRH |= GPIO_CRH_MODE11;   //Output mode, max 50 MHz
    GPIOA->CRH &= ~GPIO_CRH_CNF11;   //Push-pull
    GPIOA->CRH |= GPIO_CRH_MODE12;   //Output mode, max 50 MHz
    GPIOA->CRH &= ~GPIO_CRH_CNF12;   //Push-pull

    /* Pull Data lines low */
    GPIOA->BRR = (1U << 11U) | (1U << 12U);

    for (uint16_t i = 0x100; i > 0; i--) {
        /* Minimal delay to allow unconnected appearance */
    }

    /* Setup the USB pins */
    /* PA11 is USB Data Minus */
    GPIOA->CRH |= GPIO_CRH_CNF11_1;  //Alternate function push-pull
    /* PA12 is USB Data Plus */
    GPIOA->CRH |= GPIO_CRH_CNF12_1;  //Alternate function push-pull
    /* Set pins high */
    GPIOA->BSRR = (1 << 11U) | (1 << 12U);

    USB->CNTR &= ~USB_CNTR_PDWN;     //Exit Power Down mode
    for (uint16_t i = 0x100; i > 0; i--) {
        /* Delay more than 1 us to allow startup time */
    }
    USB->CNTR &= ~USB_CNTR_FRES;     //Clear forced USB reset
    USB->ISTR = 0x00U;               //Clear any spurious interrupts

    /* Enable CTRM and RESET interrupts */
    USB->CNTR |= (USB_CNTR_CTRM | USB_CNTR_RESETM);
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn); //Enable the USB NVIC interrupts
}
void USBreset(void)
{
#ifdef DEBUG_USB
	debugSendString("Resetting USB.\n");
#endif

    /* Enable device (and set the device address to zero */
    USB->DADDR = USB_DADDR_EF;

    /* Enable endpoints  */
    // Endpoint zero (control)
    USB_BDT(USB_EP0)->ADDR_TX = 0x0040;
    USB_BDT(USB_EP0)->ADDR_RX = 0x0080;

    // Endpoint one
    USB_BDT(USB_EP1)->ADDR_TX = 0x00C0;

    USB_BDT(USB_EP2)->ADDR_RX = 0x0100;

    /* Set block size to 32 bytes and number of blocks to 2 */
    USB_BDT(USB_EP0)->COUNT_RX = USB_COUNT0_RX_BLSIZE | USB_COUNT0_RX_NUM_BLOCK_1;
    USB_BDT(USB_EP2)->COUNT_RX = USB_COUNT0_RX_BLSIZE | USB_COUNT0_RX_NUM_BLOCK_1;

    /* Set Buffer Description Table address */
    USB->BTABLE = 0x0000U;


    /*
     * Set the endpoint types.
     * Also, start with TX NAK, because we have nothing prepared to send
     */

    USB->EP0R = ( ((USB->EP0R & (USB_EP_STAT_TX | USB_EP_STAT_RX)) ^ (USB_RX_VALID | USB_TX_NAK))
    		|  USB_EP_CONTROL | USB_CLEAR_MASK ) & ~(USB_EP_DTOG_RX | USB_EP_DTOG_TX);


    USB->EP1R = ( ( (USB->EP1R & (USB_EP_STAT_TX | USB_EP_STAT_RX )) ^ (USB_RX_NAK | USB_TX_NAK))
    		|  USB_EP_BULK | USB_CLEAR_MASK | 0x0001) & ~( USB_EP_DTOG_RX | USB_EP_DTOG_TX | 0x000E);

    USB->EP2R = ( ( (USB->EP2R & (USB_EP_STAT_TX | USB_EP_STAT_RX )) ^ (USB_RX_VALID | USB_TX_NAK))
    		|  USB_EP_BULK | USB_CLEAR_MASK | 0x0002) & ~( USB_EP_DTOG_RX | USB_EP_DTOG_TX | 0x000D);

  /*  debugSendString("Reset status: ");
    debugSendString(Dhex2str(USB->EP0R));
    debugSendString(" ");
    debugSendString(Dhex2str( USB->DADDR));
    debugSendString(" ");
    debugSendString(Dhex2str( USB->ISTR));
    debugSendString("\n");*/
}

void USB_LP_CAN1_RX0_IRQHandler()
{
#ifdef DEBUG_USB
	debugSendString("E: ");
	printUSBstate();
	debugSendString("\n");
#endif

	uint16_t ISTR = USB->ISTR;

    if (0 != (ISTR & USB_ISTR_RESET))
    {
#ifdef DEBUG_USB
	debugSendString("Reset....\n");
#endif

        /* Reset Request */
    	USB->ISTR = ~USB_ISTR_RESET; // Clear interrupt

//    	USBreset();
    	//return;

    	/* Set Buffer Description Table address */
    	USB->BTABLE = 0x0000U;

    	USB_BDT(USB_EP0)->ADDR_TX =  USB_BDT(USB_EP0)->ADDR_RX = 0x0040;


    	/* Set block size to 32 bytes and number of blocks to 2 */
    	USB_BDT(USB_EP0)->COUNT_RX = USB_COUNT0_RX_BLSIZE | USB_COUNT0_RX_NUM_BLOCK_1;

         USB->EP0R = ( ((USB->EP0R & (USB_EP_STAT_TX | USB_EP_STAT_RX)) ^ (USB_RX_VALID | USB_TX_NAK))
    	    		|  USB_EP_CONTROL | USB_CLEAR_MASK ) & ~(USB_EP_DTOG_RX | USB_EP_DTOG_TX);

  	    USBAppCallback(USBresetCmd);
    	return;
    }

    if (0 != (ISTR & USB_ISTR_CTR) )
    {
    	if (0 != (ISTR & USB_ISTR_DIR) )
    	{
    		if (0 != (USB->EP0R & USB_EP_SETUP) )
    		{
    			/* Setup requested. */

    			/* Clear RX bit */
    			USB->EP0R = USB->EP0R & ~(USB_TOGGLE_MASK | USB_EP_CTR_RX | USB_EP_CTR_TX);
    			USB->EP0R = USB->EP0R  ^ (USB_RX_NAK | USB_TX_NAK  | USB_EP_DTOG_TX);
    			USBAppCallback(USBsetupCmd);
    			return;
    		}

    		USBAppCallback(USBtransOut | ((ISTR & USB_EP_EA) << 8) );

    		return;
    	}

    	USBAppCallback(USBtransIn | ((ISTR & USB_EP_EA) << 8) );
    	return;
    }
}

void USBdisable()
{
	/* Should do a proper shutdown. Fix later */
#ifdef DEBUG_USB
	debugSendString("Shutting Down USB Services.\n");
#endif
	NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
}

void USBresume()
{
	 USB->DADDR |= USB_DADDR_EF;
}

void USBpause()
{
	USB->DADDR = (USB->DADDR & ~USB_DADDR_EF);
}

void USBsetAddress(uint8_t newAddress)
{
	USB->DADDR = (USB->DADDR & USB_DADDR_EF) | newAddress;
}

uint8_t USBgetAddress()
{
	return USB->DADDR & USB_DADDR_ADD;
}

void USBconfigEPs(USB_EP_block_t *EPs, int nEP)
{
	int addrOffs = 0x0080;

	while (nEP-- != 0)
	{
		uint16_t epr = (EPs[nEP].EPid & 0x0F) | (EPs[nEP].features & 0x0600) | USB_CLEAR_MASK;

		if (EPs[nEP].bufSizeRX != 0)
		{
			uint16_t cntRx = (EPs[nEP].bufSizeRX & 0x03E0);
			USB_BDT(EPs[nEP].EPid)->ADDR_RX = addrOffs;
			addrOffs += cntRx;
			USB_BDT(EPs[nEP].EPid)->COUNT_RX = USB_COUNT0_RX_BLSIZE | (cntRx << 5);

			/* Allow incoming data */
			epr |= USB_RX_VALID;

#ifdef DEBUG_USB
			debugSendString("Setup RX for EP");
			debugSendString(Dhex2str(EPs[nEP].EPid));
			debugSendString(" with COUNT_RX ");
			debugSendString(Dhex2str(USB_BDT(EPs[nEP].EPid)->COUNT_RX));
			debugSendString(" and with ADDR_RX ");
			debugSendString(Dhex2str(USB_BDT(EPs[nEP].EPid)->ADDR_RX));
			debugSendString("\n");
#endif
		}

		if (EPs[nEP].bufSizeTX != 0)
		{
			USB_BDT(EPs[nEP].EPid)->ADDR_TX = addrOffs;
			addrOffs += EPs[nEP].bufSizeTX;
			USB_BDT(EPs[nEP].EPid)->COUNT_TX = EPs[nEP].bufSizeTX;

			/* Set to not acknowledge, since we do not know what to send yet. */
			epr |= USB_TX_NAK;
#ifdef DEBUG_USB
			debugSendString("Setup TX for EP");
			debugSendString(Dhex2str(EPs[nEP].EPid));
			debugSendString(" with buffersize ");
			debugSendString(Dhex2str(USB_BDT(EPs[nEP].EPid)->COUNT_TX));
			debugSendString(" and with ADDR_TX ");
			debugSendString(Dhex2str(USB_BDT(EPs[nEP].EPid)->ADDR_TX));
			debugSendString("\n");
#endif
		}

		USB_EP(EPs[nEP].EPid) = (USB_EP(EPs[nEP].EPid) ^ (USB_TOGGLE_MASK & epr)) | epr;
#ifdef DEBUG_USB
			debugSendString("Setup EP");
			debugSendString(Dhex2str(EPs[nEP].EPid));
			debugSendString("R: ");
			debugSendString(Dhex2str(USB_EP(EPs[nEP].EPid)));
			debugSendString("\n");
#endif

	}
	printUSBstate();
}

int USBepRead(int EPid, void *buf, int len)
{
	int N = len < (USB_BDT(EPid)->COUNT_RX & USB_COUNT0_RX_COUNT0_RX) ? len : (USB_BDT(EPid)->COUNT_RX & USB_COUNT0_RX_COUNT0_RX);

	uint32_t *src = (uint32_t *)(PMA_BASE + (USB_BDT(EPid)->ADDR_RX << 1U));
	uint16_t *dst = (uint16_t *) buf;

	for (int n = (N + 1) / 2; 0 != n; n--) *dst++ = *src++;

	/* Allow new incoming data */

	uint16_t x = USB_EP(EPid);
	USB_EP(EPid) = ((x & USB_EP_STAT_RX) ^ USB_RX_VALID) | (x & (USB_EP_EA | USB_EP_KIND | USB_EP_TYPE ) ) | USB_EP_CTR_TX;

	return N;
}

int USBepSend(int EPid, const void *buf, int len)
{
	if ((USB_EP(EPid) & USB_EP_STAT_TX) != USB_TX_NAK )
	{
		debugSendString("Waiting to send on EP ");
		debugSendString(Dhex2str(EPid));
		debugSendString("\n");
		while ((USB_EP(EPid) & USB_EP_STAT_TX) != USB_TX_NAK);
	}

	debugSendString("Sending on EP ");
	debugSendString(Dhex2str(EPid));
	debugSendString("\n");

	uint32_t *dst = (uint32_t *) (PMA_BASE + (USB_BDT(EPid)->ADDR_TX << 1U));
	uint16_t *src = (uint16_t *) buf;

	for (int n = (len + 1) / 2; 0 != n; n--) *dst++ = *src++;

	USB_BDT(EPid)->COUNT_TX = len;
	USB_EP(EPid) = (USB_EP(EPid) & ~USB_TOGGLE_MASK) | USB_CLEAR_MASK | USB_EP_FLIP_TX;

	return len;
}

void USBacknowledge(int EPid)
{
	USB_BDT(EPid)->COUNT_TX = 0;
	USB_EP(EPid) = (USB_EP(EPid) & ~USB_TOGGLE_MASK) | USB_CLEAR_MASK | USB_EP_FLIP_TX;
}

void USBconfirmSent(int EPid)
{
	USB_EP(EPid) = USB_EP(EPid) & ~USB_TOGGLE_MASK & ~USB_EP_CTR_TX;
}
