#include <boost/filesystem.hpp>

#include "DumpExfat.h"

int main(int argc, char* argv[])
{
   if(argc < 3)
   {
      std::cout << "Usage: src_dump_file_path dest_dir_path" << std::endl;
      return -1;
   }

   boost::filesystem::path srcFilePath(argv[1]);
   boost::filesystem::path destRootPath(argv[2]);

   DumpMMCCard(srcFilePath, destRootPath);

	return 0;
}