/* rete_state_single.h

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
#ifndef __INCLUDED_RETE_STATE_SINGLE
#define __INCLUDED_RETE_STATE_SINGLE

#include "strategy.h"
#include "rete_state_single_struct.h"
#include "rule_instance_single.h"


rule_instance_single* choose_next_instance_single(rete_state_single*);
rete_state_backup * backup_rete_state(rete_state_single*);
void destroy_rete_backup(rete_state_backup*);
void restore_rete_state(rete_state_single*, rete_state_backup*);


#endif
