/*
 * usb.c
 *
 *  Created on: 6 mars 2018
 *      Author: erik.welander
 */

#include "app_spec.h"
#include "scsi.h"
#include <debug.h>
#include <usb.h>

extern uint16_t *USBstringDesc[];
extern const USB_device_descriptor_t USBdevDesc;
extern const USB_combined_MS_descriptor_t USBcomboMSdesc;
extern USB_EP_block_t EPConfig;
extern const uint8_t USB_MAX_LUN;

#ifdef DEBUG_USB
void dumpUSBSetupPacket(USB_setup_packet_t *setupPacket)
{
	char numBuf[0x10];
	numBuf[0xF] = 0;
	numBuf[0xE] = '\n';
	debugSendString("bmRequestType: ");
	debugSendString(hex2str(setupPacket->bmRequestType, &numBuf[0xD]));

	debugSendString("bRequest: ");
	debugSendString(hex2str(setupPacket->bRequest, &numBuf[0xD]));

	debugSendString("wValue: ");
	debugSendString(hex2str(setupPacket->wValue, &numBuf[0xD]));

	debugSendString("wIndex: ");
	debugSendString(hex2str(setupPacket->wIndex, &numBuf[0xD]));

	debugSendString("wLength: ");
	debugSendString(hex2str(setupPacket->wLength, &numBuf[0xD]));
}

void dumpCBW(USB_command_block_wrapper *cbw)
{
	debugSendString("signature: ");
	debugSendString(Dhex2str(cbw->signature));

	debugSendString("\ntag: ");
	debugSendString(Dhex2str(cbw->tag));

	debugSendString("\ndataTransferLength: ");
	debugSendString(Dhex2str(cbw->dataTransferLength));

	debugSendString("\nflags: ");
	debugSendString(Dhex2str(cbw->flags));

	debugSendString("\nLUN: ");
	debugSendString(Dhex2str(cbw->LUN));

	debugSendString("\nCBlength ");
	debugSendString(Dhex2str(cbw->CBlength));

	debugSendString("\nCommandBlock: ");

	for (int n = 0; n != cbw->CBlength; n++)
	{
		debugSendString(Dhex2str(cbw->commandBlock[n]));
		debugSendString(" ");
	}

	debugSendString("\n");
}

#endif

void USBCallback(uint16_t event)
{
	static uint8_t newAddress = 0;
	uint8_t what = event & 0x00FF;
	uint8_t EPid = event >> 8;

	switch (what)
	{
	case USBresetCmd :
#ifdef DEBUG_USB
		debugSendString("Got Reset Command.\n");
#endif
		USBconfigEPs(&EPConfig, 2);
		USBsetAddress(0);
		USBresume();
		return;

	case USBsetupCmd :
		if (0 != EPid)
		{
			debugSendString("Incorrect SETUP on EP");
			debugSendString(Dhex2str(EPid));
			debugSendString(" !\n");
			USBdisable();
			return;
		}

		USB_setup_packet_t setup;
		USBepRead(0, &setup, sizeof(setup));

		switch (setup.bmRequestType)
		{
		case USB_DIR_DEVICE_TO_HOST | USB_RECIPIENT_DEVICE :
			switch (setup.bRequest)
			{
			case USB_SETUP_GET_DESCRIPTOR :
				switch (setup.wValue >> 8)
				{
				case USB_SETUP_DESC_DEVICE :
#ifdef DEBUG_USB
					debugSendString("Sending device descriptor.\n");
#endif
					USBepSend(0, &USBdevDesc, sizeof(USBdevDesc));
					return;

				case USB_SETUP_DESC_CONFIGURATION :
#ifdef DEBUG_USB
					debugSendString("Sending configuration descriptor.\n");
#endif
					USBepSend(0, &USBcomboMSdesc, setup.wLength < sizeof(USBcomboMSdesc) ? sizeof(USBcomboMSdesc.configDesc) : sizeof(USBcomboMSdesc));
					return;

				case USB_SETUP_DESC_STRING :
				{
					uint8_t stringIndex = setup.wValue & 0xFF;

					if (stringIndex <= 3)
					{
#ifdef DEBUG_USB
					debugSendString("Sending string descriptor (");
					debugSendString(Dhex2str(stringIndex));
					debugSendString("). Length: ");
					debugSendString(Dhex2str(*USBstringDesc[stringIndex] & 0xFF));
					debugSendString("\n");
#endif

					USBepSend(0, USBstringDesc[stringIndex], *USBstringDesc[stringIndex] & 0xFF);
					return;
					}

					break;
				}

				default:
					break;
				}

				break;

			default:
				break;
			}
			break;

		case USB_DIR_HOST_TO_DEVICE | USB_RECIPIENT_DEVICE :
			switch (setup.bRequest)
			{
			case USB_SETUP_SET_ADDRESS :
				newAddress = setup.wValue & 0x7F;
				USBacknowledge(0);
				return;

			case USB_SETUP_SET_CONFIGURATION :
#ifdef DEBUG_USB
				debugSendString("New configuration assigned: ");
				debugSendString(Dhex2str(setup.wValue & 0x7F));
				debugSendString("\n");
#endif
				USBacknowledge(0);
				return;

			default:
				break;
			}

			break;

		case USB_DIR_DEVICE_TO_HOST | USB_RECIPIENT_INTERFACE |  USB_TYPE_CLASS :
			switch (setup.bRequest)
			{
			case USB_MS_GET_MAX_LUN : ;
#ifdef DEBUG_USB
				debugSendString("Max LUN requested.\n");
#endif
				//uint8_t maxLun = 0;
				USBepSend(0, &USB_MAX_LUN, 1);

				return;

			default:
				break;
			}
			break;

		default:
			break;
		}

		debugSendString("Unhandeled USB setup: ");
		dumpUSBSetupPacket(&setup);
		debugSendString("\n");
		return;

	case USBtransIn :
		if (newAddress != 0)
		{
			USBsetAddress(newAddress);
			newAddress = 0;
#ifdef DEBUG_USB
			debugSendString("New address set to ");
			debugSendString(Dhex2str(USBgetAddress()));
			debugSendString("\n");
		}
else
		{
			debugSendString("Sent on EP");
			debugSendString(Dhex2str(EPid));
			debugSendString("...\n");
#endif
		}
		USBconfirmSent(EPid);
		return;

	case USBtransOut :
		if (EPid == 2)
		{
			USB_command_block_wrapper cbw;
			USBepRead(2, &cbw, sizeof(cbw));

			if (CBW_SIGN != cbw.signature  || USB_MAX_LUN < cbw.LUN || 0x10 < cbw.CBlength  || 0 == cbw.CBlength)
			{
				debugSendString("CBW Data makes no sense:\n");
#ifdef DEBUG_USB
				dumpCBW(&cbw);
#endif
			}
			else
			{
				(SCSIhandleCommandBlock(cbw.commandBlock, cbw.CBlength));
				return;
			}
		}

		debugSendString("Out transaction.\n");
		USBdisable();
		break;
	}

	debugSendString("Unhandeled USB event: ");
	debugSendString(Dhex2str(event));
	debugSendString(".\n");
}
