
// attribute.h

#ifndef BJ_ATTRIBUTE_H
#define BJ_ATTRIBUTE_H

#ifdef __cplusplus
extern "C"
{
#endif

//======================================================================
// function attibutes

// Use with static when needed

#define bj_inline_fn inline __attribute__((always_inline)) 
#define bj_naked_fn __attribute__((naked)) 
#define bj_isr_fn __attribute__((interrupt)) 
#define bj_asm __asm__ __volatile__

	
#ifdef __cplusplus
}
#endif

#endif // BJ_ATTRIBUTE_H

