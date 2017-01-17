

// bj_shared_data.h

#ifndef BJ_SHARED_DATA_H
#define BJ_SHARED_DATA_H

#include <inttypes.h>
#include "attribute.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define bj_axis_bits	6
#define bj_axis_mask	0x3f
	
typedef uint16_t bj_id_t;	// e_coreid_t
typedef uint16_t bj_coor_t;

typedef uint32_t bj_addr_t;

#define bj_null 0x0

struct bj_sys_def { 
	bj_coor_t 	xx;		// absolute xx epiphany space coordinates
	bj_coor_t 	yy;		// absolute yy epiphany space coordinates
	bj_coor_t 	xx_sz;		// this running sys number of ekores in xx axis (sys length)
	bj_coor_t 	yy_sz;		// this running sys number of ekores in yy axis (sys witdh)
};
typedef struct bj_sys_def bj_sys_st;
	
//======================================================================
// convertion functions
	
// xx and yy are absolute epiphany space coordinates
// ro and co are relative epiphany space coordinates with respect to the 
// 		allocated running cores (bj_glb_sys)
// id is the coreid absolute in epiphany space 
// nn is a consec with respect to the allocated running cores (bj_glb_sys)

#define bj_id_to_xx(id)	(((id) >> bj_axis_bits) & bj_axis_mask)
#define bj_id_to_yy(id)	((id) & bj_axis_mask)
#define bj_xx_to_ro(xx)	((xx) - ((bj_glb_sys)->xx))
#define bj_yy_to_co(yy)	((yy) - ((bj_glb_sys)->yy))
#define bj_id_to_ro(id)	bj_xx_to_ro(bj_id_to_xx(id))
#define bj_id_to_co(id)	bj_yy_to_co(bj_id_to_yy(id))
#define bj_ro_to_xx(ro)	((ro) + ((bj_glb_sys)->xx))
#define bj_co_to_yy(co)	((co) + ((bj_glb_sys)->yy))
#define bj_ro_co_to_nn(ro, co) (((ro) * ((bj_glb_sys)->yy_sz)) + (co))
#define bj_xx_yy_to_id(xx, yy) (((xx) << bj_axis_bits) + (yy))
#define bj_ro_co_to_id(ro, co) ((bj_ro_to_xx(ro) << bj_axis_bits) + bj_co_to_yy(co))
#define bj_nn_to_ro(nn)	((nn) / ((bj_glb_sys)->yy_sz))
#define bj_nn_to_co(nn)	((nn) % ((bj_glb_sys)->yy_sz))
#define bj_id_to_nn(id) (bj_ro_co_to_nn(bj_id_to_ro(id), bj_id_to_co(id)))
#define bj_nn_to_id(nn) (bj_ro_co_to_id(bj_nn_to_ro(id), bj_nn_to_co(id)))

//======================================================================
// sane alignment/access functions

uint32_t bj_inline_fn
bj_v32_of_p16(uint16_t* p16){
	uint32_t v32 = p16[1];
	v32 <<= 16;
	v32 |= p16[0];
	return v32;
}

#define set_off_chip_var(var, val) \
	(var) = (val); \
	while((var) != (val)); \
		
// end_macro
	

//======================================================================
// bitarray

// where 'a' is the byte stream pointer, and b is the bit number.

#define bj_div8(b)	((b)>>3)
#define bj_mod8(b)	((b)&7)

#define bj_get_bit(a, b)		((((uint8_t*)a)[bj_div8(b)] >> bj_mod8(b)) & 1)
#define bj_set_bit(a, b)		(((uint8_t*)a)[bj_div8(b)] |= (1 << bj_mod8(b)))
#define bj_reset_bit(a, b) 	(((uint8_t*)a)[bj_div8(b)] &= ~(1 << bj_mod8(b)))
#define bj_toggle_bit(a, b) 	(((uint8_t*)a)[bj_div8(b)] ^= (1 << bj_mod8(b)))

#define bj_to_bytes(num_bits)	(bj_div8(num_bits) + (bj_mod8(num_bits) > 0))
#define bj_to_bits(num_bytes)	(num_bytes * k_num_bits_byte)

#define num_ck_lines 7

#define bj_mem_2K   2048
#define bj_mem_32K   32768
#define bj_max_core_addr 0x7ff0
#define bj_max_opcodes_func 16384

#define max_ptrs 16
#define max_16bit_idx   8192

#define BJ_MAGIC_ID 0xabcd9876
#define BJ_MAGIC_END 0x6789abcd

#define BJ_ABORT_ERR 				0xdeadbeaf
#define BJ_CALL_STACK_TRACE_ERR		0x01234567

#define BJ_MAX_CALL_STACK_SZ	20

struct bj_in_core_shared_data_def { // CAREFUL !! sometimes aligment(ekore) != aligment(host)
	uint32_t 	magic_id;
	void** 		dbg_stack_trace;
	
	bj_sys_st 	dev;
	
	uint32_t 	dbg_error_code;
	uint32_t 	dbg_progress_flag;
	uint32_t 	dbg_info_wait;

	uint16_t* pc_val;
	uint16_t* rts_addr;
	uint16_t* call_addr;
	uint16_t disp; 
	
	bj_id_t 	the_coreid;
	uint8_t 	cpp_fun1;
	uint8_t 	cpp_dcla1;
	uint8_t 	got_irq0;
	
	uint32_t 	magic_end;
};
typedef struct bj_in_core_shared_data_def bj_in_core_st;

#define BJ_NOT_FINISHED_VAL 0x21
#define BJ_FINISHED_VAL 	0xf1

#define BJ_NOT_WAITING_VAL 	0x0
#define BJ_WATING_VAL 		0xaa

struct bj_off_core_shared_data_def { 
	uint32_t 	magic_id;
	bj_id_t		the_coreid;
	uint8_t 	is_finished;
	uint8_t 	is_waiting;
	bj_in_core_st* core_data;
};
typedef struct bj_off_core_shared_data_def bj_off_core_st;

// off_core_st all_cores[];

int 
bjh_prt_call_stack(const char *elf_nm, int addrs_sz, void** stack_addrs);


#ifdef __cplusplus
}
#endif

#endif // BJ_SHARED_DATA_H

