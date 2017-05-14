
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <e-hal.h>

#include "shared.h"
#include "core_loader_znq.h"

void 
bjh_abort_func(long val, const char* msg){
	fprintf(stderr, "\nABORTING! %s\n", msg); 
	exit(val);
}

int main(int argc, char *argv[])
{
	e_platform_t platform;
	e_epiphany_t dev;
	e_mem_t emem_1;

	// initialize system, read platform params from
	// default HDF. Then, reset the platform and
	// get the actual system parameters.
	e_init(NULL);
	e_reset_system();
	e_get_platform_info(&platform);

	// Allocate a buffer in shared external memory
	// for message passing from eCore to host.
	if(e_alloc(&emem_1, 0x01000000, 1234567)){
		bjh_abort_func(0, "\nERROR: Can't allocate emem_1!\n\n");
		return -1;
	}
	
    	// Open a workgroup
	e_open(&dev, 0, 0, platform.rows, platform.cols);

	// Reset the workgroup
	e_reset_group(&dev);

	// Load the device program onto all the eCores
	bj_load_group("an_epiphany_exe.elf", &dev, 0, 0, platform.rows, platform.cols, E_FALSE);

	// Close the workgroup
	e_close(&dev);
	
	// Release the allocated buffer and finalize the
	// e-platform connection.
	e_free(&emem_1);
	e_finalize();

	printf("Finished.\n");

	return 0;
}

int
write_file(char* the_pth, uint8_t* the_data, long the_sz, int write_once){
	int fd = 0;
	
	if(write_once){
		// old perm 0444
		if((fd = open(the_pth, O_RDWR|O_CREAT|O_EXCL, 0777)) == -1){
			return 0;
		}
	} else {
		if((fd = creat(the_pth, 0777)) == -1){
			return 0;
		}
	}

	write(fd, the_data, the_sz);
	close(fd);

	return 1;
}

uint8_t*
read_file(char* the_pth, off_t* size){
	int fd;
	uint8_t* pt_str = NULL;

	if(size == NULL){
		return pt_str;
	}
	*size = 0;

	fd = open(the_pth, O_RDONLY, 0744);
	if(fd == -1){
		*size = 0;
		return pt_str;
	}


	off_t pos0 = 0;
	off_t pos1 = lseek(fd, 0, SEEK_END);

	*size = pos1;

	if((pos1 != -1) && (pos1 != 0)){
		pos0 = lseek(fd, 0, SEEK_SET);
		if(pos0 != 0){
			close(fd);
			*size = 0;
			return pt_str;
		}

		pt_str = (uint8_t*)malloc(pos1 + 1);
		ssize_t nr = read(fd, pt_str, pos1);
		if(nr != pos1){
			close(fd);
			*size = 0;
			free(pt_str);
			pt_str = NULL;
			return pt_str;
		}
		pt_str[pos1] = 0;
	}

	close(fd);
	return pt_str;
}

bool 
bjh_call_assert(bool vv_ck, const char* file, int line, const char* ck_str, const char* msg){
	
	if(! vv_ck){
		fprintf(stderr, "ASSERT '%s' FAILED\nFILE= %s\nLINE=%d \n", ck_str, file, line);
		//bj_out << get_stack_trace(file, line) << bj_eol;
		if(msg != NULL){
			fprintf(stderr, "MSG=%s\n", msg);
		}
		fprintf(stderr, "------------------------------------------------------------------\n");
	}
	assert(vv_ck);
	return vv_ck;
}

