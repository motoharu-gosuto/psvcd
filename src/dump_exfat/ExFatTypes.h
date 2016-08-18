#pragma once

#include <stdint.h>

#pragma pack(push, 1)

#define SCEIidConstant "Sony Computer Entertainment Inc."

#define EXFATContant "EXFAT   "

namespace psvcd {

struct FsSonyRoot
{
   uint8_t  SCEIid[32];
   uint32_t Unk0;
   uint32_t Unk1;
   uint64_t Unk2;
   uint64_t Unk3;
   uint64_t Unk4;
   uint64_t Unk5;
   uint64_t Unk6;
   uint32_t FsOffset;
   uint32_t VolumeLength;
   uint8_t BytesPerSectorShift; //not sure about this one
   uint8_t unk70;
   uint8_t unk71;
   uint8_t unk72;
   uint32_t Unk8;
   uint32_t Unk9;
   uint32_t Unk10;
   uint32_t Unk11;
   uint32_t Unk12;
   uint8_t  BootCode[398];
   uint8_t  Signature[2];
};

struct VBR
{
   uint8_t    JumpBoot[3];
   uint8_t    FileSystemName[8];
   uint8_t    MustBeZero[53];
   uint64_t   PartitionOffset;
   uint64_t   VolumeLength;
   uint32_t    FatOffset; //sector address
   uint32_t    FatLength; // length in sectors
   uint32_t    ClusterHeapOffset; //sector address
   uint32_t    ClusterCount; //number of clusters
   uint32_t    RootDirFirstClust; //cluster address
   uint32_t    VolumeSerialNumber;
   uint8_t  FileSystemRevision2;
   uint8_t  FileSystemRevision1;
   uint8_t  VolumeFlags[2];
   uint8_t  BytesPerSectorShift;
   uint8_t  SectorsPerClusterShift;
   uint8_t  NumberOfFats;
   uint8_t  DriveSelect;
   uint8_t  PercentInUse;
   uint8_t  Reserved[7];
   uint8_t  BootCode[390];
   uint8_t  Signature[2];
};

struct FAT
{
   uint32_t Data[128];
};

struct VolumeLabelDirectoryEntry
{
    uint8_t   EntryTypeX83;
    uint8_t CharacterCount;
    wchar_t VolumeLabel[11];
    uint8_t Reserved[8];
};

struct AllocationBitMapEntry
{
    uint8_t   EntryTypeX81;
    uint8_t BitMapFlags;
    uint8_t Reserved[18];
    uint32_t FirstCluster;
    uint64_t DataLength;
};

struct UpCaseTableEntry
{
    uint8_t   EntryTypeX82;
    uint8_t Reserved1[3];
    uint32_t TableChecksum;
    uint8_t Reserved2[12];
    uint32_t FirstCluster;
    uint64_t DataLength;
};

struct DirectoryEntry
{
    uint8_t   EntryTypeX85;
    uint8_t SecondaryCount;
    uint8_t SetChecksum[2];
    uint8_t FileAttributes[2];
    
    uint8_t Reserved1[2];
    
    uint32_t   CreateTimestamp;
    uint32_t   LastModifiedTimestamp;
    uint32_t   LastAccessedTimestamp;
    
    uint8_t Create10msIncrement;
    uint8_t LastModified10msIncrement;
    uint8_t CreateTZ;
    uint8_t LastModifiedTZ;
    uint8_t LastAccessedTZ;
    uint8_t Reserved2[7];
};

struct StreamExtensionDirectoryEntry
{
    uint8_t   EntryTypeXC0;
    uint8_t GeneralSecondaryFlags;
    
    uint8_t Reserved1;
    uint8_t   NameLength;
    uint8_t   NameHash[2];
    uint8_t Reserved2[2];
    uint64_t ValidDataLength;
    uint32_t   Reserved3;
    uint32_t FirstCluster;
    uint64_t DataLength;
};

struct FileNameDirectoryEntry
{
    uint8_t EntryTypeXC1;
    uint8_t GeneralSecondaryFlags;
    uint8_t FileName[30];
};

struct DirTableEntry
{
    uint32_t DirFirstClusterAddr;
    uint32_t DirParent;
    uint32_t ParentIndex;
    uint8_t DirEntCode;
    uint8_t DirEntryName[512]; //max filename is 255*2 = 510 + 2 bytes for null terminator
    uint8_t ParentDirectory[512];
};

struct FileTableEntry
{
   uint32_t DirFirstClusterAddr;
   uint32_t DirParent;
   uint8_t DirEntCode;    
   uint8_t FileEntryName[512]; //max filename is 255*2 = 510 + 2 bytes for null terminator
   uint64_t DataLength;
   uint64_t ValidDataLength;
};

};

#pragma pack(pop)