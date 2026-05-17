#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

int start_client();
int start_server();