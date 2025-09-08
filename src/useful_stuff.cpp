#include "useful_stuff.h"

#ifdef DEBUG
LogLvl LOGLVL = LogLvl::DEBUG;
#else
LogLvl LOGLVL = LogLvl::ERR;
#endif
