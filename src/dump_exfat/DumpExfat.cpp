//exfat dumping code is based on this code
//https://rshullic.wordpress.com/2010/01/08/sample-c-program-to-parse-out-the-exfat-directory-structure/
//though I had very heavily changed it

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>

#include <boost/filesystem.hpp>

#include <unordered_map>

#include "ExFatTypes.h"
#include "DumpExfat.h"

//The largest cluster number allowed
#define F6 0xFFFFFFF6

//This marks a bad cluster
#define F7 0xFFFFFFF7

//End of FAT Chain
#define FF 0xFFFFFFFF

//No FAT Chain Indicator
#define ZE 0x00000000

std::vector<std::vector<uint8_t> > BitmapTableDataSectors;
std::vector<std::vector<uint8_t> > UpCaseTableDataSectors;

bool DirInProgress = false;
bool FileInProgress = false;

std::vector<psvcd::DirTableEntry> DIRTBL;
int32_t  DIRTBL_Index = 0;

std::vector<psvcd::FileTableEntry> FILETBL;
int32_t FILETBL_Index = 0;

signed int NameOffset;

uint32_t  CurrentClus; //Cluster Being Processed
uint32_t  NextCluster; //for computing the next cluster in a chain

bool ReadSector(std::ifstream& dumpStream, const psvcd::VBR& vbr, uint32_t sectorAddress, std::vector<uint8_t>& resp_data)
{
   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);

   resp_data.resize(BperS);
   dumpStream.seekg((sectorAddress * BperS), std::ios_base::beg);
   dumpStream.read((char*)resp_data.data(), BperS);

   return true;
}

bool ReadSector(std::ifstream& dumpStream, int32_t BperS, uint32_t sectorAddress, std::vector<uint8_t>& resp_data)
{
   resp_data.resize(BperS);
   dumpStream.seekg((sectorAddress * BperS), std::ios_base::beg);
   dumpStream.read((char*)resp_data.data(), BperS);

   return true;
}

bool DumpFsSonyRoot(std::ifstream& dumpStream, psvcd::FsSonyRoot* root)
{
   dumpStream.seekg(0x0000000000000000, std::ios_base::beg);
   memset(root, 0, sizeof(psvcd::FsSonyRoot));
   dumpStream.read((char*)root, sizeof(psvcd::FsSonyRoot));
   return true;
}

bool DumpVBR(std::ifstream& dumpStream, int32_t BperS, uint32_t vbr_address, psvcd::VBR* vbr)
{
   int64_t address = BperS * vbr_address;
   dumpStream.seekg(address, std::ios_base::beg);
   memset(vbr, 0, sizeof(psvcd::VBR));
   dumpStream.read((char*)vbr, sizeof(psvcd::VBR));
   return true;
}

bool DumpFAT(std::ifstream& dumpStream, const psvcd::VBR& vbr, uint32_t fat_address, psvcd::FAT* fat)
{
   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);

   int64_t address = BperS * fat_address;
   dumpStream.seekg(address, std::ios_base::beg);
   memset(fat, 0, sizeof(psvcd::FAT));
   dumpStream.read((char*)fat, sizeof(psvcd::FAT));
   return true;
}

//This function will look at all FAT entries and count cell variations

void FatWalk(std::ifstream& dumpStream, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr)
{
   uint32_t CntFF = 0; //Count End Of Chains
   uint32_t CntZE = 0; //Count Zero Records
   uint32_t CntF7 = 0; //Count Bad Blocks
   uint32_t NonZero = 0; //All Else

   psvcd::FAT fat;
   uint32_t prevFatSector = -1;

   for (uint32_t i = 2; i < vbr.ClusterCount + 2; i++)
   {
      uint32_t sectorOffset = i / 128; //there are 128 cluster records per sector
      uint32_t elementOffset = i % 128; //cluster record index
      uint32_t firstFatSector = fsRoot.FsOffset + vbr.FatOffset;
      uint32_t currentFatSector = firstFatSector + sectorOffset;
      
      if(prevFatSector != currentFatSector)
      {
         if(!DumpFAT(dumpStream, vbr, currentFatSector, &fat))
            throw std::runtime_error("Failed to dump FAT");
         prevFatSector = currentFatSector;
      }
      
      switch(fat.Data[elementOffset])
      {
      case (0xFFFFFFFF) : 
         CntFF++;
         break;
      case (0x00000000) : 
         CntZE++;
         break;
      case (0xFFFFFFF7) : 
         CntF7++;
         break;
      default : 
         NonZero++;
         break;
      }
   }
    
   std::cout << "FF (End Of Chains): " << CntFF << " F7 (Bad Clusters): " << CntF7 << " Cell Contains Zero: " << CntZE << " NonZero (Remaining Non-Zero Cells): " << NonZero << std::endl;
}

//This function will walk the FAT chain and print the clusters in order

uint32_t WalkFatChain(std::ifstream& dumpStream, uint32_t FirstCluster, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr)
{
    uint32_t FatCellAddr; //For current Cell Address
        
    uint32_t FatCellData = FirstCluster; //To Hold FAT Data //Initialize to unused cell flag //Copy starting Cluster
    uint32_t ChainSize = 0;  //To Count number of clusters in chain //Initialize chain cell count

    //If there is only one cluster, then the cell contents should be zero, nothing to do
    //Therefore, the first cell should not be end of chain

    uint32_t sectorOffset = FirstCluster / 128; //there are 128 cluster records per sector
    uint32_t elementOffset = FirstCluster % 128; //cluster record index
    uint32_t firstFatSector = fsRoot.FsOffset + vbr.FatOffset;
    uint32_t currentFatSector = firstFatSector + sectorOffset;

    psvcd::FAT fat;
    if(!DumpFAT(dumpStream, vbr, currentFatSector, &fat))
       throw std::runtime_error("Failed to dump FAT");

    while(FatCellData != FF) //stop at end of chain
    {
       FatCellData = fat.Data[elementOffset];
       
       if (FatCellData == 0) 
         return(ChainSize); //if we hit a zero, nothing to do

       throw std::runtime_error("not implemented");

       std::cout << "   Address of Cell: " << FatCellAddr << " Hex Contents of Cell: " << FatCellData << "X Dec Contents of Cell: " << FatCellData << std::endl;
       ChainSize++; //Count The Cell
    }
    
    return(ChainSize); //Tell caller how many clusters in chain
}

//This functions will return a 1 if the cluster is allocated and 0 if the cluster is not allocated

bool BitMap(uint32_t cluster)
{
   //For Historical Reasons, First Cluster is at Index 2   

   uint32_t ClusOffset = cluster - 2; //First Cluster is at index 
   uint32_t sectorOfst = ClusOffset / 4096; //there are 512 * 8 = 4096 cluster bit records per sector
   uint32_t bitOfst = ClusOffset % 4096;
   std::vector<uint8_t> bitmapSector = BitmapTableDataSectors[sectorOfst];
   
   uint32_t byteOfst = bitOfst / 8;
   uint32_t bitBitOffset = bitOfst % 8;
   uint8_t BitMapByte = bitmapSector[byteOfst];
   
   uint32_t bitTest = (uint32_t)pow(2, bitBitOffset);
   return((BitMapByte & bitTest) == bitTest);
}

//This functions will Dump the entire allocation map

void DumpBitMap(std::ifstream& dumpStream, const psvcd::VBR& vbr)
{
   uint32_t Blocks = vbr.ClusterCount;

   int64_t StartRange = -1;
   int64_t EndRange = -1;
   int64_t InAlloc = -1;
   int32_t InUnAlloc = -1;
   int64_t CntAlloc = 0;
   int64_t CntUnAlloc = 0;

   uint32_t i = 2;

   for (i; i <= Blocks + 1; i++)
   {
      bool AllocStat = BitMap(i);
      if (AllocStat)
      {
         CntAlloc++; //Count Allocated Cluster Units

         if (InUnAlloc == 1) //we were in an unallocation range
         {
               std::cout << "-" << (i - 1) << " unallocated clusters" << std::endl; //Close unallocation range
               std::cout << i << std::endl; //Open new allocation range
               InUnAlloc = 0;
               InAlloc = 1;
         }

         if ((InAlloc == -1) | (InUnAlloc == -1)) //we are in an allocation range
         {
               std::cout << i << std::endl; //Open new allocation range
               InUnAlloc = 0;
               InAlloc = 1;
         }
      }

      if (!AllocStat)
      {
         CntUnAlloc++; //Count Free Blocks

         if (InAlloc == 1) //we were in an allocation range
         {
               std::cout << "-" << (i - 1) << " allocated clusters" << std::endl; //Close allocation range
               std::cout << i << std::endl; //Open new unallocation range
               InUnAlloc = 1;
               InAlloc = 0;
         }
            
         if ((InAlloc == -1) | (InUnAlloc == -1)) //we are in the first pass
         {
               std::cout << i << std::endl; //Open new unallocation range
               InUnAlloc = 1;
               InAlloc = 0;
         }
      }
   }

   if (InAlloc == 1) //we were in an unallocation range
   {
        std::cout << "-" << (i - 1) << " allocated clusters" << std::endl; //Close unallocation range
   }

   if (InUnAlloc == 1) //we were in an unallocation range
   {
       std::cout << "-" << (i - 1) << " unallocated clusters" << std::endl; //Close unallocation range
   }

   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);
   int32_t SperC = (int32_t)pow(2, vbr.SectorsPerClusterShift);
   int32_t BperC = BperS * SperC;
  
   std::cout << std::endl << std::endl << "Printing Simulated chkdsk totals" << std::endl;
   std::cout << std::endl << std::endl << BperC << " bytes in each allocation unit." << std::endl;
   std::cout << Blocks << " Total allocation units on disk." << std::endl << CntUnAlloc << " Allocation units available on disk." << std::endl << CntAlloc << " Allocation units in use." << std::endl;
}

void dump_other(std::ifstream& dumpStream, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr)
{
    //cout << "Dumping Bit Allocation Map" << endl;
    //DumpBitMap(ftHandle, VBR);

    //cout << "Analyzing 1st FAT" << endl;
    //FatWalk(ftHandle, fsRoot, vbr);

    //lseek(FD, 0, SEEK_SET);
    //read(FD, &MainVBR, 6144);
    //cout << "VBR Checksum Calculation " << VBRChecksum(&MainVBR[0], 5632) << endl;
}

uint32_t get_sectorAddress(const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr, uint32_t cluster)
{
   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);
   int32_t SperC = (int32_t)pow(2, vbr.SectorsPerClusterShift);
   int32_t BperC = BperS * SperC;

   uint32_t HeapOffsetClus = vbr.ClusterHeapOffset / SperC;

   uint64_t byteLoc = (HeapOffsetClus + cluster - 2) * BperC;    
   return (uint32_t)((byteLoc / BperS) + fsRoot.FsOffset);
}

void parse_volume_record(int32_t& sectorOffset, const uint8_t* rootLocData)
{
    std::cout << "Volume Record" << std::endl;
    
    psvcd::VolumeLabelDirectoryEntry  vlde;
    memset(&vlde, 0, sizeof(psvcd::VolumeLabelDirectoryEntry));

    const uint8_t* start = rootLocData + sectorOffset;
    memcpy(&vlde, start, sizeof(psvcd::VolumeLabelDirectoryEntry));
    sectorOffset = sectorOffset + sizeof(psvcd::VolumeLabelDirectoryEntry);

    std::cout << "Volume Label Size is " << (int)vlde.CharacterCount << " characters, Label:" << std::endl;
    std::wcout << std::wstring(vlde.VolumeLabel, vlde.CharacterCount) << std::endl;
}

void parse_bitmap_table_record(std::ifstream& dumpStream, int32_t& sectorOffset, const uint8_t* rootLocData, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr)
{
   int32_t SperC = (int32_t)pow(2, vbr.SectorsPerClusterShift);

   std::cout << "Bitmap Table Record" << std::endl;

   psvcd::AllocationBitMapEntry abme;
   memset(&abme, 0, sizeof(psvcd::AllocationBitMapEntry));

   const uint8_t* start = rootLocData + sectorOffset;
   memcpy(&abme, start, sizeof(psvcd::AllocationBitMapEntry));
   sectorOffset = sectorOffset + sizeof(psvcd::AllocationBitMapEntry);

   std::cout << "Bit Map First Cluster " << abme.FirstCluster << std::endl;
   std::cout << "Bit Map Data Length " << abme.DataLength << std::endl;
   std::cout << "Bit Map Flags " << abme.BitMapFlags << std::endl;

   if ((abme.BitMapFlags & 01) == 00) 
      std::cout << " First Bitmap Allocation" << std::endl;
   if ((abme.BitMapFlags & 01) == 01) 
      std::cout << " Second Bitmap Allocation" << std::endl;

   uint32_t BitMapSectorLoc = get_sectorAddress(fsRoot, vbr, abme.FirstCluster);

   std::cout << "Bit Map Sector Offset into Image is: " << BitMapSectorLoc << std::endl;

   //dump of bitmap table should be done before calling BitMap function!
   for(int32_t i = 0; i < SperC; i++)
   {
      std::vector<uint8_t> resp_data;
      ReadSector(dumpStream, vbr, BitMapSectorLoc, resp_data);

      BitmapTableDataSectors.push_back(resp_data);
      BitMapSectorLoc++;
   }
   
   if (BitMap(abme.FirstCluster)) 
      std::cout << "Cluster " << abme.FirstCluster << " is Allocated" << std::endl;

   if (!BitMap(abme.FirstCluster)) 
      std::cout << "Cluster " << abme.FirstCluster << " is Not Allocated" << std::endl;
}

void parse_up_case_table_record(std::ifstream& dumpStream, int32_t& sectorOffset, const uint8_t* rootLocData, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr)
{
   int32_t SperC = (int32_t)pow(2, vbr.SectorsPerClusterShift);

   std::cout << "Up Case Table Record" << std::endl;

   psvcd::UpCaseTableEntry ucte;
   memset(&ucte, 0, sizeof(psvcd::UpCaseTableEntry));

   const uint8_t* start = rootLocData + sectorOffset;
   memcpy(&ucte, start, sizeof(psvcd::UpCaseTableEntry));
   sectorOffset = sectorOffset + sizeof(psvcd::UpCaseTableEntry);

   std::cout << "UP Case Table Checksum " << ucte.TableChecksum << std::endl;
   std::cout << "Up Case Table First Cluster " << ucte.FirstCluster << std::endl;
   std::cout << "Up Case Table Data Length " << ucte.DataLength << std::endl;

   uint32_t UPCaseTableSectorLoc = get_sectorAddress(fsRoot, vbr, ucte.FirstCluster);

   std::cout << "UP Case Table Sector Offset into Image is: " << UPCaseTableSectorLoc << std::endl;

   if (BitMap(ucte.FirstCluster)) 
       std::cout << "Cluster " << ucte.FirstCluster << " is Allocated" << std::endl;
        
   if (!BitMap(ucte.FirstCluster)) 
       std::cout << "Cluster " << ucte.FirstCluster << " is Not Allocated" << std::endl;

   /*
   for(int32_t i = 0; i < SperC; i++)
   {
      std::vector<BYTE> resp_data;
      CMD17_Retry(ftHandle, UPCaseTableSectorLoc, resp_data);

      UpCaseTableDataSectors.push_back(resp_data);
      UPCaseTableSectorLoc++;
   }
   */

   /*
   cout << "Up-Case Table Checksum Calculation " << UPCaseChecksum(&UPCaseTable[0], UpCaseTableEntry.DataLength) << endl;
   */
}

void parse_directory_entry_record(int32_t& sectorOffset, const uint8_t* rootLocData, uint8_t RootType, uint32_t parentCluster)
{
   psvcd::DirectoryEntry de;
   memset(&de, 0, sizeof(psvcd::DirectoryEntry));

   const uint8_t* start = rootLocData + sectorOffset;
   memcpy(&de, start, sizeof(psvcd::DirectoryEntry));
   sectorOffset = sectorOffset + sizeof(psvcd::DirectoryEntry);

   //uint16_t   CalcChecksum; //returned checksum

   long   TS_Year;
   long   TS_Month;
   long   TS_Day;
   long   TS_Hours;
   long   TS_Minutes;
   long   TS_DoubleSeconds;
    
   long   Date_Work;
    
   unsigned int tz;
   unsigned int tzp;

   if (RootType == 005) 
      std::cout << "Directory Entry Record (Deleted)" << std::endl;  //X’05’
   if (RootType == 133) 
      std::cout << "Directory Entry Record" << std::endl; //X’85’
              
   DirInProgress = false; //Not a Directory vs. a File
   FileInProgress = false;
   
   /*
   lseek (FD, RootLoc, SEEK_SET); //re-seek to start of 85 entry
   ReadCnt = read(FD, &DirEntrySet, 608);
   cout << "Checksum: " << de.SetChecksum[1] << de.SetChecksum[0] << endl;

   ReadCnt = (de.SecondaryCount + 1) * 32;
   cout << "Calculated Checksum is: " << (CalcChecksum = EntrySetChecksum(&DirEntrySet[0], ReadCnt)) << " Size Directory Set (bytes): " << ReadCnt << endl;
   */

   std::cout << "Secondary Count " << (int)de.SecondaryCount << std::endl;
   std::cout << "File Attributes: " << (int)de.FileAttributes[1] << (int)de.FileAttributes[0] << std::endl;

   if (((de.FileAttributes[0] & 16) == 16) && (RootType == 133))
   {
      std::cout << " Directory " << std::endl;
      DirInProgress = true; //Processing a chained dir
      DIRTBL_Index++; //record a directory entry
      NameOffset = 0; //indexing for directory name
      DIRTBL.push_back(psvcd::DirTableEntry());
      DIRTBL[DIRTBL_Index].DirEntCode = RootType; //save in diretory table
      DIRTBL[DIRTBL_Index].DirParent = parentCluster; //Save Parent Cluster Value
   }
   else
   {
      std::cout << " File " << std::endl;
      FileInProgress = true;
      FILETBL_Index++;
      NameOffset = 0;
      FILETBL.push_back(psvcd::FileTableEntry());
      FILETBL[FILETBL_Index].DirEntCode = RootType; //save in diretory table
      FILETBL[FILETBL_Index].DirParent = parentCluster; //Save Parent Cluster Value
   }

   if ((de.FileAttributes[0] & 04) == 04) 
      std::cout << " System " << std::endl;
   if ((de.FileAttributes[0] & 02) == 02) 
      std::cout << " Hidden " << std::endl;
   if ((de.FileAttributes[0] & 01) == 01) 
      std::cout << " Read/Only " << std::endl;
   if ((de.FileAttributes[0] & 32) == 32) 
      std::cout << " Archive " << std::endl;
   std::cout << std::endl;

   std::cout << "Create Timestamp: " << de.CreateTimestamp << std::endl;
   Date_Work = de.CreateTimestamp; //get a timestamp
   TS_DoubleSeconds = Date_Work %  32; // Get 5 bits
   TS_DoubleSeconds = TS_DoubleSeconds * 2; // Double it
   Date_Work = Date_Work / 32; // Shift 5 bits
   TS_Minutes = Date_Work % 64; // Get 6 bits
   Date_Work = Date_Work / 64; // Shift 6 bits
   TS_Hours = Date_Work % 32; // Get 5 bits
   Date_Work = Date_Work /32; // Shift 5 bits
   TS_Day = Date_Work % 32; // get 5 Bits
   Date_Work = Date_Work /32; // Shift 5 Bits
   TS_Month = Date_Work % 16; // Get 4 bits
   Date_Work = Date_Work / 16; // Shift 4 bits
   TS_Year = Date_Work + 1980; // Year is 1980+
   std::cout << TS_Month << "/" << TS_Day << "/" << TS_Year << " " << TS_Hours << ":" << TS_Minutes << ":" << TS_DoubleSeconds << std::endl;

   std::cout << "Last Modified Timestamp: " << de.LastModifiedTimestamp << std::endl;
   Date_Work = de.LastModifiedTimestamp; // get a timestamp
   TS_DoubleSeconds = Date_Work %  32; // Get 5 bits
   TS_DoubleSeconds = TS_DoubleSeconds * 2; // Double it
   Date_Work = Date_Work / 32; // Shift 5 bits
   TS_Minutes = Date_Work % 64; // Get 6 bits
   Date_Work = Date_Work / 64; // Shift 6 bits
   TS_Hours = Date_Work % 32; // Get 5 bits
   Date_Work = Date_Work /32; // Shift 5 bits
   TS_Day = Date_Work % 32; // get 5 Bits
   Date_Work = Date_Work /32; // Shift 5 Bits
   TS_Month = Date_Work % 16; // Get 4 bits
   Date_Work = Date_Work / 16; // Shift 4 bits
   TS_Year = Date_Work + 1980; // Year is 1980+
   std::cout << TS_Month << "/" << TS_Day << "/" << TS_Year << " " << TS_Hours << ":" << TS_Minutes << ":" << TS_DoubleSeconds << std::endl;

   std::cout << "Last Accessed Timestamp: "<< de.LastAccessedTimestamp << std::endl;
   Date_Work = de.LastAccessedTimestamp; //get a timestamp
   TS_DoubleSeconds = Date_Work %  32; // Get 5 bits
   TS_DoubleSeconds = TS_DoubleSeconds * 2; // Double it
   Date_Work = Date_Work / 32; // Shift 5 bits
   TS_Minutes = Date_Work % 64; // Get 6 bits
   Date_Work = Date_Work / 64; // Shift 6 bits
   TS_Hours = Date_Work % 32; // Get 5 bits
   Date_Work = Date_Work /32; // Shift 5 bits
   TS_Day = Date_Work % 32; // get 5 Bits
   Date_Work = Date_Work /32; // Shift 5 Bits
   TS_Month = Date_Work % 16; // Get 4 bits
   Date_Work = Date_Work / 16; // Shift 4 bits
   TS_Year = Date_Work + 1980; // Year is 1980+
   std::cout << TS_Month << "/" << TS_Day << "/" << TS_Year << " " << TS_Hours << ":" << TS_Minutes << ":" << TS_DoubleSeconds << std::endl;

   std::cout << " 10 ms Offset Create " << de.Create10msIncrement << de.Create10msIncrement << std::endl;
   std::cout << " 10 ms Offset Modified " << de.LastModified10msIncrement << de.LastModified10msIncrement << std::endl;
   std::cout << " Time Zone Create " << de.CreateTZ << de.CreateTZ << std::endl;
   tz = de.CreateTZ, de.CreateTZ;
 
   if (tz >= 208)
   {
      tzp = ((256-tz) % 4) * 15; tz = (256-tz) / 4;
      std::cout << " Value of tz is: GMT -" << tz << ":" << tzp << std::endl;
   }
   else if ((tz >= 128) && (tz <= 192))
   {
      tzp = ((tz-128) % 4) * 15; tz = (tz-128) / 4; 
      std::cout << " Value of tz is: GMT +" << tz << ":" << tzp << std::endl;
   }
   else 
   {
      std::cout << " NOT UTC Timestamp" << std::endl;
   }

   std::cout << " Time Zone Modified " << de.LastModifiedTZ << de.LastModifiedTZ << std::endl;
   tz = de.CreateTZ, de.LastModifiedTZ;
    
   if (tz >= 208)
   {
      tzp = ((256-tz) % 4) * 15;  tz = (256-tz) / 4; 
      std::cout << " Value of tz is: GMT -" << tz << ":" << tzp << std::endl;
   }
   else if ((tz >= 128) && (tz <= 192))
   {
      tzp = ((tz-128) % 4) * 15; tz = (tz-128) / 4; 
      std::cout << " Value of tz is: GMT +" << tz << ":" << tzp << std::endl;
   }
   else 
   {
      std::cout << " NOT UTC Timestamp" << std::endl;
   }

   std::cout << " Time Zone Last Accessed " << de.LastAccessedTZ << de.LastAccessedTZ << std::endl;
   tz = de.CreateTZ, de.LastAccessedTZ;
    
   if (tz >= 208)
   {
      tzp = ((256-tz) % 4) * 15;  tz = (256-tz) / 4; 
      std::cout << " Value of tz is: GMT -" << tz << ":" << tzp << std::endl;
   }
   else if ((tz >= 128) && (tz <= 192))
   {
      tzp = ((tz-128) % 4) * 15; tz = (tz-128) / 4; 
      std::cout << " Value of tz is: GMT +" << tz << ":" << tzp << std::endl;
   }
   else 
   {
      std::cout << " NOT UTC Timestamp" << std::endl;
   }
}

void parse_directory_entry_stream_ex_record(std::ifstream& dumpStream, int32_t& sectorOffset, const uint8_t* rootLocData, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr, uint8_t RootType)
{
   psvcd::StreamExtensionDirectoryEntry sede;
   memset(&sede, 0, sizeof(psvcd::StreamExtensionDirectoryEntry));

   const uint8_t* start = rootLocData + sectorOffset;
   memcpy(&sede, start, sizeof(psvcd::StreamExtensionDirectoryEntry));
   sectorOffset = sectorOffset + sizeof(psvcd::StreamExtensionDirectoryEntry);

   if (RootType == 64)  
      std::cout << "Directory Entry Record, Stream Extension (Deleted)" << std::endl;
   if (RootType == 192) 
      std::cout << "Directory Entry Record, Stream Extension" << std::endl;

   std::cout << "Secondary Flags: " << (int)sede.GeneralSecondaryFlags << std::endl;
    
   if ((sede.GeneralSecondaryFlags & 01) == 01) 
      std::cout << " Flag Bit 0: Allocation Possible" << std::endl;
        
   if ((sede.GeneralSecondaryFlags & 01) != 01) 
      std::cout << " Flag Bit 0: Allocation Not Possible" << std::endl;
        
   if ((sede.GeneralSecondaryFlags & 02) != 02) 
      std::cout << " Flag Bit 1: FAT Chain Valid" << std::endl;
        
   if ((sede.GeneralSecondaryFlags & 02) == 02) 
      std::cout << " Flag Bit 1: FAT Chain Invalid" << std::endl;

   std::cout << "Length of UniCode Filename is: " << (int)sede.NameLength << std::endl;
   std::cout << "Name Hash Value is: " << (int)sede.NameHash[0] << (int)sede.NameHash[1] << std::endl;

   if (sede.FirstCluster == 0)
      std::cout << "Stream Extension First Cluster Not Defined" << std::endl;

   uint32_t FileSectorLoc =  get_sectorAddress(fsRoot, vbr, sede.FirstCluster);

   if (sede.FirstCluster != 0)
   {
      std::cout << "Stream Extension First Cluster " << sede.FirstCluster << " Sector Location: " << FileSectorLoc << std::endl;

      /*
      if (BitMap(sede.FirstCluster)) 
         cout << "Cluster " << sede.FirstCluster << " is Allocated" << endl;

      if (!BitMap(sede.FirstCluster)) 
         cout << "Cluster " << sede.FirstCluster << " is Not Allocated" << endl;
      */
   }

   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);
   int32_t SperC = (int32_t)pow(2, vbr.SectorsPerClusterShift);
   int32_t BperC = BperS * SperC;

   uint64_t BlkTemp = sede.DataLength / BperC; //compute number of blocks
   uint64_t BlkDiff = sede.DataLength - (BlkTemp * BperC); //compute slack

   if (BlkDiff > 0) 
      BlkTemp++; //compute true number of blocks

   std::cout << "Stream Extension Data Length " << sede.DataLength << std::endl;
   std::cout << "Bytes Slack: " << BlkDiff << std::endl;
   std::cout << "Clusters Used: " << BlkTemp << std::endl;

   BlkTemp = sede.ValidDataLength / BperC; //compute number of blocks
   BlkDiff = sede.ValidDataLength - (BlkTemp * BperC); //compute slack
    
   if (BlkDiff > 0) 
      BlkTemp++; //compute true number of blocks

   std::cout << "Stream Extension Valid Data Length " << sede.ValidDataLength << std::endl;
   std::cout << "Bytes Slack: " << BlkDiff << std::endl;
   std::cout << "Clusters Used: " << BlkTemp << std::endl;

   if (DirInProgress)
   {
      DIRTBL[DIRTBL_Index].DirFirstClusterAddr = sede.FirstCluster;
      memset(DIRTBL[DIRTBL_Index].DirEntryName, 0, 512);
   }

   if(FileInProgress)
   {
      FILETBL[FILETBL_Index].DirFirstClusterAddr = sede.FirstCluster;
      FILETBL[FILETBL_Index].DataLength = sede.DataLength;
      FILETBL[FILETBL_Index].ValidDataLength = sede.ValidDataLength;
      memset(FILETBL[FILETBL_Index].FileEntryName, 0, 512);
   }

   /*
   if (((sede.GeneralSecondaryFlags & 02) == 02) & (RootType == 192))
   {
      std::cout << endl << "Dumping FAT Chain" << std::endl; //Title Line
      uint32_t chainSize =  WalkFatChain(ftHandle, sede.FirstCluster, fsRoot, vbr); //Traverse the Linked List
      std::cout << "Size of Chain is: " << chainSize << std::endl; //Print Chain Size
   }
   */
}

void parse_directory_entry_file_name_ex_record(int32_t& sectorOffset, const uint8_t* rootLocData, uint8_t RootType)
{
   psvcd::FileNameDirectoryEntry fnde;
   memset(&fnde, 0, sizeof(psvcd::FileNameDirectoryEntry));

   const uint8_t* start = rootLocData + sectorOffset;
   memcpy(&fnde, start, sizeof(psvcd::FileNameDirectoryEntry));
   sectorOffset = sectorOffset + sizeof(psvcd::FileNameDirectoryEntry);

   if (RootType == 65) 
      std::cout << "Directory Entry Record, File Name Extension (Deleted)" << std::endl;
   if (RootType == 193) 
      std::cout << "Directory Entry Record, File Name Extension" << std::endl;

   std::cout << "Secondary Flags: " << fnde.GeneralSecondaryFlags << std::endl;

   if ((fnde.GeneralSecondaryFlags & 01) == 01) 
      std::cout << " Flag Bit 0: Allocation Possible" << std::endl;
        
   if ((fnde.GeneralSecondaryFlags & 01) != 01) 
      std::cout << " Flag Bit 0: Allocation Not Possible" << std::endl;
        
   if ((fnde.GeneralSecondaryFlags & 02) != 02) 
      std::cout << " Flag Bit 1: FAT Chain Valid" << std::endl;
        
   if ((fnde.GeneralSecondaryFlags & 02) == 02) 
      std::cout << " Flag Bit 1: FAT Chain Invalid" << std::endl;

   if (DirInProgress) 
   {
      for (int i = 0; i < 30; i++)
      {
         DIRTBL[DIRTBL_Index].DirEntryName[NameOffset] = fnde.FileName[i];
         NameOffset++;
      }
   }

   if(FileInProgress)
   {
      for (int i = 0; i < 30; i++)
      {
         FILETBL[FILETBL_Index].FileEntryName[NameOffset] = fnde.FileName[i];
         NameOffset++;
      }
   }
}

bool parse_root_record(std::ifstream& dumpStream, uint8_t* rootLocData, int32_t& sectorOffset, uint8_t RootType, uint32_t RootIndex, uint32_t RootLocSector, uint32_t FirstClus, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr)
{
   std::cout << std::endl << "Seeking sector: " << RootLocSector << ", For Directory Index: " << RootIndex << ", " << std::endl;

   switch (RootType)
   {
   case 0x01: 
      std::cout << "Bitmap Table Record (Deleted)" << std::endl;
      sectorOffset = sectorOffset + 32;
      break;
   case 0x81: 
      parse_bitmap_table_record(dumpStream, sectorOffset, rootLocData, fsRoot, vbr);
      break;

   case 0x02: 
      std::cout << "Up Case Table Record (Deleted)" << std::endl;
      sectorOffset = sectorOffset + 32;
      break;
   case 0x82: 
      parse_up_case_table_record(dumpStream, sectorOffset, rootLocData, fsRoot, vbr);
      break;
   
   case 0x03: 
      std::cout << "No Volume Label" << std::endl;
      sectorOffset = sectorOffset + 32;
      break;
   case 0x83: 
      parse_volume_record(sectorOffset, rootLocData);
      break;

   case 0x05:
   case 0x85:
      parse_directory_entry_record(sectorOffset, rootLocData, RootType, FirstClus);
      break;

   case 0x40:
   case 0xC0:
      parse_directory_entry_stream_ex_record(dumpStream, sectorOffset, rootLocData, fsRoot, vbr, RootType);
      break;

   case 0x41:
   case 0xC1:
      parse_directory_entry_file_name_ex_record(sectorOffset, rootLocData, RootType);
      break;
   
   case 0xA0: 
      //parse_volume_guid_record();
      //RootLoc = RootLoc + 32;
      break;
   case 0xA1: 
      //parse_tex_fat_record();
      //RootLoc = RootLoc + 32;
      break;
   case 0xA2: 
      //parse_WCE_ACD_record();
      //RootLoc = RootLoc + 32;
      break;
   
   case 0x00: 
      std::cout << "Unused Entry" << std::endl;
      sectorOffset = sectorOffset + 32;
      break;
   default: 
      std::cout << "Unknown" << std::endl;
      sectorOffset = sectorOffset + 32;
      break;
   }

   return true;
}

void DumpRootDirectory(std::ifstream& dumpStream, uint32_t DEperS, uint32_t FirstClus, uint32_t FatSector, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr)
{
   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);

   uint32_t RootLocSector = get_sectorAddress(fsRoot, vbr, FirstClus);

   uint8_t RootType = 255;
   uint32_t RootIndex = 0;
   CurrentClus = FirstClus;

   std::cout << std::endl << "First Cluster being processed is cluster #: " << FirstClus << " Located at sector location: " << RootLocSector << " Fat sector is: " << FatSector << std::endl;

   std::vector<uint8_t> resp_data;
   ReadSector(dumpStream, vbr, RootLocSector, resp_data);

   uint8_t* rootLocData = resp_data.data();
   int32_t sectorOffset = 0;

   while (RootType != 0)
   {
      RootIndex++;

      if (RootIndex > DEperS) 
      {
         std::cout << "End Of Directory Sector Reached" << std::endl;

         //this check does not work. FAT looks like to be empty in exfat

         /*
         //We have to look at the FAT Chain to see if there are any more clusters

         uint32_t clusterSectorOfst = CurrentClus / 128; //there are 128 cluster records per sector
         uint32_t clusterSectorElementOfst = CurrentClus % 128; //cluster record index
         uint32_t firstFatSector = fsRoot.FsOffset + vbr.FatOffset;
         uint32_t currentFatSector = firstFatSector + clusterSectorOfst;

         FAT fat;
         DumpFAT(dumpStream, vbr, currentFatSector, &fat);

         NextCluster = fat.Data[clusterSectorElementOfst];

         cout << "Next Cluster Link is located in the FAT table at: " << clusterSectorElementOfst << endl;

         // we check the chaining of the FAT links, however, we may terminate on a dir entry of zero and not even hit
         // the end of chains, but if the last cluster is actually full, we will hit this code

         // If we are walking the chain, which means that there are at least 2 clusters, then all F’s is end of chain
         // We don’t test here, but if we are actually in the first cluster and hit the all F’s, we actually have a
         // Problem because something isn’t correct
         if (NextCluster == FF)
         {
               cout << "End Of Chain Hit" << endl << endl;
               return;
         }
   
         // If we are at the first cluster, and the FAT chain is zero, then there is no chain at all, and the file is
         // only one cluster in size. However, if we are walking the chain, and hit zero on anything other than the
         // first cluster, then this isn’t correct either and we have a problem.
         if (NextCluster == ZE)
         {
               cout << "No Chain Defined, only One Cluster" << endl << endl;
               return;
         }

         throw runtime_error("Not implemented");
         */

         /*
         RootIndex = 1;
         CurrentClus = NextCluster;
         RootLoc = (_HeapOffsetClus + CurrentClus -2) * _BperC;
         cout << "Next Cluster Address is cluster: " << NextCluster << " New Byte Location is: " << RootLoc << endl
   
         if ( BitMap(FD, CurrentClus))
            cout << "Cluster " << CurrentClus << " is being processed and is Allocated" << endl;
                
         if (!BitMap(FD, CurrentClus)) 
            cout << "Cluster " << CurrentClus << " is being processed and is Not Allocated" << endl
         */

         RootLocSector++;
         resp_data.clear();
         ReadSector(dumpStream, vbr, RootLocSector, resp_data);

         rootLocData = resp_data.data();
         sectorOffset = 0;
         RootIndex = 0;
      }

      RootType = rootLocData[sectorOffset];
      std::cout << std::endl << "Root Entry Type Read is: " << (int)RootType << std::endl;
        
      parse_root_record(dumpStream, rootLocData, sectorOffset, RootType, RootIndex, RootLocSector, FirstClus, fsRoot, vbr);
   }

   return;
}

bool DumpDirectories(std::ifstream& dumpStream, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr)
{
   DIRTBL.push_back(psvcd::DirTableEntry());
   FILETBL.push_back(psvcd::FileTableEntry());

   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);
   int32_t SperC = (int32_t)pow(2, vbr.SectorsPerClusterShift);
   int32_t BperC = BperS * SperC;

   uint32_t FatSector = fsRoot.FsOffset + vbr.FatOffset;

   uint32_t RootLocSector = get_sectorAddress(fsRoot, vbr, vbr.RootDirFirstClust);
   std::cout << std::endl << "The location of the root directory is: " << RootLocSector << std::endl;

   int32_t DEperS = BperS / 32;
   std::cout << std::endl << "Each Directory Sector will have " << DEperS << " Enteries" << std::endl;

   std::cout << ">>>>>> Traverse the Root Directory <<<<<<" << std::endl;

   DumpRootDirectory(dumpStream, DEperS, vbr.RootDirFirstClust, FatSector, fsRoot, vbr);

   std::cout << std::endl << std::endl << std::endl << ">>>>>>>>>>>>>>>>>>>> Traverse Sub Directories Directory <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl << std::endl << std::endl;

   for (int32_t i = 1; i <= DIRTBL_Index; i++)
   {
      uint32_t sectorOffset = get_sectorAddress(fsRoot, vbr, DIRTBL[i].DirFirstClusterAddr);
      std::cout << std::endl << std::endl << "Index: " << i << " Entry Code: " << DIRTBL[i].DirEntCode << " First Cluster Location: " << DIRTBL[i].DirFirstClusterAddr << std::endl;
      std::cout << " Sector Offset: " << sectorOffset << std::endl;
      std::wcout << "Directory: " << std::wstring((wchar_t*)DIRTBL[i].DirEntryName) << std::endl;
      DumpRootDirectory(dumpStream, DEperS, DIRTBL[i].DirFirstClusterAddr, FatSector, fsRoot, vbr);
   }

   // Initialize Parent Index
   for (int32_t i = 1; i <= DIRTBL_Index; i++)
   {
      DIRTBL[i].ParentIndex = 0;
   }

   for (int32_t i = 1; i <= DIRTBL_Index; i++)
   {
      for (int32_t j = 1; j <= DIRTBL_Index; j++)
      {
         if (DIRTBL[i].DirFirstClusterAddr == DIRTBL[j].DirParent)
               DIRTBL[j].ParentIndex = i;
      }
   }

   std::cout << std::endl << std::endl << std::endl << ">>>>>>>>>>>>>>>>>>>> Recap Directory Entries <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl << std::endl << std::endl;

   for (int32_t i = 1; i <= DIRTBL_Index; i++)
   {
      uint32_t sectorOffset = get_sectorAddress(fsRoot, vbr, DIRTBL[i].DirFirstClusterAddr);

      std::cout << "Index: " << i << " Entry Code: " << DIRTBL[i].DirEntCode << " First Cluster Location: " << DIRTBL[i].DirFirstClusterAddr << std::endl;
      std::cout << " Parent Cluster " << DIRTBL[i].DirParent << " Index " << DIRTBL[i].ParentIndex << std::endl;
      std::cout << " Sector Offset: " << sectorOffset << std::endl;
      std::wcout << "Directory: " << std::wstring((wchar_t*)DIRTBL[i].DirEntryName) << std::endl;

      int32_t j = DIRTBL[i].ParentIndex;
        
      while (j != 0)
      {
         std::wcout << " in " << std::wstring((wchar_t*)DIRTBL[j].DirEntryName) << std::endl;
         j = DIRTBL[j].ParentIndex;
      }
        
      if (j == 0) 
         std::cout << " in Root " << std::endl;
   }

   return true;
}

void dump_content(std::ifstream& dumpStream, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr, psvcd::FileTableEntry& fileRecord)
{
   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);

   uint32_t cluster = fileRecord.DirFirstClusterAddr;
   uint64_t dataLength = fileRecord.DataLength;

   std::wstring fileName = std::wstring((wchar_t*)fileRecord.FileEntryName);
   std::wstring filePath = L"D:\\dumps\\files\\" + fileName;

   uint32_t sectorAddress = get_sectorAddress(fsRoot, vbr, cluster);
   uint64_t nSectors = dataLength / BperS;
   uint32_t tailSize = dataLength % BperS;
   
   std::ofstream str(filePath, std::ios::out | std::ios::binary);

   //move to start cluster
   dumpStream.seekg((sectorAddress * BperS), std::ios_base::beg);

   for(uint64_t i = 0; i < nSectors; i++)
   {
      std::vector<uint8_t> resp_data(BperS);
      dumpStream.read((char*)resp_data.data(), BperS);
      
      uint8_t* raw_data = resp_data.data();
      str.write((const char*)raw_data, BperS);

      sectorAddress++;
   }

   std::vector<uint8_t> resp_data_tail;
   dumpStream.read((char*)resp_data_tail.data(), BperS);
   
   uint8_t* raw_data_tail = resp_data_tail.data();
   str.write((const char*)raw_data_tail, tailSize);

   std::cout << std::endl;
}

bool DumpFile(std::ifstream& dumpStream, int64_t byteOffset, uint64_t length, int32_t BperS, std::ofstream& filestream)
{
   dumpStream.seekg(byteOffset);

   int64_t nSectors = length / BperS;
   int64_t nTail = length % BperS;

   std::vector<char> bytes(BperS);

   for(int64_t i = 0; i < nSectors; i++)
   {
      dumpStream.read(bytes.data(), BperS);
      filestream.write(bytes.data(), BperS);
   }

   dumpStream.read(bytes.data(), nTail);
   filestream.write(bytes.data(), nTail);

   return true;
}

bool DumpFiles(std::ifstream& dumpStream, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr, const std::unordered_map<uint32_t, boost::filesystem::path>& fullPaths)
{
   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);

   std::cout << "Files: " << std::endl;

   for(int32_t i = 1; i <= FILETBL_Index; i++)
   {
      uint32_t sectorOffset = get_sectorAddress(fsRoot, vbr, FILETBL[i].DirFirstClusterAddr);
      std::wstring fileName((wchar_t*)FILETBL[i].FileEntryName);
      int64_t byteOffset = sectorOffset * BperS;
      uint64_t dataLength = FILETBL[i].DataLength;

      uint32_t parent = FILETBL[i].DirParent;
      std::unordered_map<uint32_t, boost::filesystem::path>::const_iterator it = fullPaths.find(parent);
      if(it == fullPaths.end())
      {
         std::cout << "Path is missing" << std::endl;
         continue;
      }

      boost::filesystem::path directoryPath = it->second;
      boost::filesystem::path filePath = directoryPath / boost::filesystem::path(fileName);

      std::ofstream filestream(filePath.generic_wstring(), std::ios::out | std::ios::binary);

      DumpFile(dumpStream, byteOffset, dataLength, BperS, filestream);
   }

   return true;
}

std::wstring ConcatPaths(const std::vector<std::wstring>& paths)
{
   std::wstring path;

   for(std::vector<std::wstring>::const_reverse_iterator it = paths.rbegin(); it != paths.rend(); ++it)
   {
      path +=  (*it) + L"/";
   }

   return path;
}

void CreateDirectories(boost::filesystem::path rootPath, std::unordered_map<uint32_t, boost::filesystem::path>& fullPaths)
{
   std::unordered_map<uint32_t, std::wstring> paths;

   std::set<boost::filesystem::path> uniquePaths;

   for (int32_t i = 1; i <= DIRTBL_Index; i++)
   {
      std::vector<std::wstring> pathParts;

      std::wstring dirNameW((wchar_t*)DIRTBL[i].DirEntryName);
      pathParts.push_back(dirNameW);
      
      //wcout << "Directory: " << dirNameW << endl;
      int32_t j = DIRTBL[i].ParentIndex;

      uint32_t clusterAddress = DIRTBL[i].DirFirstClusterAddr;
        
      while (j != 0)
      {
         std::wstring internalDirNameW((wchar_t*)DIRTBL[j].DirEntryName);
         pathParts.push_back(internalDirNameW);

         //wcout << " in " << internalDirNameW << endl;
         j = DIRTBL[j].ParentIndex;
      }
        
      if (j == 0)
      {
         //cout << " in Root " << endl;
         //paths.push_back(L"root");
      }

      std::wstring p =  ConcatPaths(pathParts);
      std::wcout << p << std::endl;

      paths.insert(std::make_pair(clusterAddress, p));
      uniquePaths.insert(p);

      pathParts.clear();
   }

   //create real directories
   for(std::set<boost::filesystem::path>::const_iterator it = uniquePaths.begin(); it != uniquePaths.end(); ++it)
   {
      boost::filesystem::path fullPath = rootPath / boost::filesystem::path(*it);
      
      if(!boost::filesystem::exists(fullPath))
         boost::filesystem::create_directory(fullPath);
   }

   //link full paths to cluster addresses
   for(std::unordered_map<uint32_t, std::wstring>::const_iterator it = paths.begin(); it != paths.end(); ++it)
   {
      boost::filesystem::path fullPath = rootPath / boost::filesystem::path(it->second);
      fullPaths.insert(std::make_pair(it->first, fullPath));
   }   
}

void DumpMMCCard(boost::filesystem::path srcFilePath, boost::filesystem::path destRootPath)
{
   std::ifstream dumpStream(srcFilePath.generic_string().c_str(), std::ios::in | std::ios::binary);

   psvcd::FsSonyRoot fsRoot;
   if(!DumpFsSonyRoot(dumpStream, &fsRoot))
      return;

   int32_t BperS0 = (int32_t)pow(2, fsRoot.BytesPerSectorShift); //Bytes per Sector

   psvcd::VBR vbr;
   if(!DumpVBR(dumpStream, BperS0, fsRoot.FsOffset, &vbr))
      return;

   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift); //Bytes per Sector
   int32_t SperC = (int32_t)pow(2, vbr.SectorsPerClusterShift); //Sectors per Cluster
   int32_t BperC = BperS * SperC; //Bytes per Cluster

   psvcd::FAT fat;
   uint32_t FatSector = fsRoot.FsOffset + vbr.FatOffset;
   if(!DumpFAT(dumpStream, vbr, FatSector, &fat))
      return;

   std::cout << std::hex << "Media type:" << fat.Data[0] << std::endl;

   if(!DumpDirectories(dumpStream, fsRoot, vbr))
      return;

   std::unordered_map<uint32_t, boost::filesystem::path> fullPaths;
   CreateDirectories(destRootPath, fullPaths);

   DumpFiles(dumpStream, fsRoot, vbr, fullPaths);
   
   //dump_other(dumpStream, fsRoot, vbr);

   return;
}

