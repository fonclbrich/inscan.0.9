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

// int SCSIhandleCommandBlock(uint8_t *CB, uint8_t len);

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

typedef struct
{
	uint8_t		reserved[3];
	uint8_t 	listLength;
	uint8_t 	numberOfBlocks[4];	/* Note: should be Big Endian */
	uint8_t		descriptorType;
	uint8_t		blockLength[3];
} __attribute__((packed)) formatCapacitiesData;

const formatCapacitiesData capData = {
	.reserved = 		{00, 00, 00},
	.listLength = 		8,
	.numberOfBlocks =   {00, 00, 04, 00},
	.descriptorType =	2,
	.blockLength =		{00, 02, 00}
};

int SCSIhandleCommandBlock(uint8_t *CB, uint8_t len, USB_command_status_wrapper *csw)
{
	int returnCode = 1;

#ifdef DEBUG_SCSI
	debugSendString("SCSI command ");
	debugSendString(Dhex2str(CB[0]));
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
			returnCode = 0x10;
			break;
		}

		csw->dataResidue -= USBepSend(1, &inquiryData, sizeof(inquiryData));
		returnCode = 0;
		break;

	case SCSI_READ_FORMAT_CAPACITIES: ;
		int allocationLength = CB[8] + (((int) CB[7]) << 8);

		if (sizeof(capData) < allocationLength) allocationLength = sizeof(capData);
		csw->dataResidue -= USBepSend(1, &capData, allocationLength);
		returnCode = 0;
		break;

	default:
#ifdef DEBUG_SCSI
		debugSendString("Could not interpret SCSI command string: ");
		dropCommandBlock(CB, len);
		debugSendString("\n");

		return returnCode;
#else
		break;
#endif
	}

#ifdef DEBUG_SCSI
	if ((returnCode != 0) && (returnCode != 1))
	{
		debugSendString("Error processing SCSI command string: ");
		dropCommandBlock(CB, len);
		debugSendString("\n");
	}
#endif

	return returnCode;
}
