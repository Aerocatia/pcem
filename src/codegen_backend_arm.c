#ifdef __ARM_EABI__

#include <stdlib.h>
#include "ibm.h"
#include "codegen.h"
#include "codegen_backend.h"
#include "codegen_backend_arm_defs.h"
#include "codegen_backend_arm_ops.h"
#include "codegen_reg.h"
#include "x86.h"

#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#endif
#if defined WIN32 || defined _WIN32 || defined _WIN32
#include <windows.h>
#endif

void *codegen_mem_load_byte;
void *codegen_mem_load_word;
void *codegen_mem_load_long;

void *codegen_mem_store_byte;
void *codegen_mem_store_word;
void *codegen_mem_store_long;

int codegen_host_reg_list[CODEGEN_HOST_REGS] =
{
        REG_R4,
        REG_R5,
        REG_R6,
	REG_R7,
	REG_R8,
	REG_R9,
	REG_R11,
};

int codegen_host_fp_reg_list[CODEGEN_HOST_FP_REGS] =
{
        REG_D8,
        REG_D9,
        REG_D10,
	REG_D11,
	REG_D12,
	REG_D13,
	REG_D14,
	REG_D15
};

static void build_load_routine(codeblock_t *block, int size)
{
        uint32_t *branch_offset;
        uint32_t *misaligned_offset;

        /*In - R0 = address
          Out - R0 = data, R1 = abrt*/
	/*MOV R1, R0, LSR #12
	  MOV R2, #readlookup2
	  LDR R1, [R2, R1, LSL #2]
	  CMP R1, #-1
	  BNE +
	  LDRB R0, [R1, R0]
	  MOV R1, #0
	  MOV PC, LR
	* STR LR, [SP, -4]!
	  BL readmemb386l
	  LDRB R1, cpu_state.abrt
	  LDR PC, [SP], #4
	*/
	host_arm_MOV_REG_LSR(block, REG_R1, REG_R0, 12);
	host_arm_MOV_IMM(block, REG_R2, (uint32_t)readlookup2);
	host_arm_LDR_REG_LSL(block, REG_R1, REG_R2, REG_R1, 2);
	if (size != 1)
	{
		host_arm_TST_IMM(block, REG_R0, size-1);
		misaligned_offset = host_arm_BNE_(block);
	}
	host_arm_CMP_IMM(block, REG_R1, -1);
	branch_offset = host_arm_BEQ_(block);
	if (size == 1)
		host_arm_LDRB_REG(block, REG_R0, REG_R1, REG_R0);
	else if (size == 2)
		host_arm_LDRH_REG(block, REG_R0, REG_R1, REG_R0);
	else if (size == 4)
		host_arm_LDR_REG(block, REG_R0, REG_R1, REG_R0);
	host_arm_MOV_IMM(block, REG_R1, 0);
	host_arm_MOV_REG(block, REG_PC, REG_LR);

        *branch_offset |= ((((uintptr_t)&block->data[block_pos] - (uintptr_t)branch_offset) - 8) & 0x3fffffc) >> 2;
	if (size != 1)
	        *misaligned_offset |= ((((uintptr_t)&block->data[block_pos] - (uintptr_t)misaligned_offset) - 8) & 0x3fffffc) >> 2;
	host_arm_STR_IMM_WB(block, REG_LR, REG_HOST_SP, -4);
	if (size == 1)
		host_arm_BL(block, (uintptr_t)readmemb386l);
	else if (size == 2)
		host_arm_BL(block, (uintptr_t)readmemwl);
	else if (size == 4)
		host_arm_BL(block, (uintptr_t)readmemll);
	else
		fatal("build_load_routine - unknown size %i\n", size);
	host_arm_LDRB_ABS(block, REG_R1, &cpu_state.abrt);
	host_arm_LDR_IMM_POST(block, REG_PC, REG_HOST_SP, 4);

        block_pos = (block_pos + 63) & ~63;
}

static void build_store_routine(codeblock_t *block, int size)
{
        uint32_t *branch_offset;
        uint32_t *misaligned_offset;

        /*In - R0 = address
          Out - R0 = data, R1 = abrt*/
	/*MOV R1, R0, LSR #12
	  MOV R2, #readlookup2
	  LDR R1, [R2, R1, LSL #2]
	  CMP R1, #-1
	  BNE +
	  LDRB R0, [R1, R0]
	  MOV R1, #0
	  MOV PC, LR
	* STR LR, [SP, -4]!
	  BL readmemb386l
	  LDRB R1, cpu_state.abrt
	  LDR PC, [SP], #4
	*/
	host_arm_MOV_REG_LSR(block, REG_R2, REG_R0, 12);
	host_arm_MOV_IMM(block, REG_R3, (uint32_t)writelookup2);
	host_arm_LDR_REG_LSL(block, REG_R2, REG_R3, REG_R2, 2);
	if (size != 1)
	{
		host_arm_TST_IMM(block, REG_R0, size-1);
		misaligned_offset = host_arm_BNE_(block);
	}
	host_arm_CMP_IMM(block, REG_R2, -1);
	branch_offset = host_arm_BEQ_(block);
	if (size == 1)
		host_arm_STRB_REG(block, REG_R1, REG_R2, REG_R0);
	else if (size == 2)
		host_arm_STRH_REG(block, REG_R1, REG_R2, REG_R0);
	else if (size == 4)
		host_arm_STR_REG(block, REG_R1, REG_R2, REG_R0);
	host_arm_MOV_IMM(block, REG_R1, 0);
	host_arm_MOV_REG(block, REG_PC, REG_LR);

        *branch_offset |= ((((uintptr_t)&block->data[block_pos] - (uintptr_t)branch_offset) - 8) & 0x3fffffc) >> 2;
	if (size != 1)
	        *misaligned_offset |= ((((uintptr_t)&block->data[block_pos] - (uintptr_t)misaligned_offset) - 8) & 0x3fffffc) >> 2;
	host_arm_STR_IMM_WB(block, REG_LR, REG_HOST_SP, -4);
	if (size == 1)
		host_arm_BL(block, (uintptr_t)writememb386l);
	else if (size == 2)
		host_arm_BL(block, (uintptr_t)writememwl);
	else if (size == 4)
		host_arm_BL(block, (uintptr_t)writememll);
	else
		fatal("build_store_routine - unknown size %i\n", size);
	host_arm_LDRB_ABS(block, REG_R1, &cpu_state.abrt);
	host_arm_LDR_IMM_POST(block, REG_PC, REG_HOST_SP, 4);

        block_pos = (block_pos + 63) & ~63;
}

static void build_loadstore_routines(codeblock_t *block)
{
        codegen_mem_load_byte = &codeblock[block_current].data[block_pos];
        build_load_routine(block, 1);
        codegen_mem_load_word = &codeblock[block_current].data[block_pos];
        build_load_routine(block, 2);
        codegen_mem_load_long = &codeblock[block_current].data[block_pos];
        build_load_routine(block, 4);

        codegen_mem_store_byte = &codeblock[block_current].data[block_pos];
        build_store_routine(block, 1);
        codegen_mem_store_word = &codeblock[block_current].data[block_pos];
        build_store_routine(block, 2);
        codegen_mem_store_long = &codeblock[block_current].data[block_pos];
        build_store_routine(block, 4);
}

void codegen_backend_init()
{
        int c;
#if defined(__linux__) || defined(__APPLE__)
	void *start;
	size_t len;
	long pagesize = sysconf(_SC_PAGESIZE);
	long pagemask = ~(pagesize - 1);
#endif

#if defined WIN32 || defined _WIN32 || defined _WIN32
        codeblock = VirtualAlloc(NULL, (BLOCK_SIZE+1) * sizeof(codeblock_t), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
        codeblock = malloc((BLOCK_SIZE+1) * sizeof(codeblock_t));
#endif
	if (!codeblock)
		fatal("codeblock failed to alloc - %i\n", (BLOCK_SIZE+1) * sizeof(codeblock_t));
        codeblock_hash = malloc(HASH_SIZE * sizeof(codeblock_t *));

        memset(codeblock, 0, (BLOCK_SIZE+1) * sizeof(codeblock_t));
        memset(codeblock_hash, 0, HASH_SIZE * sizeof(codeblock_t *));

        for (c = 0; c < BLOCK_SIZE; c++)
                codeblock[c].pc = BLOCK_PC_INVALID;

#if defined(__linux__) || defined(__APPLE__)
	start = (void *)((long)codeblock & pagemask);
	len = (((BLOCK_SIZE+1) * sizeof(codeblock_t)) + pagesize) & pagemask;
	if (mprotect(start, len, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
	{
		perror("mprotect");
		exit(-1);
	}
#endif
//        pclog("Codegen is %p\n", (void *)pages[0xfab12 >> 12].block);

        block_current = BLOCK_SIZE;
        block_pos = 0;
        build_loadstore_routines(&codeblock[block_current]);
}

/*R10 - cpu_state*/
void codegen_backend_prologue(codeblock_t *block)
{
        block_pos = 0;

        block_pos = BLOCK_GPF_OFFSET;
	host_arm_MOV_IMM(block, REG_R0, 0);
	host_arm_MOV_IMM(block, REG_R1, 0);
	host_arm_call(block, x86gpf);
	while (block_pos != BLOCK_EXIT_OFFSET)
		host_arm_nop(block);

        block_pos = BLOCK_EXIT_OFFSET; /*Exit code*/
	host_arm_ADD_IMM(block, REG_HOST_SP, REG_HOST_SP, 0x40);
	host_arm_LDMIA_WB(block, REG_HOST_SP, REG_MASK_LOCAL | REG_MASK_PC);
	while (block_pos != BLOCK_START)
		host_arm_nop(block);

	/*Entry code*/

	host_arm_STMDB_WB(block, REG_HOST_SP, REG_MASK_LOCAL | REG_MASK_LR);
	host_arm_SUB_IMM(block, REG_HOST_SP, REG_HOST_SP, 0x40);
	host_arm_MOV_IMM(block, REG_CPUSTATE, (uint32_t)&cpu_state);
        if (block->flags & CODEBLOCK_HAS_FPU)
        {
		host_arm_LDR_IMM(block, REG_TEMP, REG_CPUSTATE, (uintptr_t)&cpu_state.TOP - (uintptr_t)&cpu_state);
		host_arm_SUB_IMM(block, REG_TEMP, REG_TEMP, block->TOP);
		host_arm_STR_IMM(block, REG_TEMP, REG_HOST_SP, IREG_TOP_diff_stack_offset);
        }
}

void codegen_backend_epilogue(codeblock_t *block)
{
	host_arm_ADD_IMM(block, REG_HOST_SP, REG_HOST_SP, 0x40);
	host_arm_LDMIA_WB(block, REG_HOST_SP, REG_MASK_LOCAL | REG_MASK_PC);

        if (block_pos > ARM_LITERAL_POOL_OFFSET)
                fatal("Over limit!\n");

	__clear_cache(&block->data[0], &block->data[block_pos]);
}

#endif