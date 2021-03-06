
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <e-hal.h>
#include <e-loader.h>

#define f_nm_sz   1024
#define buff_sz   (4096)
#define buff_offset (0x01000000)

int main(int argc, char *argv[])
{
	unsigned row, col, coreid;
	e_platform_t platform;
	e_epiphany_t dev;
	e_mem_t emem;
	char emsg[buff_sz];
	char f_nm[f_nm_sz];
	
	e_coreid_t the_coreid;
	
	e_set_loader_verbosity(H_D0);

	// initialize system, read platform params from
	// default HDF. Then, reset the platform and
	// get the actual system parameters.
	e_init(NULL);
	e_reset_system();
	e_get_platform_info(&platform);

	// Allocate a buffer in shared external memory
	// for message passing from eCore to host.
	e_alloc(&emem, buff_offset, buff_sz);	
	
    	// Open a workgroup
	e_open(&dev, 0, 0, platform.rows, platform.cols);

	// Reset the workgroup
	e_reset_group(&dev);

	// Load the device program onto all the eCores
	e_load_group("e_prog_3.elf", &dev, 0, 0, platform.rows, platform.cols, E_FALSE);

	for (row=0; row<platform.rows; row++){
		for (col=0; col<platform.cols; col++){
			// clear shared buffer.
			memset(emsg, 0, buff_sz);
			e_write(&emem, 0, 0, 0x0000, emsg, buff_sz);

			// Print working core 
			coreid = (row + platform.row) * 64 + col + platform.col;
			printf("eCore 0x%03x (%2d,%2d): \n", coreid, row, col);

			// Start one core
			e_start(&dev, row, col);
			
			// Wait for core program execution to finish.
			usleep(10000);

			// Read message from shared buffer
			the_coreid = 0;
			e_read(&emem, 0, 0, 0x0, &the_coreid, sizeof(the_coreid));
			sprintf(emsg, "Hello World from core 0x%03x! \n", the_coreid);

			// Print the message and close the workgroup.
			printf("%s\n", emsg);	
		}
	}
	
	// Close the workgroup
	e_close(&dev);
	
	// Release the allocated buffer and finalize the
	// e-platform connection.
	e_free(&emem);
	e_finalize();

	return 0;
}

