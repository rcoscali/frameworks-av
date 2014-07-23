
#include <fcntl.h>

#include "MPDParser.h"

int
main(int argc, char *argv[])
{
  char buff[20*1024];

  FILE *fp = fopen(argv[1], "r");
  size_t sz = fread(buff, (20*1024)-1, 1, fp);
  MPDParser parser(argv[1], buff, sz);
}
