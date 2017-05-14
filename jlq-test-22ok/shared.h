
// bj_in_core_st the IN_CORE shared with host data

// bj_shared_data.h

#ifndef BJ_SHARED_DATA_H
#define BJ_SHARED_DATA_H


#ifdef __cplusplus
bj_c_decl {
#endif

#include <assert.h>

#include "stdbool.h"

#define bj_false 0x00
#define bj_true 0xff
#define bj_null 0x0

#define bj_force_assig(var, val) \
	do { (var) = (val); } while((var) != (val)); \
		
// end_macro


typedef uint8_t bj_bool_t;
typedef uint32_t bj_addr_t;
typedef uint16_t bj_core_co_t;
typedef uint16_t bj_size_t;

int
write_file(char* the_pth, uint8_t* the_data, long the_sz, int write_once);

uint8_t*
read_file(char* the_pth, off_t* size);

void 
bjh_abort_func(long val, const char* msg);

bool 
bjh_call_assert(bool vv_ck, const char* file, int line, const char* ck_str, const char* msg);

#define bjh_assert(vv) bjh_call_assert(vv, __FILE__, __LINE__, #vv, NULL)

#define BJH_CK(prm) bjh_assert(prm)

#ifdef __cplusplus
}
#endif


#endif // BJ_SHARED_DATA_H

