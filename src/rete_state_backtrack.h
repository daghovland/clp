/* rete_state_backtrack.h

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

#ifndef __INCLUDE_RETE_STATE_BACKTRACK_H
#define __INCLUDE_RETE_STATE_BACKTRACK_H

#include "common.h"
#include "rete.h"

typedef struct rete_state_backtrack_t {
  unsigned int rule_queue_start;
  unsigned int axm_inst_queue_start[];
} rete_state_backtrack;

/**
   Called after the rete net is created.
   Initializes the substition lists and queues in the state
**/
rete_state_backtrack* create_backtrack(const rete_net_state*);


#endif
