#pragma once

#include <windows.h>
#include <vector>

namespace psvcd {

enum MMC_CardStatus : BYTE
{
   idle = 0,
   ready = 1,
   ident = 2,
   stby = 3,
   tran = 4,
   data = 5,
   rcv = 6,
   prg = 7,
   dis = 8,
   btst = 9,
   slp = 10,
   reserved = 255
};

struct MMC_CardStatusInfo
{
   BYTE cmd;

   bool addressOutOfRange;
   bool addressMisalign;
   bool blockLenError;
   bool eraseSeqError;
   bool eraseParam;
   bool wpViolation;
   bool cardIsLocked;
   bool lockUnlockFailed;

   bool comCrcError;
   bool illegalCommand;
   bool cardEccFailed;
   bool ccError;
   bool error;
   bool underrun;
   bool overrun;
   bool cidCsdOveriwrite;

   bool wpEraseSkip;
   bool reserved14;
   bool eraseReset;
   MMC_CardStatus currentState;
   bool readyForData;

   bool switchError;
   bool reserved6;
   bool appCmd;
   bool reserved4;
   bool reserved3;
   bool reserved2;
   bool reserved1;
   bool reserved0;

   MMC_CardStatusInfo()
      :  cmd(0xFF),
         addressOutOfRange(false),
         addressMisalign(false),
         blockLenError(false),
         eraseSeqError(false),
         eraseParam(false),
         wpViolation(false),
         cardIsLocked(false),
         lockUnlockFailed(false),
         comCrcError(false),
         illegalCommand(false),
         cardEccFailed(false),
         ccError(false),
         error(false),
         underrun(false),
         overrun(false),
         cidCsdOveriwrite(false),
         wpEraseSkip(false),
         reserved14(false),
         eraseReset(false),
         currentState(reserved),
         readyForData(false),
         switchError(false),
         reserved6(false),
         appCmd(false),
         reserved4(false),
         reserved3(false),
         reserved2(false),
         reserved1(false),
         reserved0(false)
   {
   }

   bool is_invalid()
   {
      return addressOutOfRange || addressMisalign || blockLenError || eraseSeqError || eraseParam || wpViolation ||
             cardIsLocked || lockUnlockFailed || comCrcError || illegalCommand || cardEccFailed || ccError ||
             error || underrun || overrun || cidCsdOveriwrite || wpEraseSkip || eraseReset || switchError;
   }
};

bool PrintMMCStatus(const std::vector<BYTE>& bytes);

bool GetMMCStatus(const std::vector<BYTE>& bytes, MMC_CardStatusInfo& info);

};