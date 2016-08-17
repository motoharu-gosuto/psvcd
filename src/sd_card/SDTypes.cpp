#include <iostream>
#include <string>

#include "SDTypes.h"

std::string SDStatusToString(BYTE value)
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
   default:
      return "reserved";
   }
}

bool psvcd::PrintSDStatus(const std::vector<BYTE>& bytes)
{
   std::cout << "CMD: " << (int)bytes[0] << std::endl;
      
   bool outOfRange =       ((bytes[1] & 0x80) > 0);
   bool addressError =     ((bytes[1] & 0x40) > 0);
   bool blockLenError =    ((bytes[1] & 0x20) > 0);
   bool eraseSeqError =    ((bytes[1] & 0x10) > 0);
   bool eraseParam =       ((bytes[1] & 0x08) > 0);
   bool wpViolation =      ((bytes[1] & 0x04) > 0);
   bool cardIsLocked =     ((bytes[1] & 0x02) > 0);
   bool lockUnlockFailed = ((bytes[1] & 0x01) > 0);

   bool comCrcError =    ((bytes[2] & 0x80) > 0);
   bool illegalCommand = ((bytes[2] & 0x40) > 0);
   bool cardEccFailed =  ((bytes[2] & 0x20) > 0);
   bool ccError =        ((bytes[2] & 0x10) > 0);
   bool error =          ((bytes[2] & 0x08) > 0);
   bool reserved18 =     ((bytes[2] & 0x04) > 0);
   bool reserved17 =     ((bytes[2] & 0x02) > 0);   
   bool csdOveriwrite =  ((bytes[2] & 0x01) > 0);

   bool wpEraseSkip =     ((bytes[3] & 0x80) > 0);
   bool cardEccDisabled = ((bytes[3] & 0x40) > 0);
   bool eraseReset =      ((bytes[3] & 0x20) > 0);
   std::string currentState = SDStatusToString(((bytes[3] & 0x1E) >> 1));
   bool readyForData =    ((bytes[3] & 0x01) > 0);

   bool reserved7 =      ((bytes[4] & 0x80) > 0);
   bool reserved6 =      ((bytes[4] & 0x40) > 0);
   bool appCmd =         ((bytes[4] & 0x20) > 0);
   bool reserved4 =      ((bytes[4] & 0x10) > 0);
   bool akeSeqError =    ((bytes[4] & 0x08) > 0);
   bool reserved2 =      ((bytes[4] & 0x04) > 0);
   bool reserved1 =      ((bytes[4] & 0x02) > 0);
   bool reserved0 =      ((bytes[4] & 0x01) > 0);

   if(outOfRange)
      std::cout << "OUT_OF_RANGE: "       << outOfRange << std::endl;
   if(addressError)
      std::cout << "ADDRESS_ERROR: "      << addressError << std::endl;
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
   if(csdOveriwrite)
      std::cout << "CSD_OVERWRITE: "   << csdOveriwrite << std::endl;

   if(wpEraseSkip)
      std::cout << "WP_ERASE_SKIP: "     << wpEraseSkip << std::endl;

   if(cardEccDisabled)
      std::cout << "CARD_ECC_DISABLED: " << cardEccDisabled << std::endl;

   if(eraseReset)
      std::cout << "ERASE_RESET: "       << eraseReset << std::endl;
   
   std::cout << "CURRENT_STATE: "     << currentState << std::endl;
   
   if(readyForData)
      std::cout << "READY_FOR_DATA: "    << readyForData << std::endl;
   
   if(appCmd)
      std::cout << "APP_CMD: "       << appCmd << std::endl;
   if(akeSeqError)
      std::cout << "AKE_SEQ_ERROR: " << akeSeqError << std::endl;

   if(reserved18)
      std::cout << "RESERVED18: " << reserved18 << std::endl;
   if(reserved17)
      std::cout << "RESERVED17: " << reserved17 << std::endl;
   if(reserved7)
      std::cout << "RESERVED7: " << reserved7 << std::endl;
   if(reserved6)
      std::cout << "RESERVED6: " << reserved6 << std::endl;
   if(reserved4)
      std::cout << "RESERVED4: " << reserved4 << std::endl;
   if(reserved2)
      std::cout << "RESERVED2: " << reserved2 << std::endl;
   if(reserved1)
      std::cout << "RESERVED1: " << reserved1 << std::endl;
   if(reserved0)
      std::cout << "RESERVED0: " << reserved0 << std::endl;

   return true;
}

bool psvcd::PrintSD_CMD3Status(const std::vector<BYTE>& bytes)
{
   std::cout << "CMD: " << (int)bytes[0] << std::endl;

   bool comCrcError =     ((bytes[3] & 0x80) > 0);
   bool illegalCommand =  ((bytes[3] & 0x40) > 0);
   bool error =           ((bytes[3] & 0x20) > 0);
   std::string currentState = SDStatusToString(((bytes[3] & 0x1E) >> 1));
   bool readyForData =    ((bytes[3] & 0x01) > 0);

   bool appCmd =          ((bytes[4] & 0x20) > 0);
   bool akeSeqError =     ((bytes[4] & 0x08) > 0);

   if(comCrcError)
      std::cout << "COM_CRC_ERROR: "   << comCrcError << std::endl;
   if(illegalCommand)
      std::cout << "ILLEGAL_COMMAND: " << illegalCommand << std::endl;
   if(error)
      std::cout << "ERROR: "           << error << std::endl;

   std::cout << "CURRENT_STATE: "     << currentState << std::endl;
   
   if(readyForData)
      std::cout << "READY_FOR_DATA: "    << readyForData << std::endl;

   if(appCmd)
      std::cout << "APP_CMD: "       << appCmd << std::endl;
   
   if(akeSeqError)
      std::cout << "AKE_SEQ_ERROR: " << akeSeqError << std::endl;

   return true;
}