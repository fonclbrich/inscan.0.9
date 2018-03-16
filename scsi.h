/*
 * scsi.h
 *
 *  Created on: 13 feb. 2018
 *      Author: erik.welander
 */

#ifndef STM32F10X_SCSI_H_
#define STM32F10X_SCSI_H_

#define SCSI_CMD_INQUIRY		0x12

#include <stdint.h>
#include <usb.h>

int SCSIhandleCommandBlock(uint8_t *CB, uint8_t len, USB_command_status_wrapper *csw);

#endif /* STM32F10X_SCSI_H_ */
