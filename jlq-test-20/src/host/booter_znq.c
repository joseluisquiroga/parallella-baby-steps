

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <e-hal.h>
//include <e-loader.h>
#include <epiphany-hal-api-local.h>

#include "shared.h"
#include "booter.h"
#include "core_loader_znq.h"

//include "test_align.h"


//define f_nm_sz   1024

//=====================================================================================

const char* epiphany_elf_nm = "bj-core-actor.elf";

int	write_file(char* the_pth, char* the_data, long the_sz, int write_once);

bj_sys_sz_st bj_glb_sys;

bj_sys_sz_st*
bj_get_glb_sys_sz(){
	return &bj_glb_sys;
}

void
print_core_info(bj_off_core_st* sh_dat_1, e_epiphany_t* dev, unsigned row, unsigned col){
	bj_in_core_st inco;
	memset(&inco, 0, sizeof(bj_in_core_st));
	e_read(dev, row, col, (uint32_t)(sh_dat_1->core_data), &inco, sizeof(inco));
	int err = prt_inko_shd_dat(&inco);
	if(err){
		bjh_abort_func((bj_addr_t)(print_core_info), "print_core_info failed");
	}
	
	void* 	trace[BJ_MAX_CALL_STACK_SZ];
	memset(trace, 0, sizeof(trace));
	e_read(dev, row, col, (uint32_t)(inco.dbg_stack_trace), trace, sizeof(trace));
	bjh_prt_core_call_stack(epiphany_elf_nm, BJ_MAX_CALL_STACK_SZ, trace);
}

bj_off_sys_st* DBG_BASE;

int boot_znq(int argc, char *argv[])
{
	bj_core_co_t row, col, max_row, max_col;
	bj_core_nn_t tot_cores;
	bj_core_id_t core_id;
	e_platform_t platform;
	e_epiphany_t dev;
	e_mem_t emem;
	char f_nm[200];

	//e_set_loader_verbosity(H_D0);

	e_init(NULL);
	e_reset_system();
	e_get_platform_info(&platform);

	e_alloc(&emem, BJ_SHARED_MEM_START_DISP, sizeof(bj_off_sys_st));
	
	e_open(&dev, 0, 0, platform.rows, platform.cols);

	bjh_init_glb_sys_sz_with_dev(bj_get_glb_sys_sz(), &dev);

	void* the_base = (void*)(emem.base);
	printf("the_base=%p \n", the_base);

	DBG_BASE = (bj_off_sys_st*)the_base;

	bj_off_sys_st* pt_shd_data = (bj_off_sys_st*)the_base;
	BJH_CK(sizeof(*pt_shd_data) == sizeof(bj_off_sys_st));

	DBG_BASE->magic_id = BJ_MAGIC_ID;
	BJH_CK(DBG_BASE->magic_id == BJ_MAGIC_ID);

	//e_reset_group(&dev);
	//bj_load_group(epiphany_elf_nm, &dev, 0, 0, platform.rows, platform.cols, E_FALSE);

	// init shared data.
	memset(pt_shd_data, 0, sizeof(*pt_shd_data));

	pt_shd_data->magic_id = BJ_MAGIC_ID;
	BJH_CK(pt_shd_data->magic_id == BJ_MAGIC_ID);

	//bj_load_group(epiphany_elf_nm, &dev, 0, 0, platform.rows, platform.cols, E_FALSE); // resets magic_id !!!!!

	BJH_CK(pt_shd_data->magic_id == BJ_MAGIC_ID);

	pt_shd_data->pt_this_from_znq = pt_shd_data;

	bj_sys_sz_st* sys_sz = bj_get_glb_sys_sz();

	pt_shd_data->wrk_sys = *sys_sz;
	BJH_CK(ck_sys_data(&(pt_shd_data->wrk_sys)));

	e_reset_group(&dev);
	bj_load_group(epiphany_elf_nm, &dev, 0, 0, platform.rows, platform.cols, E_FALSE); // resets magic_id !!!!!

	max_row = 1;
	max_col = 2;
	max_row = dev.rows;
	max_col = dev.cols;

	tot_cores = max_row * max_col;
	BJH_CK(tot_cores <= bj_out_num_cores);

	char* all_f_nam[tot_cores];
	memset(&all_f_nam, 0, sizeof(all_f_nam));

	for (row=0; row < max_row; row++){
		for (col=0; col < max_col; col++){
			//core_id = (row + platform.row) * 64 + col + platform.col;
			core_id = bj_ro_co_to_id(row, col);
			bj_core_nn_t num_core = bj_id_to_nn(core_id);

			printf("STARTING CORE 0x%03x (%2d,%2d) NUM=%d\n", core_id, row, col, num_core);

			memset(&f_nm, 0, sizeof(f_nm));
			sprintf(f_nm, "log_core_%02d.txt", num_core);

			all_f_nam[num_core] = strdup((const char*)f_nm);
			
			// init shared data.
			pt_shd_data->sys_cores[num_core].magic_id = BJ_MAGIC_ID;
			BJH_CK(pt_shd_data->sys_cores[num_core].magic_id == BJ_MAGIC_ID);

			bj_core_out_st* pt_buff = &(pt_shd_data->sys_out_buffs[num_core]);

			pt_buff->magic_id = BJ_MAGIC_ID;
			BJH_CK(pt_buff->magic_id == BJ_MAGIC_ID);

			bj_rr_init(&(pt_buff->rd_arr), BJ_OUT_BUFF_SZ, pt_buff->buff, 1);

			// Start one core
			e_start(&dev, row, col);
		}
	}

	bool core_started[max_row][max_col];
	memset(core_started, 0, sizeof(core_started));

	bool core_finished[max_row][max_col];
	memset(core_finished, 0, sizeof(core_finished));

	bool has_work = true;	
	while(has_work){
		sched_yield();				//yield
		has_work = false;
		for (row=0; row < max_row; row++){
			for (col=0; col < max_col; col++){
				//core_id = (row + platform.row) * 64 + col + platform.col;
				core_id = bj_ro_co_to_id(row, col);
				bj_core_nn_t num_core = bj_id_to_nn(core_id);
				bj_off_core_st* sh_dat_1 = &(pt_shd_data->sys_cores[num_core]);
				bj_core_out_st* pt_buff = &(pt_shd_data->sys_out_buffs[num_core]);

				// Wait for core program execution to start
				if((sh_dat_1->core_data == 0x0) || (sh_dat_1->is_finished == 0x0)){
					has_work = true;
					BJH_CK(sh_dat_1->magic_id == BJ_MAGIC_ID);
					continue;
				}
				
				BJH_CK(sh_dat_1->magic_id == BJ_MAGIC_ID);
				BJH_CK(	(sh_dat_1->is_finished == BJ_NOT_FINISHED_VAL) ||
						(sh_dat_1->is_finished == BJ_FINISHED_VAL)
				);

				BJH_CK(sh_dat_1->ck_core_id == core_id);
				if(! core_started[row][col] && (sh_dat_1->is_finished == BJ_NOT_FINISHED_VAL)){ 
					core_started[row][col] = true;
					printf("Waiting for finish 0x%03x (%2d,%2d) NUM=%d\n", 
								core_id, row, col, num_core);
				}

				// wait for finish
				if(sh_dat_1->is_finished == BJ_NOT_FINISHED_VAL){
					has_work = true;
					print_out_buffer(&(pt_buff->rd_arr), all_f_nam[num_core], num_core);
					if(sh_dat_1->is_waiting){
						if(sh_dat_1->is_waiting == BJ_WAITING_ENTER){
							print_core_info(sh_dat_1, &dev, row, col);
							get_enter(row, col);
						}
						if(sh_dat_1->is_waiting == BJ_WAITING_BUFFER){
							print_out_buffer(&(pt_buff->rd_arr), all_f_nam[num_core], num_core);
						}
						
						int SYNC = (1 << E_SYNC);
						sh_dat_1->is_waiting = BJ_NOT_WAITING;
						if (ee_write_reg(&dev, row, col, E_REG_ILATST, SYNC) == E_ERR) {
							printf("ERROR sending SYNC to (%d, %d) \n", row, col);
							break;
						}
					}
				} else {
					BJH_CK(sh_dat_1->is_finished == BJ_FINISHED_VAL);
	
					if(! core_finished[row][col]){
						core_finished[row][col] = true;

						print_out_buffer(&(pt_buff->rd_arr), all_f_nam[num_core], num_core);
						BJH_CK(bj_rr_ck_zero(&(pt_buff->rd_arr)));

						printf("Finished\n");
						print_core_info(sh_dat_1, &dev, row, col);
					}

				}
			}
		}
	} // while
	
	printf("PLATFORM row=%2d col=%2d \n", platform.row, platform.col);
	printf("sys_sz->xx=%d\n", sys_sz->xx);
	printf("sys_sz->yy=%d\n", sys_sz->yy);
	printf("sys_sz->xx_sz=%d\n", sys_sz->xx_sz);
	printf("sys_sz->yy_sz_pw2=%d\n", sys_sz->yy_sz_pw2);

	printf("SHD_DATA_addr_as_seen_from_eph=%p\n", pt_shd_data->pt_this_from_eph);
	printf("SHD_DATA_addr_as_seen_from_znq=%p\n", pt_shd_data->pt_this_from_znq);
	printf("SHD_DATA_displacement_from_shd_mem_base_adddr= %p\n", (void*)BJ_SHARED_MEM_START_DISP);

	printf("the_base=%p \n", the_base);
	
	// Reset the workgroup
	e_reset_group(&dev); // FAILS. Why?
	e_reset_system();
	
	// Close the workgroup
	e_close(&dev);
	
	// Release the allocated buffer and finalize the
	// e-platform connection.
	e_free(&emem);
	e_finalize();

	int nn;
	for (nn=0; nn < tot_cores; nn++){
		if(all_f_nam[nn] != bj_null){
			free(all_f_nam[nn]);
		}
	}

	//prt_host_aligns();

	return 0;
}

int main(int argc, char *argv[]) {
	boot_znq(argc, argv);
	return 0;
}

