
#include "global.h"

//======================================================================
// off chip shared memory

bj_off_sys_st BJK_OFF_CHIP_SHARED_MEM bj_section("shared_dram");



//=====================================================================
// incore memory

void* 	bjk_dbg_call_stack_trace[BJ_MAX_CALL_STACK_SZ];

bj_sys_st bj_glb_sys;

bj_off_core_st* bj_off_core_pt;
bj_rrarray_st* bj_write_rrarray;
bj_in_core_st bj_in_core_shd;

uint16_t bjk_trace_err;

uint8_t bj_out_str[BJ_OUT_BUFF_MAX_OBJ_SZ];

//=====================================================================
// global funcs

#define BJ_B_OPCODE 0x000000e8 // OpCode of the B<*> instruction

void 
sync_interruption(void);

void 
bjk_set_sync_irq() bj_global_code_dram;
	
void 
bjk_set_sync_irq(){
	unsigned * ivt = 0x0;
	*ivt = ((((unsigned)sync_interruption) >> 1) << 8) | BJ_B_OPCODE;
}

void bj_inline_fn
bjk_set_coreid(void) {
	bj_asm("movfs %0, coreid" : "=r" (bj_in_core_shd.the_coreid));
}

void 
bjk_init_global(void) {
	// basic init
	bjk_set_sync_irq();
	bj_off_core_pt = 0x0;
	bj_write_rrarray = 0x0;
	bj_init_glb_sys();
	
	if(BJK_OFF_CHIP_SHARED_MEM.magic_id != BJ_MAGIC_ID){
		bjk_abort((uint32_t)bjk_init_global, 0, 0x0);
	}
	
	// bj_glb_sys init
	bj_id_t koid = bjk_get_coreid();
	bj_memset((uint8_t*)&bj_glb_sys, 0, sizeof(bj_glb_sys));
	bj_glb_sys = BJK_OFF_CHIP_SHARED_MEM.wrk_sys;

	// num_core init
	bj_consec_t num_core = bj_id_to_nn(koid);
	bj_off_core_pt = &((BJK_OFF_CHIP_SHARED_MEM.sys_cores)[num_core]);

	if((BJK_OFF_CHIP_SHARED_MEM.sys_out_buffs)[num_core].magic_id != BJ_MAGIC_ID){
		bjk_abort((uint32_t)bjk_init_global, 0, 0x0);
	}

	bj_core_out_st* out_st = &((BJK_OFF_CHIP_SHARED_MEM.sys_out_buffs)[num_core]);
	bj_write_rrarray = &(out_st->wr_arr);
	bj_rr_init(bj_write_rrarray, BJ_OUT_BUFF_SZ, out_st->buff, 0);
	
	if(bj_off_core_pt->magic_id != BJ_MAGIC_ID){
		bjk_abort((uint32_t)bjk_init_global, 0, 0x0);
	}
	
	// bj_in_core_shd init
	bj_memset((uint8_t*)&bj_in_core_shd, 0, sizeof(bj_in_core_shd));
	bj_memset((uint8_t*)bjk_dbg_call_stack_trace, 0, sizeof(bjk_dbg_call_stack_trace));
	
	bj_in_core_shd.magic_id = BJ_MAGIC_ID;
	bj_in_core_shd.dbg_stack_trace = bjk_dbg_call_stack_trace;
	bj_in_core_shd.magic_end = BJ_MAGIC_END;	
	bj_in_core_shd.the_coreid = koid;
	
	// bj_off_core_pt init	
	//bj_set_off_chip_var(bj_off_core_pt->magic_id, BJ_MAGIC_ID);
	bj_set_off_chip_var(bj_off_core_pt->the_coreid, bj_in_core_shd.the_coreid);
	bj_set_off_chip_var(bj_off_core_pt->core_data, &(bj_in_core_shd));
	
	bjk_set_finished(BJ_NOT_FINISHED_VAL);
	bj_set_off_chip_var(bj_off_core_pt->is_waiting, BJ_NOT_WAITING);
}

void
abort(){	// Needed when optimizing for size
	BJK_CK2(ck2_abort, 0);
	bjk_abort((uint32_t)abort, 0, 0x0);
	while(1);
}

void
bjk_slog(char* msg){ 
	uint16_t oln = bj_strlen(msg);
	if(oln > (BJ_OUT_BUFF_MAX_OBJ_SZ - 2)){
		oln = (BJ_OUT_BUFF_MAX_OBJ_SZ - 2);
	}
	bj_memcpy(bj_out_str + 2, (uint8_t*)msg, oln);
	bj_out_str[0] = BJ_OUT_LOG;
	bj_out_str[1] = BJ_CHR;
	uint16_t ow = bj_rr_write_obj(bj_write_rrarray, oln + 2, (uint8_t*)bj_out_str);
	if(ow == 0){
		bjk_wait_sync(BJ_WAITING_BUFFER, 0, bj_null);
	}
}

void
bjk_aux_ilog(uint32_t vv, bj_type_t tt){
	uint16_t oln = 2 + sizeof(uint32_t);
	uint8_t msg[oln];
	msg[0] = BJ_OUT_LOG;
	msg[1] = tt;
	uint8_t* pt = (uint8_t*)(&vv);
	msg[2] = pt[0];
	msg[3] = pt[1];
	msg[4] = pt[2];
	msg[5] = pt[3];
	uint16_t ow = bj_rr_write_obj(bj_write_rrarray, oln, (uint8_t*)msg);
	if(ow == 0){
		bjk_wait_sync(BJ_WAITING_BUFFER, 0, bj_null);
	}
}

