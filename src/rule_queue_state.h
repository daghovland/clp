/* rule_queue_state.h

   Copyright 2011 

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA  02110-1301, USA */

/*   Written 2011 by Dag Hovland, hovlanddag@gmail.com  */
#ifndef __INCLUDED_RULE_QUEUE_STATE_H
#define __INCLUDED_RULE_QUEUE_STATE_H

//#include "rete_state_struct.h"
#include "rete_state_single_struct.h"
/**
   An interface for the implementation of a RETE_STATE.
   Used by choose_next_instance in strategy.c
   and by rete_insert.c
**/


typedef union rule_queue_state_t {
  //  rete_net_state* state;
  rete_state_single* single;
} rule_queue_state;
	

#endif
