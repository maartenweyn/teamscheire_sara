#ifndef UWB_PARSER_H
#define UWB_PARSER_H

#include <stdlib.h>

void uwb_parser_init();
int uwb_parser_check_data();

void locator_task( void *pvParameters );

#endif
