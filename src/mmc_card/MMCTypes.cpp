#include <windows.h>
#include <string>
#include <iostream>
#include <vector>

#include "MMCTypes.h"

std::string MMCStatusToString(BYTE value)
{
   switch(value)
   {
   case 0:
      return "idle";
   case 1:
      return "ready";
   case 2:
      return "ident";
   case 3:
      return "stby";
   case 4:
      return "tran";
   case 5:
      return "data";
   case 6:
      return "rcv";
   case 7:
      return "prg";
   case 8:
      return "dis";
   case 9:
      return "btst";
   case 10:
      return "slp";
   default:
      return "reserved";
   }
}

bool psvcd::PrintMMCStatus(const std::vector<BYTE>& bytes)
{
   std::cout << "CMD: " << (int)bytes[0] << std::endl;
      
   bool addressOutOfRange = ((bytes[1] & 0x80) > 0);
   bool addressMisalign =   ((bytes[1] & 0x40) > 0);
   bool blockLenError =     ((bytes[1] & 0x20) > 0);
   bool eraseSeqError =     ((bytes[1] & 0x10) > 0);
   bool eraseParam =        ((bytes[1] & 0x08) > 0);
   bool wpViolation =       ((bytes[1] & 0x04) > 0);
   bool cardIsLocked =      ((bytes[1] & 0x02) > 0);
   bool lockUnlockFailed =  ((bytes[1] & 0x01) > 0);

   bool comCrcError =       ((bytes[2] & 0x80) > 0);
   bool illegalCommand =    ((bytes[2] & 0x40) > 0);
   bool cardEccFailed =     ((bytes[2] & 0x20) > 0);
   bool ccError =           ((bytes[2] & 0x10) > 0);
   bool error =             ((bytes[2] & 0x08) > 0);
   bool underrun =          ((bytes[2] & 0x04) > 0);
   bool overrun =           ((bytes[2] & 0x02) > 0);
   bool cidCsdOveriwrite =  ((bytes[2] & 0x01) > 0);

   bool wpEraseSkip =     ((bytes[3] & 0x80) > 0);
   bool reserved14 =      ((bytes[3] & 0x40) > 0);
   bool eraseReset =      ((bytes[3] & 0x20) > 0);
   std::string currentState = MMCStatusToString(((bytes[3] & 0x1E) >> 1));
   bool readyForData =    ((bytes[3] & 0x01) > 0);

   bool switchError =  ((bytes[4] & 0x80) > 0);
   bool reserved6 =    ((bytes[4] & 0x40) > 0);
   bool appCmd =       ((bytes[4] & 0x20) > 0);
   bool reserved4 =    ((bytes[4] & 0x10) > 0);
   bool reserved3 =    ((bytes[4] & 0x08) > 0);
   bool reserved2 =    ((bytes[4] & 0x04) > 0);
   bool reserved1 =    ((bytes[4] & 0x02) > 0);
   bool reserved0 =    ((bytes[4] & 0x01) > 0);

   if(addressOutOfRange)
      std::cout << "ADDRESS_OUT_OF_RANGE: "       << addressOutOfRange << std::endl;
   if(addressMisalign)
      std::cout << "ADDRESS_MISALIGN: "      << addressMisalign << std::endl;
   if(blockLenError)
      std::cout << "BLOCK_LEN_ERROR: "    << blockLenError << std::endl;
   if(eraseSeqError)
      std::cout << "ERASE_SEQ_ERROR: "    << eraseSeqError << std::endl;
   if(eraseParam)
      std::cout << "ERASE_PARAM: "        << eraseParam << std::endl;
   if(wpViolation)
      std::cout << "WP_VIOLATION: "       << wpViolation << std::endl;  
   if(cardIsLocked)
      std::cout << "CARD_IS_LOCKED: "     << cardIsLocked << std::endl;
   if(lockUnlockFailed)
      std::cout << "LOCK_UNLOCK_FAILED: " << lockUnlockFailed << std::endl;

   if(comCrcError)
      std::cout << "COM_CRC_ERROR: "   << comCrcError << std::endl;
   if(illegalCommand)
      std::cout << "ILLEGAL_COMMAND: " << illegalCommand << std::endl;
   if(cardEccFailed)
      std::cout << "CARD_ECC_FAILED: " << cardEccFailed << std::endl;
   if(ccError)
      std::cout << "CC_ERROR: "        << ccError << std::endl;
   if(error)
      std::cout << "ERROR: "           << error << std::endl;
   if(underrun)
      std::cout << "UNDERRUN: "           << underrun << std::endl;
   if(overrun)
      std::cout << "OVERRUN: "           << overrun << std::endl;
   if(cidCsdOveriwrite)
      std::cout << "CID/CSD_OVERWRITE: "   << cidCsdOveriwrite << std::endl;

   if(wpEraseSkip)
      std::cout << "WP_ERASE_SKIP: "     << wpEraseSkip << std::endl;
   if(eraseReset)
      std::cout << "ERASE_RESET: "       << eraseReset << std::endl;
   
   std::cout << "CURRENT_STATE: "     << currentState << std::endl;
   
   if(readyForData)
      std::cout << "READY_FOR_DATA: "    << readyForData << std::endl;

   if(switchError)
      std::cout << "SWITCH_ERROR: "<< switchError << std::endl;
   if(appCmd)
      std::cout << "APP_CMD: "       << appCmd << std::endl;
   
   if(reserved14)
      std::cout << "RESERVED14: " << reserved14 << std::endl;
   if(reserved6)
      std::cout << "RESERVED6: " << reserved6 << std::endl;
   if(reserved4)
      std::cout << "RESERVED4: " << reserved4 << std::endl;
   if(reserved3)
      std::cout << "RESERVED3: " << reserved3 << std::endl;
   if(reserved2)
      std::cout << "RESERVED2: " << reserved2 << std::endl;
   if(reserved1)
      std::cout << "RESERVED1: " << reserved1 << std::endl;
   if(reserved0)
      std::cout << "RESERVED0: " << reserved0 << std::endl;    

   return true;
}

bool psvcd::GetMMCStatus(const std::vector<BYTE>& bytes, psvcd::MMC_CardStatusInfo& info)
{
   info.cmd = (int)bytes[0];
      
   info.addressOutOfRange = ((bytes[1] & 0x80) > 0);
   info.addressMisalign =   ((bytes[1] & 0x40) > 0);
   info.blockLenError =     ((bytes[1] & 0x20) > 0);
   info.eraseSeqError =     ((bytes[1] & 0x10) > 0);
   info.eraseParam =        ((bytes[1] & 0x08) > 0);
   info.wpViolation =       ((bytes[1] & 0x04) > 0);
   info.cardIsLocked =      ((bytes[1] & 0x02) > 0);
   info.lockUnlockFailed =  ((bytes[1] & 0x01) > 0);

   info.comCrcError =       ((bytes[2] & 0x80) > 0);
   info.illegalCommand =    ((bytes[2] & 0x40) > 0);
   info.cardEccFailed =     ((bytes[2] & 0x20) > 0);
   info.ccError =           ((bytes[2] & 0x10) > 0);
   info.error =             ((bytes[2] & 0x08) > 0);
   info.underrun =          ((bytes[2] & 0x04) > 0);
   info.overrun =           ((bytes[2] & 0x02) > 0);
   info.cidCsdOveriwrite =  ((bytes[2] & 0x01) > 0);

   info.wpEraseSkip =     ((bytes[3] & 0x80) > 0);
   info.reserved14 =      ((bytes[3] & 0x40) > 0);
   info.eraseReset =      ((bytes[3] & 0x20) > 0);
   info.currentState = (MMC_CardStatus)((bytes[3] & 0x1E) >> 1);
   info.readyForData =    ((bytes[3] & 0x01) > 0);

   info.switchError =  ((bytes[4] & 0x80) > 0);
   info.reserved6 =    ((bytes[4] & 0x40) > 0);
   info.appCmd =       ((bytes[4] & 0x20) > 0);
   info.reserved4 =    ((bytes[4] & 0x10) > 0);
   info.reserved3 =    ((bytes[4] & 0x08) > 0);
   info.reserved2 =    ((bytes[4] & 0x04) > 0);
   info.reserved1 =    ((bytes[4] & 0x02) > 0);
   info.reserved0 =    ((bytes[4] & 0x01) > 0);

   return true;
}