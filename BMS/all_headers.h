#ifndef ALL_HEADERS_H
#define ALL_HEADERS_H

/*================================================================
all_headers.h
==================================================================*/

//standard headers
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

//MACROS
#define INTERFACENAMESIZE 50


//MACORS for color printing
#define BLACK  "\033[30m" 
#define BLUE    "\033[34m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"

#define HIGHLIGHT "\033[33m\033[41m"
#define MAXHIGHLIGHT "\033[31m\033[43m"
#define MINHIGHLIGHT "\033[34m\033[43m"


//MACORS for setting cursor position
#define CLEAR_SCREEN "\033[1J"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"
#define SET_CURSOR_UL "\033[H"
#define CURSOR_UP "\033[1A"

//MACROS for setting default values for dbc
#define BATTERY_CELLS 192
#define VOLTAGE_MIN 2.5
#define VOLTAGE_MAX 4.5
#define CELLS_IN_LINE 10
#define CELL_CAPACITY 4.07611f
#define SOC_TAPER_START 80.0
#define SOC_TAPER_END 98.0

#define COULOMBIC_EFFICIENCY 1
#define DELTA_TIME 1

//userdefined headers
#include "dbc.h"
#include "./ocv_soc/ocv_soc_t.h"

#endif // ALL_HEADERS_H