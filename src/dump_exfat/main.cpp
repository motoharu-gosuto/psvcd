#include <boost/filesystem.hpp>

#include "DumpExfat.h"

int main(int argc, char* argv[])
{
   //TODO:add cmd line args
   boost::filesystem::path srcFilePath;
   boost::filesystem::path destRootPath;

   DumpMMCCard(srcFilePath, destRootPath);

	return 0;
}