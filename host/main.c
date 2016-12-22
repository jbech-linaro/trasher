#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* OP-TEE TEE client API */
#include <tee_client_api.h>

/* To the the UUID */
#include "../ta/include/trasher_ta.h"

/*
 * This is the physical address of the memory that we're using. Manually found
 * out that this is almost always the addressed used in param[0]. One way to
 * find this address is to set a breakpoint in entry_invoke_command(..) and see
 * what value "params" have in the in.params structure.
 */
#define READ_ADDR 0x7fe01000

static void read_memory(uint32_t addr)
{
	int   devmem;
	off_t offset = 0;
	off_t page_addr;
	const struct timespec ts = {0, 10};

	unsigned long *hw_addr = (unsigned long *)addr;
	unsigned long *mapped_addr;

	devmem = open("/dev/mem", O_RDWR | O_SYNC);
	offset = (off_t)hw_addr % getpagesize();
	page_addr = (off_t)(hw_addr - offset);
	printf("NW: offset: 0x%08lx, page_addr: 0x%08lx\n", offset, page_addr);

	printf("NW: Try to map and read the data from: 0x%08lx\n", page_addr);
	sleep(2);
	hw_addr = (unsigned long *)mmap(0, 4, PROT_READ|PROT_WRITE, MAP_SHARED, devmem, page_addr);

	mapped_addr = hw_addr + offset;

	printf("2. NW: Waiting for secure side / TA to update the buffer "
	       "without returning back to NW\n");

	while (1) {
		nanosleep(&ts, NULL);
		if (*mapped_addr == SW_UPDATE_VALUE) {
			printf("3. NW: Found updated TEE shared memory: 0x%08lX\n",
			       *mapped_addr);

			printf("4. NW: Sleep 3 seconds ...\n");
			sleep(3);

			printf("5. NW: Write new value to shared memory: 0x%08X (TA finds this and return back to NW)\n",
			       FINAL_VALUE);
			*mapped_addr = FINAL_VALUE;
			return;
		}
	}
}

static void *read_thread(void *arg)
{
	pthread_t id = pthread_self();
	printf("NW: Thread with 0x%08lx created\n", id);
	read_memory(READ_ADDR);

	return NULL;
}

static void call_ta(void)
{
        TEEC_Result res;
        TEEC_Context ctx;
        TEEC_Session sess;
        TEEC_Operation op;
        TEEC_UUID uuid = TRASHER_TA_UUID;
        uint32_t err_origin;
	uint32_t buf = NW_INITIAL_VALUE;

        res = TEEC_InitializeContext(NULL, &ctx);
        if (res != TEEC_SUCCESS)
                errx(1, "NW: TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL,
			       NULL, &err_origin);
        if (res != TEEC_SUCCESS)
                errx(1, "NW: TEEC_Opensession failed with code 0x%x origin 0x%x",
                        res, err_origin);

        memset(&op, 0, sizeof(op));
        op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
                                         TEEC_NONE,
                                         TEEC_NONE,
                                         TEEC_NONE);

        op.params[0].tmpref.buffer = (void *)&buf;
        op.params[0].tmpref.size = sizeof(uint32_t);

	printf("1. NW: Invoke the TA with initial buf value: 0x%08X\n", buf);

	res = TEEC_InvokeCommand(&sess, TRASH, &op, &err_origin);
        if (res != TEEC_SUCCESS)
                errx(1, "NW: TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
                        res, err_origin);

        /* We're done with the TA, close the session ... */
        TEEC_CloseSession(&sess);

        /* ... and destroy the context. */
        TEEC_FinalizeContext(&ctx);

        return;
}  

int main() {
	pthread_t pt;
	int err;
	err = pthread_create(&pt, NULL, &read_thread, NULL);
	if (err)
		printf("NW: can't create thread :[%s]\n", (char *)strerror(err));

	call_ta();

	return 0;
}
