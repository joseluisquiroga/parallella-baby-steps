
#include "global.h"

#include "thread_emu.hh"
#include "booter.h"

//=====================================================================
// global funcs

bjk_glb_sys_st*
bjk_get_glb_sys(){
	return &(bjk_get_thread_info()->bj_glb_sys_data);
}

bj_sys_sz_st*
bj_get_glb_sys_sz(){
	if(bj_is_host_thread()){
		return &bj_glb_host_sys;
	}
	return &(bjk_get_glb_sys()->sys_sz);
}

void 
bjk_set_irq0_handler(){
}

