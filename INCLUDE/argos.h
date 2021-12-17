/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *                                      ARGOS Scada System
 *
 * This file contains the manifest constants used globally within the ARGOS project. 
 * This file needs to tailored to a particular installation and then the system cab
 * be re-made for that particular configuration.
 * 
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
/* ++++++++++++++++++++++++++++++++++++++++++++++
 * Standard UNIX header files needed by almost
 * every ARGOS program ...
 * ++++++++++++++++++++++++++++++++++++++++++++++
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <syslog.h>


/* ++++++++++++++++++++++++++++++++++++++++++++++
 * Which system log file to use ...
 * This needs to be setup in the OS ...
 * ++++++++++++++++++++++++++++++++++++++++++++++
 */

#define ARGOS_LOG  LOG_LOCAL3     
