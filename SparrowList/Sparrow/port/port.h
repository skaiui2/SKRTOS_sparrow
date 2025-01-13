
#ifndef PORT_H
#define PORT_H

#include "stdint.h"

uint32_t  xEnterCritical( void );
void xExitCritical( uint32_t xReturn );
void StartFirstTask(void);

#endif


