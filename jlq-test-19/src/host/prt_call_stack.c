
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <elf.h>

static void lookup_sections(const void *file, int addrs_sz, void** stack_addrs);

#define MAX_CALL_STACK_SZ 100	// should be >= BJ_MAX_CALL_STACK_SZ

const char* stack_names[MAX_CALL_STACK_SZ];

int bjh_prt_call_stack(const char *elf_nm, int addrs_sz, void** stack_addrs)
{
	int          fd;
	struct stat  st;
	void        *file;
	
	//memset(stack_addrs, 0, addrs_sz * sizeof(void*));

	fd = open(elf_nm, O_RDONLY);
	if (fd == -1) {
		printf("ERROR: Can't open elf file \"%s\".\n", elf_nm);
		return 1;
	}

	if (fstat(fd, &st) == -1) {
		printf("ERROR: Can't stat elf file \"%s\".\n", elf_nm);
		close(fd);
		return 1;
    }

	file = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file == MAP_FAILED) {
		printf("ERROR: Can't mmap elf file \"%s\".\n", elf_nm);
		close(fd);
		return 1;
    }

	lookup_sections(file, addrs_sz, stack_addrs);

	munmap(file, st.st_size);
	close(fd);
	return 0;
}

static int16_t find_addr(int addrs_sz, void** stack_addrs, void* addr){
	int16_t aa = -1;
	for(aa = 0; aa < addrs_sz; aa++){
		if(stack_addrs[aa] == NULL){
			aa = -1;
			break;
		}
		if(stack_addrs[aa] == addr){
			break;
		}
	}
	return aa;
}

static void lookup_sections(const void *file, int addrs_sz, void** stack_addrs)
{
	uint16_t ii;
	uint16_t kk;
	//size_t jj;
	Elf32_Ehdr *ehdr;
	//Elf32_Phdr *phdr;
	Elf32_Shdr *shdr;
	//Elf32_Shdr *sh_strtab;
	//const char *strtab;
	uint8_t *src = (uint8_t *) file;

	ehdr = (Elf32_Ehdr *) &src[0];
	//phdr = (Elf32_Phdr *) &src[ehdr->e_phoff];
	shdr = (Elf32_Shdr *) &src[ehdr->e_shoff];
	int shnum = ehdr->e_shnum;

	//sh_strtab = &shdr[ehdr->e_shstrndx];
	//strtab = (char *) &src[sh_strtab->sh_offset];
	
	memset(stack_names, 0, sizeof(stack_names));

	int16_t num_filled = 0;
	for (ii = 0; ii < shnum; ii++) {
		if(num_filled == addrs_sz){
			break;
		}
		
		//const char* nm = &strtab[shdr[ii].sh_name];
		//printf("Section %s  ii=%d offset=%d\n", nm, ii, shdr[ii].sh_offset);
		if(shdr[ii].sh_type == SHT_SYMTAB){
			//printf("SHT_SYMTAB\n");

			Elf32_Shdr *strs_shdr = &shdr[shdr[ii].sh_link];
			const char *strs_tab = (char *) &src[strs_shdr->sh_offset];
			
			Elf32_Word sz = shdr[ii].sh_size;
			Elf32_Word esz = shdr[ii].sh_entsize;
			
			if(esz > 0){
				Elf32_Word nument = sz/esz;
				Elf32_Sym *symtab = (Elf32_Sym *) &src[shdr[ii].sh_offset];
				
				for(kk = 0; kk < nument; kk++){
					const char* nm2 = &strs_tab[symtab[kk].st_name];
					void *addr = (void *)(symtab[kk].st_value);

					if(nm2 == NULL){
						continue;
					}
					if(addr == NULL){
						continue;
					}
					
					int16_t idx = find_addr(addrs_sz, stack_addrs, addr);
					if(idx < MAX_CALL_STACK_SZ){
						if(stack_names[idx] == 0){
							stack_names[idx] = nm2;
							num_filled++;
						}
					}
					if(num_filled == addrs_sz){
						break;
					}
					
					/*if(symtab[kk].st_name != 0){
						printf("\t -------------------------------\n");
						printf("\t nm2=%s\n", nm2);
						printf("\t st_name=%d\n", symtab[kk].st_name);
						printf("\t st_value=0x%08x\n", symtab[kk].st_value);
						//printf("\t st_size=%d\n", symtab[kk].st_size);
						//printf("\t st_info=%u\n", symtab[kk].st_info);
						//printf("\t st_other=%u\n", symtab[kk].st_other);
						//printf("\t st_shndx=%d\n", symtab[kk].st_shndx);
					}*/
				}
			}
		}
	}

	printf("CALL_STACK=[\n");
	for(kk = 0; kk < MAX_CALL_STACK_SZ; kk++){
		if(kk == addrs_sz){
			break;
		}
		if(stack_addrs[kk] == NULL){
			break;
		}
		printf("%s (%p)\n", stack_names[kk], stack_addrs[kk]);
	}
	printf("]\n");
}

