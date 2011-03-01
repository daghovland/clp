/**
   Only used by the parser to create the original list of constants

   Identity of constants is based on the string name
**/

#ifndef __INCLUDE_CONSTANTS_H
#define __INCLUDE_CONSTANTS_H

#include "common.h"
#include "theory.h"

const char* parser_new_constant(theory*, const char*);

#endif
