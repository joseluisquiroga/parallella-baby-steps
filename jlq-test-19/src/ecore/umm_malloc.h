// ----------------------------------------------------------------------------
// umm_malloc.h - a memory allocator for embedded systems (microcontrollers)
//
// See copyright notice in LICENSE.TXT
// ----------------------------------------------------------------------------

#ifndef UMM_MALLOC_H
#define UMM_MALLOC_H

#include <inttypes.h>

#include "shared.h"

#define umm_null 0x0
typedef uint32_t umm_size_t;
typedef uint16_t umm_idx_t;

#define umm_align(aa) bj_align(aa) 

#define umm_inline_fn inline __attribute__((always_inline)) 

#define umm_opt_sz_fn bj_opt_sz_fn

umm_inline_fn uint8_t *
umm_memset(uint8_t* dest, uint8_t val, umm_size_t sz){
	return bj_memset(dest, val, sz);
}

umm_inline_fn uint8_t *
umm_memcpy(uint8_t* dest, const uint8_t* src, umm_size_t sz){
	return bj_memcpy(dest, src, sz);
}

umm_inline_fn uint8_t *
umm_memmove(uint8_t* dest, const uint8_t* src, umm_size_t sz){
	return bj_memmove(dest, src, sz);
}

// ----------------------------------------------------------------------------
// 
// Support for this library in a multitasking environment is provided when
// you add bodies to the UMM_CRITICAL_ENTRY and UMM_CRITICAL_EXIT macros
// (see below)
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Size of the heap in bytes
#define UMM_MALLOC_CFG__HEAP_SIZE 8192

// ----------------------------------------------------------------------------
// A couple of macros to make packing structures less compiler dependent

#define UMM_H_ATTPACKPRE
#define UMM_H_ATTPACKSUF __attribute__((__packed__))

// ----------------------------------------------------------------------------
// A couple of macros to make it easier to protect the memory allocator
// in a multitasking system. You should set these macros up to use whatever
// your system uses for this purpose. You can disable interrupts entirely, or
// just disable task switching - it's up to you
//
// NOTE WELL that these macros MUST be allowed to nest, because umm_free() is
// called from within umm_malloc()

#define UMM_CRITICAL_ENTRY()
#define UMM_CRITICAL_EXIT()

// ----------------------------------------------------------------------------
// default algorithm

#define UMM_BEST_FIT

//define UMM_FIRST_FIT

// ----------------------------------------------------------------------------
// debug log macros

#define UMM_DBG_LOG_LEVEL 0

#undef UMM_DBG_LOG_TRACE
#undef UMM_DBG_LOG_DEBUG
#undef UMM_DBG_LOG_CRITICAL
#undef UMM_DBG_LOG_ERROR
#undef UMM_DBG_LOG_WARNING
#undef UMM_DBG_LOG_INFO
#undef UMM_DBG_LOG_FORCE

#if UMM_DBG_LOG_LEVEL >= 6
#  define UMM_DBG_LOG_TRACE( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define UMM_DBG_LOG_TRACE( format, ... )
#endif

#if UMM_DBG_LOG_LEVEL >= 5
#  define UMM_DBG_LOG_DEBUG( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define UMM_DBG_LOG_DEBUG( format, ... )
#endif

#if UMM_DBG_LOG_LEVEL >= 4
#  define UMM_DBG_LOG_CRITICAL( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define UMM_DBG_LOG_CRITICAL( format, ... )
#endif

#if UMM_DBG_LOG_LEVEL >= 3
#  define UMM_DBG_LOG_ERROR( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define UMM_DBG_LOG_ERROR( format, ... )
#endif

#if UMM_DBG_LOG_LEVEL >= 2
#  define UMM_DBG_LOG_WARNING( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define UMM_DBG_LOG_WARNING( format, ... )
#endif

#if UMM_DBG_LOG_LEVEL >= 1
#  define UMM_DBG_LOG_INFO( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define UMM_DBG_LOG_INFO( format, ... )
#endif

/*
include <stdio.h>
define UMM_DBG_LOG_FORCE( force, format, ... ) \
	{if(force) {printf( format, ## __VA_ARGS__  );}}
*/

#define UMM_DBG_LOG_FORCE( force, format, ... ) 

// ----------------------------------------------------------------------------
// umm structs

typedef struct UMM_HEAP_INFO_t {
	umm_idx_t totalEntries;
	umm_idx_t usedEntries;
	umm_idx_t freeEntries; 

	umm_idx_t totalBlocks; 
	umm_idx_t usedBlocks; 
	umm_idx_t freeBlocks; 
}
UMM_HEAP_INFO;

extern UMM_HEAP_INFO heapInfo;

umm_opt_sz_fn void *umm_info( void *ptr, int force );
umm_opt_sz_fn void *umm_malloc( umm_size_t size );
umm_opt_sz_fn void *umm_realloc( void *ptr, umm_size_t size );
umm_opt_sz_fn void umm_free( void *ptr );


UMM_H_ATTPACKPRE typedef struct umm_ptr_t {
	umm_idx_t next;
	umm_idx_t prev;
} UMM_H_ATTPACKSUF umm_ptr;


UMM_H_ATTPACKPRE typedef struct umm_block_t {
	union {
		umm_ptr used;
	} header;
	union {
		umm_ptr free;
		uint8_t data[4];
	} body;
} UMM_H_ATTPACKSUF umm_block;

#define UMM_FREELIST_MASK (0x8000)
#define UMM_BLOCKNO_MASK  (0x7FFF)

#define UMM_HEAP_NUM_BLOCKS (UMM_MALLOC_CFG__HEAP_SIZE / sizeof(umm_block))

extern umm_block umm_align(8) umm_heap[UMM_HEAP_NUM_BLOCKS];

#endif // UMM_MALLOC_H

