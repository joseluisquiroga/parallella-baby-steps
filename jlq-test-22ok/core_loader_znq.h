/*
  File: e-loader.h

  This file is part of the Epiphany Software Development Kit.

  Copyright (C) 2013 Adapteva, Inc.
  See AUTHORS for list of contributors.
  Support e-mail: <support@adapteva.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License (LGPL)
  as published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  and the GNU Lesser General Public License along with this program,
  see the files COPYING and COPYING.LESSER.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#ifndef BJ_LOADER_H
#define BJ_LOADER_H

#include "e-hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "shared.h"

//=====================================================================================
// IMPORTANT NOTE:
// ---------------

// The following displacemente MUST match the definitions in the linker script and the 
// Hardware Description File (HDF) passed to e_init.

#define BJ_SHARED_MEM_START_DISP (0x01000000)

// Current link script: bj-ld-script.ldf
// Current link addres for section 'shared_dram': EXTERNAL_DRAM_1
// Current origin of EXTERNAL_DRAM_1: 0x8f000000
// Current HDF: the value of EPIPHANY_HDF enviroment variable because e_initi is called with NULL
// Current value of EPIPHANY_HDF: /opt/adapteva/esdk/bsps/current/platform.hdf
// Current value of EMEM_BASE_ADDRESS in HDF: 0x8e000000

// So we have:
// EMEM_BASE_ADDRESS 	+ BJ_SHARED_MEM_START_DISP 	== ORIGIN(EXTERNAL_DRAM_1)
// 0x8e000000 			+ 0x01000000 				== 0x8f000000

// Note also that these addresses are AS SEEN FROM THE EPIPHANY. Not as seen from the Zynq 
// side (host side)



int bj_load(const char *executable, e_epiphany_t *dev, unsigned row, unsigned col, e_bool_t start);
int bj_load_group(const char *executable, e_epiphany_t *dev, unsigned row, unsigned col, 
						unsigned rows, unsigned cols, e_bool_t start);

#define DBGH_CODE_SHD_SZ 20
extern uint16_t DBGH_CODE_SHD_1[DBGH_CODE_SHD_SZ];
extern bj_addr_t DBGH_FUNC_ADDR;

void
ck_dbg_shd_code(char* msg, uint16_t* base);

#ifdef __cplusplus
}
#endif

#endif // BJ_LOADER_H
