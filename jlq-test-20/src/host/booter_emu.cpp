
//define _GNU_SOURCE

#include <inttypes.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thread_emu.hh"
#include "booter.h"
#include "string_hst.hh"

// =====================================================================================

bj_sys_sz_st bj_glb_host_sys;

void 
bjh_prt_core_call_stack_emu(thread_info_t& thd_inf){
	char** trace = thd_inf.bj_glb_sys_data.bjk_dbg_call_nams_stack_trace;
	bjh_out << "---------------------------------------------------\n";
	bjh_out << "STACK_TRACE for core num " << std::dec << thd_inf.thread_num;
	bjh_out << " with core_id = 0x" << std::hex << thd_inf.bjk_core_id << "\n";
	for(int aa = 0; aa < BJ_MAX_CALL_STACK_SZ; aa++){
		if(trace[aa] == bj_null){ break; }
		bjh_out << trace[aa] << "\n";
	}
	bjh_out << "---------------------------------------------------\n";
}

bool
ck_all_core_ids(){
	if (ALL_THREADS_INFO == NULL){
		bjh_abort_func(0, "ck_all_core_ids. NULL ALL_THREADS_INFO \n");
	}
	for(int aa = 0; aa < TOT_THREADS; aa++){
		bj_core_id_t koid = bj_nn_to_id(aa);
		if(ALL_THREADS_INFO[aa].bjk_core_id != koid){
			bjh_abort_func(0, "ck_all_core_ids. BAD CORE ID \n");
		}
	}
	return true;
}

int 
host_main(int argc, char *argv[])
{
	bj_core_co_t row, col, max_row, max_col;
	bj_core_id_t core_id;
	char f_nm[200];
	char* all_f_nam[bj_out_num_cores];
	int ss, tnum;

	bj_init_glb_sys_sz(&bj_glb_host_sys);

	memset(&all_f_nam, 0, sizeof(all_f_nam));

	ALL_THREADS_INFO = bj_null;

	HOST_THREAD_ID = pthread_self();

	TOT_THREADS = bj_tot_nn_sys;
	ALL_THREADS_INFO = (thread_info_t *)calloc(TOT_THREADS, sizeof(thread_info_t));
	if (ALL_THREADS_INFO == NULL){
		bjh_abort_func(0, "host_main. NULL ALL_THREADS_INFO \n");
	}

	printf("TOT_THREADS = %d\n", TOT_THREADS);

	bj_off_sys_st* pt_shd_data = &BJK_OFF_CHIP_SHARED_MEM;
	BJH_CK(sizeof(*pt_shd_data) == sizeof(bj_off_sys_st));
	printf("sizeof(*pt_shd_data)=%ld\n", sizeof(*pt_shd_data));

	// init shared data.
	memset(pt_shd_data, 0, sizeof(*pt_shd_data));

	pt_shd_data->magic_id = BJ_MAGIC_ID;
	BJH_CK(pt_shd_data->magic_id == BJ_MAGIC_ID);

	bj_sys_sz_st* sys_sz = bj_get_glb_sys_sz();

	pt_shd_data->wrk_sys = *sys_sz;
	BJH_CK(ck_sys_data(&(pt_shd_data->wrk_sys)));

	max_row = 1;
	max_col = 2;
	max_row = sys_sz->xx_sz;
	max_col = (1 << (sys_sz->yy_sz_pw2));

	for (row=0; row < max_row; row++){
		for (col=0; col < max_col; col++){
			core_id = bj_ro_co_to_id(row, col);
			bj_core_nn_t num_core = bj_id_to_nn(core_id);

			thread_info_t& thd_inf = ALL_THREADS_INFO[num_core];
			thd_inf.thread_num = num_core;
			thd_inf.argv_string = bj_null;
			bj_uint16_to_hex_bytes(thd_inf.thread_num, (uint8_t*)(thd_inf.thread_name));
			thd_inf.bjk_core_id = core_id;

			printf("STARTING CORE 0x%03x (%2d,%2d) NUM=%d\n", core_id, row, col, num_core);

			if(num_core < bj_out_num_cores){
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
			}
			
			// Start one core emulation thread

			ss = pthread_create(&thd_inf.thread_id, NULL,
								&thread_start, &thd_inf);
			if (ss != 0){
				bjh_abort_func(ss, "host_main. Cannot pthread_create");
			}
		}
	}

	BJH_CK(ck_all_core_ids());

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
				core_id = bj_ro_co_to_id(row, col);
				bj_core_nn_t num_core = bj_id_to_nn(core_id);
				bj_off_core_st* sh_dat_1 = &(pt_shd_data->sys_cores[num_core]);
				bj_core_out_st* pt_buff = &(pt_shd_data->sys_out_buffs[num_core]);

				thread_info_t& thd_inf = ALL_THREADS_INFO[num_core];

				// Wait for core program execution to finish.
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

				bj_in_core_st inco;
				
				// wait for finish
				if(sh_dat_1->is_finished == BJ_NOT_FINISHED_VAL){
					has_work = true;
					print_out_buffer(&(pt_buff->rd_arr), all_f_nam[num_core], num_core);
					if(sh_dat_1->is_waiting){
						if(sh_dat_1->is_waiting == BJ_WAITING_ENTER){
							get_enter(row, col);
						}
						if(sh_dat_1->is_waiting == BJ_WAITING_BUFFER){
							print_out_buffer(&(pt_buff->rd_arr), all_f_nam[num_core], num_core);
						}
						
						sh_dat_1->is_waiting = BJ_NOT_WAITING;

						thd_inf.bj_glb_sys_data.bjk_sync_signal = 1;	// SEND signal
					}
				} else {
					BJH_CK(sh_dat_1->is_finished == BJ_FINISHED_VAL);
	
					if(! core_finished[row][col]){

						core_finished[row][col] = true;

						print_out_buffer(&(pt_buff->rd_arr), all_f_nam[num_core], num_core);
						BJH_CK(bj_rr_ck_zero(&(pt_buff->rd_arr)));

						printf("Finished\n");
						memset(&inco, 0, sizeof(bj_in_core_st));
						memcpy(&inco, &thd_inf.bj_glb_sys_data.in_core_shd, sizeof(bj_in_core_st));
						int err2 = prt_inko_shd_dat(&inco);
						if(err2){
							break;
						}

						bjh_prt_core_call_stack_emu(thd_inf);
					}

				}
			}
		}
	} // while
	
	printf("sys_sz->xx=%d\n", sys_sz->xx);
	printf("sys_sz->yy=%d\n", sys_sz->yy);
	printf("sys_sz->xx_sz=%d\n", sys_sz->xx_sz);
	printf("sys_sz->yy_sz_pw2=%d\n", sys_sz->yy_sz_pw2);
	
	for (tnum = 0; tnum < TOT_THREADS; tnum++) {
		void *res;
		ss = pthread_join(ALL_THREADS_INFO[tnum].thread_id, &res);
		if(ss != 0){
			bjh_abort_func(ss, "host_main. Cannot join thread.");
		}
	}

	free(ALL_THREADS_INFO);

	int nn;
	for (nn=0; nn < bj_out_num_cores; nn++){
		if(all_f_nam[nn] != bj_null){
			free(all_f_nam[nn]);
		}
	}

	return 0;
}

// ===============================================================================

void
show_sizes() {
	printf("HOLA JOSE LUIS\n");
	printf("sizeof(thread_info_t)= %ld\n", sizeof(thread_info_t)); 

	pthread_t slf = pthread_self();
	printf("SELF= %ld\n", slf); 

}

void pw2_ops(int argc, char *argv[]){
	uint8_t pw2 = 3;

	//if(argc > 0){ printf("arg0= %s \n", argv[0]); }
	if(argc > 2){
		pw2 = atoi(argv[2]);
	}
	bj_core_co_t yy_sz = (1 << pw2);
	printf("yy_sz= %d \n", yy_sz);
	if(argc > 1){
		//printf("arg1= %s \n", argv[1]);
		int v1 = atoi(argv[1]);
		bj_core_co_t div1 = v1 / yy_sz;
		bj_core_co_t mod1 = v1 % yy_sz;
		bj_core_co_t div2 = v1 >> pw2;
		bj_core_co_t mod2 = v1 & ((1 << pw2) - 1);
		printf("div1= %d \n", div1);
		printf("div2= %d \n", div2);
		printf("mod1= %d \n", mod1);
		printf("mod2= %d \n", mod2);
	}
}

void test_align(){
	void* pt = (void*)0x8e01f1f0;
	printf("TEST_ALIGN\n");
	if(BJ_IS_ALIGNED_16(pt)){
		printf("16\n");
	}	
	if(BJ_IS_ALIGNED_32(pt)){
		printf("32\n");
	}	
	if(BJ_IS_ALIGNED_64(pt)){
		printf("64\n");
	}	
}

int main(int argc, char *argv[]) {
	host_main(argc, argv);
	//pw2_ops(argc, argv);
	//show_sizes();
	//test_align();
}


