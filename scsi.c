/*
 * scsi.c
 *
 *  Created on: 13 feb. 2018
 *      Author: erik.welander
 */
#include "scsi.h"
#include "stdint.h"

#include <usb.h>
#include <debug.h>

int SCSIhandleCommandBlock(uint8_t *CB, uint8_t len);

#ifdef DEBUG_SCSI
void dropCommandBlock(uint8_t *CB, uint8_t len)
{
	for (int n = 0; n != len; n++)
	{
		debugSendString(Dhex2str(CB[n]));
		debugSendString(" ");
	}

}
#endif

static const uint8_t inquiryData[0x24] =
{
		0x00,	/* Device connected, Direct access */
		0x80,	/* Removable */
		0x00, 	/* Version */
		0x02, 	/* Respose format etc. Has to be 2 */
		0x1F,	/* Additional length */
		0x00,	/* Reserved */
		0x00,	/* Special features ... */
		0x00,	/* ... see the docs */
		'S', 't', 'a', 'r', ' ', 'E', 'n', 0,
		'Q', 'u', 'a', 'n', 't', 'u', 'm', ' ', 'D', 'r', 'i', 'v', 'e', 0, 0, 0,
		'0', '.', '0', '1'
};

int SCSIhandleCommandBlock(uint8_t *CB, uint8_t len)
{
#ifdef DEBUG_SCSI
	debugSendString("New SCSI command string: ");
	dropCommandBlock(CB, len);
	debugSendString("\n");
#endif

	switch (CB[0])
	{
	case SCSI_CMD_INQUIRY:
		if ((CB[1] & 3) != 0 || CB[2] != 0)
		{
#ifdef DEBUG_SCSI
			debugSendString("Strange flags in SCSI INQUIRY request!");
#endif
			return 0x10;
		}

		USBepSend(1, &inquiryData, sizeof(inquiryData));
		return 0;

	default:
		return 1;
	}
}
