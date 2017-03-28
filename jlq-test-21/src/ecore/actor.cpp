
#include <cstdlib>
#include <new>

#ifdef IS_EMU_CODE
	#include "booter.h"
#endif

#include "interruptions.h"
#include "global.h"
#include "trace.h"
//include "test_logs.h"

#include "core_main.h"
//include "row.hh"
#include "dyn_mem.hh"
#include "actor.hh"


//----------------------------------------------------------------------------
// To FAKE std c++ lib initialization and destructions of global objects
// DO NOT FORGET to call initializers explicitly.

bj_c_decl int __cxa_atexit(void* obj, void (*destruc) (void*), void* dso_hndl) bj_code_dram;

int 
__cxa_atexit(void* obj, void (*destruc) (void*), void* dso_hndl){
	static_cast<void>(obj);
	static_cast<void>(destruc);
	static_cast<void>(dso_hndl);
	return 0;
}

#ifndef IS_EMU_CODE
void* __dso_handle = bj_null;
#endif	//IS_EMU_CODE

//----------------------------------------------------------------------------

bjk_actor_id_t 	agent::THE_ACTOR_ID = bjk_actor_id(agent);
bjk_actor_id_t 	actor::THE_ACTOR_ID = bjk_actor_id(actor);
bjk_actor_id_t 	missive::THE_ACTOR_ID = bjk_actor_id(missive);
bjk_actor_id_t 	agent_ref::THE_ACTOR_ID = bjk_actor_id(agent_ref);
bjk_actor_id_t 	agent_grp::THE_ACTOR_ID = bjk_actor_id(agent_grp);

BJK_DEFINE_ACQUIRE(actor)
BJK_DEFINE_ACQUIRE(missive)
BJK_DEFINE_ACQUIRE(agent_ref)
BJK_DEFINE_ACQUIRE(agent_grp)

BJK_DEFINE_SEPARATE(actor)
BJK_DEFINE_SEPARATE(missive)
BJK_DEFINE_SEPARATE(agent_ref)
BJK_DEFINE_SEPARATE(agent_grp)

char*
agent::get_class_name(){
	kernel* ker = kernel::get_sys();
	bjk_actor_id_t id = get_actor_id();
	if(bjk_is_valid_class_name_idx(id)){
		return (ker->class_names_arr)[id];
	}
	return bj_null;
}

void 
kernel::add_out_missive(missive& msv1){
	binder * fst, * lst, * wrk;
	kernel* ker = this;

	fst = bjk_pt_to_binderpt(ker->out_work.bn_right);
	lst = &(ker->out_work);
	for(wrk = fst; wrk != lst; wrk = bjk_pt_to_binderpt(wrk->bn_right)){
		missive_grp_t* mgrp = (missive_grp_t*)wrk;
		missive* msv2 = (missive*)(mgrp->all_agts.get_right_pt());
		if(bj_addr_same_id(msv1.dst, msv2->dst)){
			mgrp->bind_to_my_left(*msv2);
		}
	}
	missive_grp_t* mgrp2 = agent_grp::acquire();
	EMU_CK(mgrp2 != bj_null);
	EMU_CK(mgrp2->all_agts.is_alone());

	mgrp2->all_agts.bind_to_my_left(msv1);
	ker->out_work.bind_to_my_left(*mgrp2);
}

void 
kernel::call_handlers_of_group(missive_grp_t* rem_mgrp){
	binder * fst, * lst, * wrk;
	kernel* ker = this;

	binder* all_msvs = &(rem_mgrp->all_agts);

	fst = binder::get_glb_right_pt(all_msvs);
	lst = all_msvs;
	for(wrk = fst; wrk != lst; wrk = binder::get_glb_right_pt(wrk)){
		missive* remote_msv = (missive*)wrk;

		actor* dst = remote_msv->dst;
		EMU_CK(dst != bj_null);
		bjk_handler_idx_t idx = dst->id_idx;
		if(bjk_is_valid_handler_idx(ker, idx)){
			(*((ker->handlers_arr)[idx]))(remote_msv);
		}
	}

	rem_mgrp->handled = bj_true;
}

void // static
kernel::actors_handle_loop(){
	kernel* ker = kernel::get_sys();
	
	while(true){
		ker->handle_missives();
	}
}

void
wait_inited_state(bj_core_id_t dst_id){
	bj_in_core_st* in_shd = bjk_get_glb_in_core_shd();
	uint8_t* loc_st = &(in_shd->the_core_state);
	uint8_t* rmt_st = (uint8_t*)bj_addr_with(dst_id, loc_st);
	while(*rmt_st != bjk_inited_state);
}

//bj_opt_sz_fn
void
wait_value(uint32_t& var, uint32_t val){
	while(var < val);
}

bj_opt_sz_fn
uint32_t
wait_time(uint32_t max){
	uint32_t aa;
	for(aa = 0; aa < max; aa++){
		aa++;
	}
	return aa;
}

void
ck_sizes(){
	BJK_CK2(ck_sz1, (sizeof(void*) == sizeof(bj_addr_t)));
	BJK_CK2(ck_sz1, (sizeof(void*) == sizeof(unsigned)));
	BJK_CK2(ck_sz1, (sizeof(void*) == sizeof(uint32_t)));
}

grip&	
agent::get_available(){
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpmf-conversions"
	bjk_abort((bj_addr_t)(void*)&agent::get_available, 0, bj_null);
	#pragma GCC diagnostic pop
	return *((grip*)bj_null);
}

void
agent::init_me(){
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpmf-conversions"
	bjk_abort((bj_addr_t)(void*)&agent::init_me, 0, bj_null);
	#pragma GCC diagnostic pop
}

void
kernel::init_kernel(){
	bj_init_arr_vals(kernel_signals_arr_sz, signals_arr, bj_false);

	bj_init_arr_vals(kernel_handlers_arr_sz, handlers_arr, bj_null);
	bj_init_arr_vals(kernel_pw0_routed_arr_sz, pw0_routed_arr, bj_null);
	bj_init_arr_vals(kernel_pw2_routed_arr_sz, pw2_routed_arr, bj_null);
	bj_init_arr_vals(kernel_pw4_routed_arr_sz, pw4_routed_arr, bj_null);
	bj_init_arr_vals(kernel_pw6_routed_arr_sz, pw6_routed_arr, bj_null);

	bj_init_arr_vals(kernel_class_names_arr_sz, class_names_arr, bj_null);

	bjk_set_class_name(agent);
	bjk_set_class_name(actor);
	bjk_set_class_name(missive);
	bjk_set_class_name(agent_ref);
	bjk_set_class_name(agent_grp);

	first_actor = actor::acquire();
}

void	// static
kernel::init_sys(){
	bjk_glb_init();

	kernel* ker = kernel::get_sys();

	new (ker) kernel(); 

	bj_in_core_st* in_shd = bjk_get_glb_in_core_shd();

	in_shd->binder_sz = sizeof(binder);
	in_shd->kernel_sz = sizeof(kernel);
	in_shd->agent_sz = sizeof(agent);
	in_shd->actor_sz = sizeof(actor);
	in_shd->missive_sz = sizeof(missive);
	in_shd->agent_ref_sz = sizeof(agent_ref);
	in_shd->agent_grp_sz = sizeof(agent_grp);
	in_shd->bjk_glb_sys_st_sz = sizeof(bjk_glb_sys_st);

	bjk_enable_all_irq();
	bjk_global_irq_enable();

	in_shd->the_core_state = bjk_inited_state;
}

void	// static
kernel::finish_sys(){
	bjk_glb_finish();
}

void test_send_irq1() bj_code_dram;
void test_send_irq1() {
	kernel::init_sys();

	if(bjk_is_core(3, 3)){
		bj_core_id_t dst = kernel::get_core_id();
		bjk_send_irq(dst, 1);	// send mem exception itself
	}

	bjk_slog2("FINISHED !!\n");	
	kernel::finish_sys();
}

uint32_t test_send_irq3 = 0;

void test_send_irq() bj_code_dram;
void test_send_irq() {
	kernel::init_sys();
	ck_sizes();

	if(kernel::get_sys()->direct_routed.is_alone()){
		bjk_slog2("direct_routed ALONE\n");
	}

	test_send_irq3 = 0;
	
	if(bjk_is_core(0,0)){
		bjk_slog2("CORE (0,0) started\n");
		bj_core_id_t dst = bj_ro_co_to_id(0, 1);
		wait_inited_state(dst);
		bjk_slog2("CORE (0,0) SAW core (0,1) INITED\n");

		BJK_MARK_PLACE(START_UGLY_WAIT);
		wait_value(test_send_irq3, 4);
		BJK_MARK_PLACE(END_UGLY_WAIT);

		bjk_slog2("got_irq3="); bjk_ilog(test_send_irq3); bjk_slog2("\n");
	}
	if(bjk_is_core(0,1)){
		bjk_slog2("CORE (0,1) started\n");
		bj_core_id_t dst = bj_ro_co_to_id(0, 0);
		wait_inited_state(dst);
		bjk_slog2("CORE (0,1) SAW core (0,0) INITED\n");

		bjk_send_irq(dst, 3);
		bjk_send_irq(dst, 3);
		bjk_send_irq(dst, 3);
		bjk_send_irq(dst, 3);

		bjk_slog2("CORE (0,1) sent 4 irq3\n");
	}

	bjk_slog2("FINISHED !!\n");	
	kernel::finish_sys();
}

bj_c_decl void f1();
void f1(){
	bjk_abort((bj_addr_t)f1, 0, bj_null);
}

bj_c_decl void f2();
void f2(){
	f1();
}

bj_c_decl void f3();
void f3(){
	f2();
}

void test_abort() bj_code_dram;
void test_abort() {
	bjk_glb_init();

	f3();

	bjk_glb_finish();
}

/*
void test_logs() bj_code_dram;
void test_logs() {
	bjk_glb_init();

	bj_core_id_t koid = bjk_get_core_id();
	bj_core_nn_t num_core = bj_id_to_nn(koid);

	char** john = (char**)(all_tests[num_core]);
	long john_sz = all_tests_sz[num_core];
	long ii;
	for(ii = 0; ii < john_sz; ii++){
		bjk_slog(john[ii]);
	}

	bjk_glb_finish();
}
*/

actor*	//	static 
kernel::get_core_actor(bj_core_id_t dst_id){
	actor* loc_act = kernel::get_core_actor();
	if(dst_id == kernel::get_core_id()){
		return loc_act;
	}
	actor* rem_act = (actor*)bj_addr_with(dst_id, loc_act);
	return rem_act;
}

void // static 
kernel::set_handler(missive_handler_t hdlr, uint16_t idx){
	kernel* ker = kernel::get_sys();
	if(bjk_is_valid_handler_idx2(idx)){
		(ker->handlers_arr)[idx] = hdlr;
	}
}

void 
kernel::process_signal(int sz, missive_grp_t** arr){
	for(int aa = 0; aa < sz; aa++){
		if(arr[aa] != bj_null){
			missive_ref_t* nw_ref = agent_ref::acquire();
			EMU_CK(nw_ref->is_alone());
			EMU_CK(nw_ref->glb_agent_ptr == bj_null);

			nw_ref->glb_agent_ptr = arr[aa];
			in_work.bind_to_my_left(*nw_ref);
			arr[aa] = bj_null;
		}
	}
}

void 
kernel::handle_missives(){
	kernel* ker = this;
	binder * fst, * lst, * wrk, * nxt;

	for(int aa = 0; aa < kernel_signals_arr_sz; aa++){
		if(signals_arr[aa] == bj_true){
			signals_arr[aa] = bj_false;
			switch(aa){
				case bjk_do_pw0_routes_sgnl:
					process_signal(kernel_pw0_routed_arr_sz, pw0_routed_arr);
					break;
				case bjk_do_pw2_routes_sgnl:
					process_signal(kernel_pw2_routed_arr_sz, pw2_routed_arr);
					break;
				case bjk_do_pw4_routes_sgnl:
					process_signal(kernel_pw4_routed_arr_sz, pw4_routed_arr);
					break;
				case bjk_do_pw6_routes_sgnl:
					process_signal(kernel_pw6_routed_arr_sz, pw6_routed_arr);
					break;
				case bjk_do_direct_routes_sgnl:
					break;
				default:
					break;
			}
			if(aa == bjk_do_direct_routes_sgnl){
			} else {
			}
		}
	}

	binder* in_grp = &(ker->in_work);
	fst = bjk_pt_to_binderpt(in_grp->bn_right);
	lst = in_grp;
	for(wrk = fst; wrk != lst; wrk = nxt){
		missive_ref_t* fst_ref = (missive_ref_t*)wrk;
		nxt = bjk_pt_to_binderpt(wrk->bn_right);

		missive_grp_t* remote_grp = (missive_grp_t*)(fst_ref->glb_agent_ptr);

		//bjk_slog2("RECEIVING MISSIVE\n");
		//EMU_PRT("RECEIVING pt_msv_grp= %p\n", remote_grp);

		call_handlers_of_group(remote_grp);
		fst_ref->release();
		EMU_CK(fst_ref->glb_agent_ptr == bj_null);
	}

	fst = bjk_pt_to_binderpt(ker->local_work.bn_right);
	lst = &(ker->local_work);
	for(wrk = fst; wrk != lst; wrk = nxt){
		missive* fst_msg = (missive*)wrk;
		nxt = bjk_pt_to_binderpt(wrk->bn_right);

		actor* dst = fst_msg->dst;
		if(bjk_addr_is_local(dst)){
			if(bjk_is_valid_handler_idx(ker, dst->id_idx)){
				(*((ker->handlers_arr)[dst->id_idx]))(fst_msg);
			}
			fst_msg->release();
		} else {
			fst_msg->let_go();
			add_out_missive(*fst_msg);
		}
	}

	fst = bjk_pt_to_binderpt(ker->out_work.bn_right);
	lst = &(ker->out_work);
	for(wrk = fst; wrk != lst; wrk = nxt){
		missive_grp_t* mgrp = (missive_grp_t*)wrk;
		nxt = bjk_pt_to_binderpt(wrk->bn_right);

		EMU_CK(! mgrp->all_agts.is_alone());
		
		missive* msv = (missive*)(mgrp->all_agts.get_right_pt());
		bj_core_id_t dst_id = bj_addr_get_core_id(msv->dst);
		bj_core_nn_t idx = bj_id_to_nn(dst_id);

		// ONLY pw0 case
		missive_grp_t** loc_pt = &((ker->pw0_routed_arr)[idx]);
		missive_grp_t** rmt_pt = (missive_grp_t**)bj_addr_with(dst_id, loc_pt);
		missive_grp_t* glb_mgrp = (missive_grp_t*)bjk_as_glb_pt(mgrp);
		//missive_grp_t* glb_mgrp = (missive_grp_t*)mgrp->get_glb_ptr();

		//EMU_PRT("SENDING pt_msv_grp= %p right= %p\n", mgrp, mgrp->get_right_pt());

		*rmt_pt = glb_mgrp;

		// send signal
		bj_bool_t* loc_sg = &((ker->signals_arr)[bjk_do_pw0_routes_sgnl]);
		bj_bool_t* rmt_sg = (bj_bool_t*)bj_addr_with(dst_id, loc_sg);

		*rmt_sg = bj_true;

		mgrp->let_go();
		ker->sent_work.bind_to_my_left(*mgrp);
	}

	fst = bjk_pt_to_binderpt(ker->sent_work.bn_right);
	lst = &(ker->sent_work);
	for(wrk = fst; wrk != lst; wrk = nxt){
		missive_grp_t* mgrp = (missive_grp_t*)wrk;
		nxt = bjk_pt_to_binderpt(wrk->bn_right);

		if(mgrp->handled){
			mgrp->release_all_agts();
			mgrp->let_go();
			mgrp->release();
		}
	}
}

void
agent_grp::release_all_agts(){
	binder * fst, * lst, * wrk;

	binder* all_agts = &(this->all_agts);

	fst = binder::get_glb_right_pt(all_agts);
	lst = all_agts;
	for(wrk = fst; wrk != lst; wrk = binder::get_glb_right_pt(wrk)){
		agent* agt = (agent*)wrk;
		agt->let_go();
		agt->release();
	}
}

void 
actor_handler(missive* msg){
	EMU_CK(bjk_addr_is_local(msg->dst));
	bj_core_id_t koid = kernel::get_core_id();
	BJ_MARK_USED(koid);
	bj_core_nn_t konn = kernel::get_core_nn();
	BJ_MARK_USED(konn);
	bjk_slog2("GOT MISSIVE\n");
	EMU_LOG("actor_handler. core_id=%lx core_nn=%d src=%p dst=%p \n", koid, konn, msg->get_source(), msg->dst);
	EMU_PRT("actor_handler. core_id=%lx core_nn=%d src=%p dst=%p \n", koid, konn, msg->get_source(), msg->dst);
	msg->dst->flags = 1;
}

void test_send_msg() bj_code_dram;
void test_send_msg() {
	kernel::init_sys();

	kernel::set_handler(actor_handler, bjk_handler_idx(actor));

	actor::separate(bj_out_num_cores);
	missive::separate(bj_out_num_cores);
	agent_ref::separate(bj_out_num_cores);
	agent_grp::separate(bj_out_num_cores);

	kernel* ker = kernel::get_sys();
	BJ_MARK_USED(ker);

	if(bjk_is_core(0,0)){
		bjk_slog2("CORE (0,0) started\n");
		bj_core_id_t dst = bj_ro_co_to_id(0, 1);
		wait_inited_state(dst);
		bjk_slog2("CORE (0,0) SAW core (0,1) INITED\n");

		actor* act1 = kernel::get_core_actor();
		BJ_MARK_USED(act1);
		while(! act1->flags){
			ker->handle_missives();
		}
	}
	if(bjk_is_core(0,1)){
		bjk_slog2("CORE (0,1) started\n");
		bj_core_id_t dst = bj_ro_co_to_id(0, 0);
		wait_inited_state(dst);
		bjk_slog2("CORE (0,1) SAW core (0,0) INITED\n");

		
		actor* act1 = kernel::get_core_actor();
		actor* act2 = kernel::get_core_actor(dst);

		missive* msv = missive::acquire();
		msv->set_source(act1);
		msv->dst = act2;
		msv->send();

		ker->handle_missives();

		bjk_slog2("SENT MISSIVE\n");
	}

	bjk_slog2("FINISHED !!\n");	
	kernel::finish_sys();
}

void test_minimal() {
	bjk_glb_init();
	bjk_slog2("STARTED !!\n");	

	//test_link_shd_code();	// THIS MAKES IT FAIL !!!!

	bjk_slog2("FINISHED !!\n");	
	bjk_glb_finish();
}

void core_main() {
	//test_send_irq1();
	//test_send_irq();
	//test_logs();
	test_send_msg();
	//test_abort();
	//test_minimal();
}

