/*
  File: e-loader.c

  This file is part of the Epiphany Software Development Kit.

  Copyright (C) 2013 Adapteva, Inc.
  See AUTHORS for list of contributors.
  Support e-mail: <support@adapteva.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License (LGPL)
  as published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  and the GNU Lesser General Public License along with this program,
  see the files COPYING and COPYING.LESSER.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <err.h>
#include <elf.h>
#include <inttypes.h>

#include "e-hal.h"
#include "core_loader_znq.h"
//include "esim-target.h"
#include "shared.h"

#ifndef BJL_MARK_USED
#define BJL_MARK_USED(X)  ((void)(&(X)))
#endif

void ee_get_coords_from_id(e_epiphany_t *dev, unsigned coreid,
								  unsigned *row, unsigned *col);


#define bjl_pt_out_of_range(pt, rs, rf) (((pt) < (rs)) || ((pt) > (rf)))

#define BJL_ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))

#define bjl_diag(vN)   if (bjl_load_verbose >= vN)

extern e_platform_t e_platform;

typedef enum {
	L_D0 = 0,
	L_D1 = 1,
	L_D2 = 2,
	L_D3 = 3,
	L_D4 = 40,
} bjl_loader_diag_t;

enum bjl_loader_sections {
	BJL_SEC_WORKGROUP_CFG,
	BJL_SEC_EXT_MEM_CFG,
	BJL_SEC_LOADER_CFG,
	BJL_SEC_NUM,
};

struct bjl_section_info {
	const char *name;
	bool present;
	Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
};
// TODO: These should be defined in common header file
#define BJL_BSS_CLEARED_FLAG 1
#define BJL_CUSTOM_ARGS_FLAG 2

struct bjl_loader_cfg {
	uint32_t flags;
	uint32_t __pad1;
	uint32_t args_ptr;
	uint32_t __pad2;
} __attribute__((packed));

void bjl_lookup_sections(const void *file, struct bjl_section_info *tbl,
							size_t tbl_size);

e_return_stat_t bjl_process_elf(const void *file, e_epiphany_t *dev,
									  e_mem_t *emem, int row, int col);

bjl_loader_diag_t bjl_load_verbose = L_D3;

/* bjl_diag_fd is set by set_loader_verbosity() */
FILE *bjl_diag_fd = bj_null;

// TODO: replace with platform data
#define BJL_EMEM_SIZE (0x02000000)

uint8_t* BJL_BASE_PT = bj_null;

void
bj_ck_memload(uint8_t* dst, uint8_t* src, size_t sz){
	bool ok = true;
	long aa;
	for(aa = 0; aa < sz; aa++){
		if(dst[aa] != src[aa]){
			ok = false;
			break;
		}
	}
	if(! ok){
		write_file("SOURCE_shd_mem_dump.dat", src, sz, false);
		write_file("DEST_shd_mem_dump.dat", dst, sz, false);
		bjh_abort_func(9, "bj_ck_memload() FAILED !! CODE_LOADING_FAILED !!\n");
	}
}

uint8_t*
bj_memload(uint8_t* dest, const uint8_t* src, bj_size_t sz){
	bj_size_t idx = 0;
	for(idx = 0; idx < sz; idx++){
		bj_force_assig(dest[idx], src[idx]);
	}
	return dest;
}

unsigned long 
bjl_rndl_page(unsigned long size)
{
	unsigned long page_size;
	unsigned long rsize;

	// Get OS memory page size
	page_size = sysconf(_SC_PAGE_SIZE);

	// Find lower integral number of pages
	rsize = (size / page_size) * page_size;

	return rsize;
}

/*
	off_t			 phy_base;	  // physical global base address of external memory segment as seen by host
	off_t			 ephy_base;	  // physical global base address of external memory segment as seen by devic
	size_t			 size;		  // size of eDRAM allocated buffer for host side
*/

void
print_platform_segs(e_platform_t* plat){
	if(plat == NULL){
		return;
	}
	int num = plat->num_emems;
	e_memseg_t *all_seg = plat->emem;
	printf("PLATAFORM NUM SEGMENTS=%d \n", num);
	int aa;
	for(aa = 0; aa < num; aa++){
		printf("objtype=%d phy_base=%lu ephy_base=%lu size=%u type=%d \n", 
			all_seg[aa].objtype, all_seg[aa].phy_base, all_seg[aa].ephy_base, all_seg[aa].size, all_seg[aa].type);
	}
	unsigned long psz = sysconf(_SC_PAGE_SIZE);
	printf("page size=%lu \n", psz);
	printf("------------------------------end of platform segments\n");
}

// prints:
// objtype=0 phy_base=2382364672 ephy_base=2382364672 size=33554432 type=0
// page size=4096

/*
typedef struct e_mem_t {
	e_objtype_t		 objtype;	  // object type identifier
	off_t			 phy_base;	  // physical global base address of external memory buffer as seen by host side
	off_t			 page_base;	  // physical base address of memory page
	off_t			 page_offset; // offset of memory region base to memory page base
	size_t			 map_size;	  // size of eDRAM allocated buffer for host side
	off_t			 ephy_base;	  // physical global base address of external memory buffer as seen by device side
	size_t			 emap_size;	  // size of eDRAM allocated buffer for device side
	void			*mapped_base; // for mmap
	void			*base;		  // application (virtual) space base address of external memory buffer
	int				 memfd;		  // for mmap

	void			*priv;		  // Target private
} e_mem_t;

*/

void
print_mem_data(e_mem_t *mbuf){
	printf("MEM_DATA \n");
	printf("phy_base=%lu page_base=%lu page_offset=%lu map_size=%u ephy_base=%lu \n", 
		mbuf->phy_base, mbuf->page_base, mbuf->page_offset, mbuf->map_size, mbuf->ephy_base);
	printf("emap_size=%u mapped_base=%p base=%p memfd=%d \n", 
		mbuf->emap_size, mbuf->mapped_base, mbuf->base, mbuf->memfd);
	printf("-------------- end of mem data \n");
}

// prints:
// phy_base=2382364672 page_base=2382364672 page_offset=0 map_size=33554432 ephy_base=2382364672 
// emap_size=33554432 mapped_base=0x4 base=0x1000000 memfd=5

#define BJL_EPIPHANY_DEV "/dev/epiphany/mesh0"
int 
bjl_shd_alloc(e_mem_t *mbuf, off_t offset, size_t size)
{
	if (e_platform.initialized == E_FALSE)
	{
		warnx("shd_alloc(): Platform was not initialized. Use e_init().");
		return E_ERR;
	}

	mbuf->objtype = E_EXT_MEM;
	mbuf->priv = NULL;

	bjl_diag(L_D2) { fprintf(bjl_diag_fd, 
		"shd_alloc(): allocating EMEM buffer at offset 0x%08x\n", (uint) offset); 
		print_platform_segs(&e_platform);
	}

	// TODO: this takes only the 1st segment into account
	mbuf->phy_base = (e_platform.emem[0].phy_base + offset); 
	mbuf->page_base = bjl_rndl_page(mbuf->phy_base);
	mbuf->page_offset = mbuf->phy_base - mbuf->page_base;
	mbuf->map_size = size + mbuf->page_offset;

	// TODO: this takes only the 1st segment into account
	mbuf->ephy_base = (e_platform.emem[0].ephy_base + offset); 
	mbuf->emap_size = size;

	mbuf->memfd = open(BJL_EPIPHANY_DEV, O_RDWR | O_SYNC);
	if (mbuf->memfd == -1){
		warnx("shd_alloc(): BJL_EPIPHANY_DEV file open failure.");
		return E_ERR;
	}

	bjl_diag(L_D2) { fprintf(bjl_diag_fd, 
		"shd_alloc(): mbuf.phy_base = 0x%08x, mbuf.ephy_base = 0x%08x, mbuf.base = 0x%08x, mbuf.size = 0x%08x\n",
		(uint) mbuf->phy_base, (uint) mbuf->ephy_base, (uint) mbuf->base, (uint) mbuf->map_size); 
		print_mem_data(mbuf);
		//printf("TYPE ENTER (before mmap)\n");
		//getchar();
	}

	mbuf->mapped_base = mmap(NULL, mbuf->map_size, PROT_READ|PROT_WRITE, 
			MAP_SHARED, mbuf->memfd, mbuf->page_base);
	//mbuf->mapped_base = mmap(NULL, mbuf->map_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, 
	//							mbuf->memfd, mbuf->page_base);	// FAILS BEFORE ABORT !
	mbuf->base = (void*)(((char*)mbuf->mapped_base) + mbuf->page_offset);

	/*bjl_diag(L_D2) { 
		printf("TYPE ENTER (after mmap) mbuf->base=%p \n", mbuf->base); 
		getchar();
	}*/

	if (mbuf->mapped_base == MAP_FAILED){
		warnx("shd_alloc(): mmap failure.");
		return E_ERR;
	}
	// DIFF OF "/proc/PID/mpas is
	// b4b92000-b6b92000 rw-s 8e000000 00:06 2684       /dev/epiphany/mesh0
	// TYPE ENTER (after mmap) mbuf->base=0xb4b92000

	return E_OK;
}

int 
bjl_shd_free(e_mem_t *mbuf)
{
	if (!mbuf)
		return E_ERR;

	if (E_SHARED_MEM == mbuf->objtype) {
		// The shared memory mapping is persistent - don't unmap
		return E_OK;
	}

	munmap(mbuf->mapped_base, mbuf->map_size);
	close(mbuf->memfd);

	return E_OK;
}

#define EM_ADAPTEVA_EPIPHANY   0x1223  /* Adapteva's Epiphany architecture.  */
static inline bool bjl_is_epiphany_exec_elf(Elf32_Ehdr *ehdr)
{
	return ehdr
		&& memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0
		&& ehdr->e_ident[EI_CLASS] == ELFCLASS32
		&& ehdr->e_type == ET_EXEC
		&& ehdr->e_version == EV_CURRENT
		&& ehdr->e_machine == EM_ADAPTEVA_EPIPHANY;
}

static bool bjl_is_srec_file(const char *hdr)
{
	const char srechdr[] = {'S', '0'};
	return (memcmp(hdr, srechdr, sizeof(srechdr)) == 0);
}

int bj_load(const char *executable, e_epiphany_t *dev, unsigned row, unsigned col, e_bool_t start)
{
	int status;

	status = bj_load_group(executable, dev, row, col, 1, 1, start);

	return status;
}

static void bjl_clear_sram(e_epiphany_t *dev,
					   unsigned row, unsigned col,unsigned rows, unsigned cols)
{
	unsigned i, j;
	size_t sram_size;
	void *empty;

	/* Assume one chip type */
	sram_size = e_platform.chip[0].sram_size;

	empty = alloca(sram_size);
	memset(empty, 0, sram_size);

	for (i = row; i < row + rows; i++){
		for (j = col; j < col + cols; j++){
			e_write(dev, i, j, 0, empty, sram_size);
		}
	}
}

int bj_load_group(const char *executable, e_epiphany_t *dev, unsigned row, unsigned col, 
			unsigned rows, unsigned cols, e_bool_t start)
{
	e_mem_t      emem;
	unsigned int irow, icol, i;
	int          status;
	//int          fd;
	struct stat  st;
	void        *file;
	bool         is_srec = false;
	e_return_stat_t retval;

	e_set_host_verbosity(H_D0);
	bjl_diag_fd = stderr;

	/*int eph_fd = open("/dev/epiphany/mesh0", O_RDWR | O_SYNC);
	if (eph_fd == -1){
		bjh_abort_func(9, "bj_load_group() Cannot open epiphany device.\n");
		return -1;
	}*/

	struct bjl_section_info tbl[] = {
		{ .name = "workgroup_cfg" },
		{ .name = "ext_mem_cfg" },
		{ .name = "loader_cfg" },
	};

	status = E_OK;

	if (!dev) {
		warnx("ERROR: Can't connect to Epiphany or external memory.\n");
		return E_ERR;
	}

	// Allocate External DRAM for the epiphany executable code
	// TODO: this is barely scalable. Really need to test ext. mem size to load
	// and possibly split the ext. mem accesses into 1MB chunks.

	//if (e_alloc(&emem, 0, BJL_EMEM_SIZE)) {
	if (bjl_shd_alloc(&emem, 0, BJL_EMEM_SIZE)) {
		warnx("\nERROR: Can't allocate external memory buffer!\n\n");
		return E_ERR;
	}

	BJL_BASE_PT = (uint8_t*)(emem.base);

	/*
	fd = open(executable, O_RDONLY);
	if (fd == -1) {
		warnx("ERROR: Can't open executable file \"%s\".\n", executable);
		e_free(&emem);
		return E_ERR;
	}

	if (fstat(fd, &st) == -1) {
		warnx("ERROR: Can't stat file \"%s\".\n", executable);
		close(fd);
		e_free(&emem);
		return E_ERR;
    }

	BJH_CK(st.st_size < bjh_code_loader_buff_sz);
	file = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file == MAP_FAILED) {
		warnx("ERROR: Can't mmap file \"%s\".\n", executable);
		close(fd);
		e_free(&emem);
		return E_ERR;
    }
	*/
	off_t fsize;
	uint8_t* fdata = read_file((char*)executable, &fsize);
	file = fdata;

	uint8_t* pt_mem_start = (uint8_t*)(emem.base);
	uint8_t* pt_mem_end = pt_mem_start + BJL_EMEM_SIZE;
	uint8_t* pt_file_start = (uint8_t*)(file);
	uint8_t* pt_file_end = pt_mem_start + st.st_size;
	bjl_diag(L_D3) {
		fprintf(bjl_diag_fd, "bj_load_group(): mem_beg=%p end=%p file_beg=%p end=%p\n",
				pt_mem_start, pt_mem_end, pt_file_start, pt_file_end); 
	}

	if (bjl_is_epiphany_exec_elf((Elf32_Ehdr *) file)) {
		bjl_diag(L_D1) { fprintf(bjl_diag_fd, "load_group(): loading ELF file %s ...\n", executable); }
	} else if (bjl_is_srec_file((char *) file)) {
		is_srec = true;
		warnx("load_group(): WARNING: SREC file support is deprecated and will be removed in the next ESDK release. Use ELF format instead.\n");
	} else {
		bjl_diag(L_D1) { fprintf(bjl_diag_fd, "load_group(): ERROR: unidentified file format\n"); }
		warnx("ERROR: Can't load executable file: unidentified format.\n");
		status = E_ERR;
		goto out;
	}

	if (is_srec) {
		/* No symbol info in SREC files, use hard coded values */
		tbl[BJL_SEC_WORKGROUP_CFG].present = true;
		tbl[BJL_SEC_WORKGROUP_CFG].sh_addr = 0x28;
		tbl[BJL_SEC_EXT_MEM_CFG].present   = true;
		tbl[BJL_SEC_EXT_MEM_CFG].sh_addr   = 0x50;
		tbl[BJL_SEC_LOADER_CFG].present    = true;
		tbl[BJL_SEC_LOADER_CFG].sh_addr    = 0x58;
	} else {
		bjl_lookup_sections(file, tbl, BJL_ARRAY_SIZE(tbl));
	}

	for (i = 0; i < BJL_SEC_NUM; i++) {
		if (!tbl[i].present) {
			warnx("load_group(): WARNING: %s section not in binary.",
				  tbl[i].name);
		}
	}

	bjl_clear_sram(dev, row, col, rows, cols);

	for (irow=row; irow<(row+rows); irow++) {
		for (icol=col; icol<(col+cols); icol++) {
			if (is_srec){
				warnx("ERROR: SREC FORMAT NOT SUPPORTED ANYMORE.\n");
				retval = E_ERR;
			} else {
				retval = bjl_process_elf(file, dev, &emem, irow, icol);
			}

			if (retval == E_ERR) {
				warnx("ERROR: Can't load executable file \"%s\".\n", executable);
				status = E_ERR;
				goto out;
			}
		}
	}

	if (start) {
		for (irow=row; irow<(row+rows); irow++) {
			for (icol=col; icol<(col + cols); icol++) {
				bjl_diag(L_D1) {
					fprintf(bjl_diag_fd,
							"load_group(): send SYNC signal to core (%d,%d)...\n",
							irow, icol); }
				e_start(dev, irow, icol);
				bjl_diag(L_D1) {fprintf(bjl_diag_fd, "load_group(): done.\n"); }
			}
		}
	}

	bjl_diag(L_D1) { fprintf(bjl_diag_fd, "load_group(): done loading.\n"); }

out:
	//munmap(file, st.st_size);
	//close(fd);
	free(fdata);
	//e_free(&emem);
	bjl_shd_free(&emem);

	return status;
}

void bjl_lookup_sections(const void *file, struct bjl_section_info *tbl,
							size_t tbl_size)
{
	int i;
	size_t j;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	Elf32_Shdr *shdr, *sh_strtab;
	const char *strtab;
	uint8_t *src = (uint8_t *) file;

	BJL_MARK_USED(phdr);

	ehdr = (Elf32_Ehdr *) &src[0];
	phdr = (Elf32_Phdr *) &src[ehdr->e_phoff];
	shdr = (Elf32_Shdr *) &src[ehdr->e_shoff];
	int shnum = ehdr->e_shnum;

	sh_strtab = &shdr[ehdr->e_shstrndx];
	strtab = (char *) &src[sh_strtab->sh_offset];

	for (i = 0; i < shnum; i++) {
		for (j = 0; j < tbl_size; j++) {
			if (tbl[j].present)
				continue;

			if (strcmp(&strtab[shdr[i].sh_name], tbl[j].name) != 0)
				continue;

			tbl[j].present = true;
			tbl[j].sh_addr = shdr[i].sh_addr;
		}
	}
}

#define BJL_COREID(_addr) ((_addr) >> 20)
static inline bool bjl_is_local(uint32_t addr)
{
	return BJL_COREID(addr) == 0;
}

static bool bjl_is_valid_addr(uint32_t addr)
{
	return bjl_is_local(addr)
		|| e_is_addr_on_chip((void *) addr)
		|| e_is_addr_in_emem(addr);
}

static bool bjl_is_valid_range(uint32_t from, uint32_t size)
{
	/* Always allow empty range */
	if (!size)
		return true;

	return bjl_is_valid_addr(from) && bjl_is_valid_addr(from + size - 1);
}

e_return_stat_t
bjl_process_elf(const void *file, e_epiphany_t *dev, e_mem_t *emem,
			   int row, int col)
{
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	bool       islocal, isonchip, isexternal;
	int        ihdr;
	unsigned  globrow, globcol;
	unsigned   coreid;
	uintptr_t  dst;
	/* TODO: Make src const (need fix in esim.h first) */
	uint8_t   *src = (uint8_t *) file;

	islocal  = false;
	isonchip = false;
	isexternal = false;
	BJL_MARK_USED(isexternal);

	ehdr = (Elf32_Ehdr *) &src[0];
	phdr = (Elf32_Phdr *) &src[ehdr->e_phoff];

	/* Range-check sections */
	for (ihdr = 0; ihdr < ehdr->e_phnum; ihdr++) {
		if (!bjl_is_valid_range(phdr[ihdr].p_vaddr, phdr[ihdr].p_memsz))
			return E_ERR;
	}

	for (ihdr = 0; ihdr < ehdr->e_phnum; ihdr++) {
		/* Nothing to do if section is empty */
		if (!phdr[ihdr].p_memsz)
			continue;

		islocal = bjl_is_local(phdr[ihdr].p_vaddr);
		isonchip = islocal ? true
						   /* TODO: Don't cast to void */
						   : e_is_addr_on_chip((void *) ((uintptr_t) phdr[ihdr].p_vaddr));
		isexternal = false;

		bjl_diag(L_D3) {
			fprintf(bjl_diag_fd, "process_elf(): copying the data (%d bytes)",
					phdr[ihdr].p_filesz); }

		/* Address calculation */
		char* dbg_case = "(undef)";
		uint32_t the_p_vaddr = phdr[ihdr].p_vaddr;
		if (islocal) {
			bjl_diag(L_D3) { dbg_case = "(local)"; fprintf(bjl_diag_fd, " (local) to core (%d,%d)\n", row, col); }

			// TODO: should this be p_paddr instead of p_vaddr?
			dst = ((uintptr_t) dev->core[row][col].mems.base)
				+ phdr[ihdr].p_vaddr;
		} else if (isonchip) {
			coreid = phdr[ihdr].p_vaddr >> 20;

			ee_get_coords_from_id(dev, coreid, &globrow, &globcol);
			//bj_core_co_t g_ro = bj_id_to_ro(coreid);
			//bj_core_co_t g_co = bj_id_to_co(coreid);
			//BJH_CK(g_ro == globrow);
			//BJH_CK(g_co == globcol);

			bjl_diag(L_D3) { dbg_case = "(onchip)"; 
				fprintf(bjl_diag_fd, " (onchip) to core (%d,%d)\n", globrow, globcol); }
			// TODO: should this be p_paddr instead of p_vaddr?
			dst = ((uintptr_t) dev->core[globrow][globcol].mems.base)
				+ (phdr[ihdr].p_vaddr & 0x000fffff);
		} else {
			isexternal = true;
			// If it is not on an eCore, it's in external memory.
			bjl_diag(L_D3) { dbg_case = "(external)"; fprintf(bjl_diag_fd, " (external) to external memory.\n"); }
			dst = phdr[ihdr].p_vaddr - emem->ephy_base + (uintptr_t) emem->base;
			bjl_diag(L_D3) {
				fprintf(bjl_diag_fd,
						"process_elf(): converting virtual (%p) to physical (%p)...\n",
						(void*) phdr[ihdr].p_vaddr,
						(void*) dst); }

		}

		/* Write */
		uint8_t* pt_dst = (void *) dst;
		uint8_t* pt_src = &src[phdr[ihdr].p_offset];
		size_t blk_sz = phdr[ihdr].p_filesz;

		uint8_t* pt_dst_end = pt_dst + blk_sz;
		uint8_t* pt_src_end = pt_src + blk_sz;

		uint8_t* pt_end_code = (BJL_BASE_PT + BJ_SHARED_MEM_START_DISP);
		uint8_t* pt_end_mem = (BJL_BASE_PT + BJL_EMEM_SIZE);
		BJL_MARK_USED(pt_end_code);
		BJL_MARK_USED(pt_end_mem);

		bjl_diag(L_D1) { fprintf(bjl_diag_fd, 
				"LOADING(%s). row=%d col=%d dst=%p end=%p src=%p end=%p sz=%d p_vaddr=%04x (%p).\n", 
				dbg_case, row, col, pt_dst, pt_dst_end, pt_src, pt_src_end, blk_sz, the_p_vaddr, 
				(void*)the_p_vaddr); 
		}

		if(isexternal){
			if(pt_dst_end > pt_end_code){
				bjl_diag(L_D3) {
					fprintf(bjl_diag_fd,
							"process_elf(): SKIP section in SHARED DATA virtual (%p) to physical (%p). SIZE=%u\n",
							(void*) phdr[ihdr].p_vaddr, (void*) dst, blk_sz); 
				}
				continue;	// Its data (an structure) NOT code.
			}
		}

		//memcpy(pt_dst, pt_src, blk_sz);		// CHANGES MEMORY OUTSIDE OF RANGES !!!!! WHY !!!!
		bj_memload(pt_dst, pt_src, blk_sz);

		bj_ck_memload(pt_dst, pt_src, blk_sz);	// LEAVE THIS. IT CAN FAIL !!!!

		//memcpy((void *) dst, &src[phdr[ihdr].p_offset], phdr[ihdr].p_filesz);
		/* We might want to clear mem in range [p_filesz-p_memsz] here.
		 * .bss sections have this. For now assume all memory is cleared
		 * elsewhere. */
	}

	return E_OK;
}

