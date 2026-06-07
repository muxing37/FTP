#pragma one
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>
#include <locale.h>
#include <errno.h>
#include <wchar.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <fcntl.h>

std::vector<std::string> startls(int argc,char **argv,std::string cwd);