
//----------------------------------------------------------------------------
// actor.hh

#ifndef ACTOR_HH
#define ACTOR_HH

#include "binder.hh"
#include "global.h"

class agent;
class actor;
class missive;
class agent_ref;
class agent_grp;

typedef agent_grp missive_grp_t;
typedef agent_ref missive_ref_t;
typedef agent_grp actor_grp_t;
typedef agent_ref actor_ref_t;

//-------------------------------------------------------------------------
// actor ids

#define bjk_actor_id(cls) BJ_ACTOR_ID_##cls

enum bjk_actor_id_t : uint8_t {
	bjk_invalid_actor = 0,

	bjk_actor_id(agent),
	bjk_actor_id(actor),
	bjk_actor_id(missive),
	bjk_actor_id(agent_ref),
	bjk_actor_id(agent_grp),

	bjk_tot_actor_ids
};

//-------------------------------------------------------------------------
// dyn mem

#define bjk_all_available(nam) kernel::get_sys()->cls_available_##nam

#define bjk_get_available(nam) \
	grip& ava = bjk_all_available(nam); \
	if(! ava.is_alone()){ \
		binder* fst = bjk_pt_to_binderpt(ava.bn_right); \
		fst->let_go(); \
		return (nam *)fst; \
	} \

// end_macro

#define BJK_DEFINE_SEPARATE(nam) \
void \
nam::separate(uint16_t sz){ \
	grip& ava = bjk_all_available(nam); \
	nam* obj = nam::acquire(sz); \
	for(int bb = 0; bb < sz; bb++){ \
		obj[bb].let_go(); \
		ava.bind_to_my_left(obj[bb]); \
	} \
} \

// end_macro

#define BJK_DEFINE_ACQUIRE(nam) \
nam* \
nam::acquire(uint16_t sz){ \
	if(sz == 1){ \
		bjk_get_available(nam); \
	} \
	nam* obj = bjk_malloc32(nam, sz); \
	if(obj == bj_null){ \
		bjk_abort((bj_addr_t)nam::acquire, 0, bj_null); \
	} \
	for(int bb = 0; bb < sz; bb++){ \
		new (&(obj[bb])) nam(); \
	} \
	return obj; \
} \

// end_macro


//-------------------------------------------------------------------------
// handler ids

#define bjk_handler_idx(cls) BJ_CLASS_IDX_##cls

enum bjk_handler_idx_t : uint8_t {
	bjk_invalid_handler = 0,

	bjk_handler_idx(actor),

	bjk_tot_handler_ids
};

enum bjk_core_state_t : uint8_t {
	bjk_invalid_state = 0,
	bjk_inited_state
};

typedef void (*missive_handler_t)(missive* msg);

//-------------------------------------------------------------------------
// kernel data

enum bjk_route_t : uint8_t {
	bjk_up_route = 0,
	bjk_down_route,
	bjk_left_route,
	bjk_right_route,
	bjk_tot_routes
};

enum bjk_signal_t : uint8_t {
	bjk_do_pw0_routes_sgnl = 0,
	bjk_do_pw2_routes_sgnl,
	bjk_do_pw4_routes_sgnl,
	bjk_do_pw6_routes_sgnl,
	bjk_do_direct_routes_sgnl,
	bjk_tot_signals
};

//-------------------------------------------------------------------------
// kernel class

#define kernel_signals_arr_sz bjk_tot_signals
#define kernel_handlers_arr_sz bjk_tot_handler_ids
#define kernel_pw0_routed_arr_sz bj_out_num_cores
#define kernel_pw2_routed_arr_sz bjk_tot_routes
#define kernel_pw4_routed_arr_sz bjk_tot_routes
#define kernel_pw6_routed_arr_sz bjk_tot_routes

#define kernel_class_names_arr_sz bjk_tot_actor_ids

class kernel { 
public:
	bj_bool_t signals_arr[kernel_signals_arr_sz];

	missive_handler_t handlers_arr[kernel_handlers_arr_sz];
	missive_grp_t* pw0_routed_arr[kernel_pw0_routed_arr_sz];
	missive_grp_t* pw2_routed_arr[kernel_pw2_routed_arr_sz];
	missive_grp_t* pw4_routed_arr[kernel_pw4_routed_arr_sz];
	missive_grp_t* pw6_routed_arr[kernel_pw6_routed_arr_sz];

	grip direct_routed;

	grip in_work;
	grip local_work;
	grip out_work;
	grip sent_work;

	char* class_names_arr[kernel_class_names_arr_sz];

	grip 	cls_available_actor;
	grip 	cls_available_missive;
	grip 	cls_available_agent_ref;
	grip 	cls_available_agent_grp;

	actor* 	first_actor;

	kernel(){
		init_kernel();
	}

	~kernel(){}

	void init_kernel() bj_code_dram;

	static void
	init_sys() bj_code_dram;

	static void
	finish_sys() bj_code_dram;

	static kernel*
	get_sys();

	static bj_inline_fn bj_in_core_st& 
	get_in_shd(){
		return *bjk_get_glb_in_core_shd();
	}

	static bj_inline_fn bjk_glb_sys_st& 
	get_glb(){
		return *bjk_get_glb_sys();
	}

	static bj_inline_fn bj_off_core_st& 
	get_off_shd(){
		return *(bjk_get_glb_sys()->off_core_pt);
	}

	static bj_inline_fn bj_sys_sz_st& 
	get_sys_sz(){
		return bjk_get_glb_sys()->sys_sz;
	}

	static bj_inline_fn bj_core_nn_t 
	get_core_nn(){
		return bjk_get_glb_in_core_shd()->the_core_nn;
	}

	static bj_inline_fn bj_core_co_t 
	get_core_ro(){
		return bjk_get_glb_in_core_shd()->the_core_ro;
	}

	static bj_inline_fn bj_core_co_t 
	get_core_co(){
		return bjk_get_glb_in_core_shd()->the_core_co;
	}

	static bj_inline_fn bj_core_id_t 
	get_core_id(){
		return bjk_get_glb_in_core_shd()->the_core_id;
	}

	static bj_inline_fn actor*
	get_core_actor(){
		return get_sys()->first_actor;
	}

	static actor*
	get_core_actor(bj_core_id_t dst_id);

	static void
	set_handler(missive_handler_t hdlr, uint16_t idx) bj_code_dram;

	static bj_opt_sz_fn void 
	actors_handle_loop();

	bj_opt_sz_fn void 
	process_signal(int sz, missive_grp_t** arr);

	bj_opt_sz_fn void 
	handle_missives();

	bj_opt_sz_fn void 
	add_out_missive(missive& msv);

	bj_opt_sz_fn void 
	call_handlers_of_group(missive_grp_t* mgrp);

};

#define bjk_is_valid_handler_idx(ker, idx) \
	((idx >= 0) && (idx < kernel_handlers_arr_sz) && ((ker->handlers_arr)[idx] != bj_null))

#define bjk_is_valid_handler_idx2(idx) ((idx >= 0) && (idx < kernel_handlers_arr_sz))

#define bj_class_name(cls) const_cast<char*>("{" #cls "}");

#define bjk_is_valid_class_name_idx(id) ((id >= 0) && (id < kernel_class_names_arr_sz))

#define bjk_set_class_name(cls) class_names_arr[bjk_actor_id(cls)] = bj_class_name(cls)

//-------------------------------------------------------------------------
// agent class 

class agent: public binder{
public:
	static
	bjk_actor_id_t	THE_ACTOR_ID;

	bj_opt_sz_fn 
	agent(){}

	bj_opt_sz_fn 
	~agent(){}

	virtual bj_opt_sz_fn 
	bjk_actor_id_t	get_actor_id(){
		return agent::THE_ACTOR_ID;
	}

	virtual bj_opt_sz_fn 
	grip&	get_available() bj_code_dram;

	virtual bj_opt_sz_fn 
	void	init_me() bj_code_dram;

	bj_opt_sz_fn 
	void	release(){
		let_go();
		init_me();
		grip& ava = get_available();
		ava.bind_to_my_left(*this);
	}

	bj_inline_fn
	agent*	get_glb_ptr(){
		return (agent*)bjk_as_glb_pt(this);
	}

	virtual
	char* 	get_class_name() bj_code_dram;
};

//-------------------------------------------------------------------------
// actor class 

class actor: public agent {
public:
	static
	bjk_actor_id_t	THE_ACTOR_ID;
	static
	actor*			acquire(uint16_t sz = 1);
	static
	void			separate(uint16_t sz) bj_code_dram;

	bjk_handler_idx_t 	id_idx;
	bjk_flags_t 		flags;

	bj_opt_sz_fn 
	actor(){
		init_me();
	}

	bj_opt_sz_fn 
	~actor(){}

	virtual bj_opt_sz_fn 
	void init_me(){
		id_idx = bjk_handler_idx(actor);
		flags = 0;
	}

	virtual
	bj_opt_sz_fn bjk_actor_id_t	get_actor_id(){
		return actor::THE_ACTOR_ID;
	}

	virtual
	bj_opt_sz_fn grip&	get_available(){
		return bjk_all_available(actor);
	}
};

//-------------------------------------------------------------------------
// missive class

typedef bjk_sptr_t bjk_actor_sptr_t;
typedef uint16_t	bjk_token_t; 

class missive : public agent {
public:
	static
	bjk_actor_id_t 	THE_ACTOR_ID;
	static
	missive*		acquire(uint16_t sz = 1);
	static
	void			separate(uint16_t sz) bj_code_dram;

	actor* 				dst;
	bjk_actor_sptr_t 	src;
	bjk_token_t 		tok;

	bj_opt_sz_fn 
	missive(){
		init_me();
	}

	bj_opt_sz_fn 
	~missive(){}

	virtual bj_opt_sz_fn 
	void init_me(){
		dst = bj_null;
		src = bjk_binderpt_to_pt(bj_null);
		tok = 0;
	}

	bj_inline_fn 
	void send(){
		kernel::get_sys()->local_work.bind_to_my_left(*this);
	}

	virtual
	bj_opt_sz_fn bjk_actor_id_t	get_actor_id(){
		return missive::THE_ACTOR_ID;
	}

	virtual
	bj_opt_sz_fn grip&	get_available(){
		return bjk_all_available(missive);
	}

	bj_inline_fn actor*
	get_source(){
		return (actor*)bjk_pt_to_binderpt(src);
	}

	bj_inline_fn void
	set_source(actor* act){
		src = bjk_binderpt_to_pt(act);
	}
};

//-------------------------------------------------------------------------
// agent_grp class 

class agent_grp : public agent {
public:
	static
	bjk_actor_id_t 	THE_ACTOR_ID;
	static
	agent_grp*		acquire(uint16_t sz = 1);
	static
	void			separate(uint16_t sz) bj_code_dram;

	grip		all_agts;
	uint8_t 	tot_agts;	// optional use
	bj_bool_t 	handled;

	bj_opt_sz_fn 
	agent_grp(){
		init_me();
	}

	bj_opt_sz_fn 
	~agent_grp(){}

	virtual bj_opt_sz_fn 
	void init_me(){
		tot_agts = 0;
		handled = bj_false;
	}

	virtual
	bj_opt_sz_fn bjk_actor_id_t	get_actor_id(){
		return agent_grp::THE_ACTOR_ID;
	}

	virtual
	bj_opt_sz_fn grip&	get_available(){
		return bjk_all_available(agent_grp);
	}

	void
	release_all_agts();
};

//-------------------------------------------------------------------------
// agent_ref class 

class agent_ref : public agent {
public:
	static
	bjk_actor_id_t 	THE_ACTOR_ID;
	static
	agent_ref* 		acquire(uint16_t sz = 1);
	static
	void			separate(uint16_t sz) bj_code_dram;

	agent* 		glb_agent_ptr;

	bj_opt_sz_fn 
	agent_ref(){
		init_me();
	}

	bj_opt_sz_fn 
	~agent_ref(){}

	virtual bj_opt_sz_fn 
	void init_me(){
		glb_agent_ptr = bj_null;
	}

	virtual
	bj_opt_sz_fn bjk_actor_id_t	get_actor_id(){
		return agent_ref::THE_ACTOR_ID;
	}

	virtual
	bj_opt_sz_fn grip&	get_available(){
		return bjk_all_available(agent_ref);
	}
};

void 
actor_handler(missive* msg);

#ifdef __cplusplus
bj_c_decl {
#endif

bj_opt_sz_fn 
void 
bjk_send_irq(bj_core_id_t koid, uint16_t num_irq);

void
wait_inited_state(bj_core_id_t dst) bj_code_dram;

void
init_class_names() bj_code_dram;

void
ck_sizes() bj_code_dram;

void test_minimal() bj_code_dram;

#ifdef __cplusplus
}
#endif


//static  void
//__static_initialization_and_destruction_0(int, int) bj_code_dram;
// ignored
// pragma GCC diagnostic warning "-fpermissive"


#endif		// ACTOR_HH
