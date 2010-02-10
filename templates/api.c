/* Generated by gep ($(date)) copyright (c) 2008 IRIT - UPS */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../include/$(proc)/api.h"
#include "platform.h"
#include "loader.h"



static char *$(proc)_string_ident[] = {
	"$(PROC)_UNKNOWN"$(foreach instructions),
	"$(PROC)_$(IDENT)"$(end)};


char *$(proc)_get_string_ident($(proc)_ident_t id)
{
	return $(proc)_string_ident[id];
}


/**
 * @typedef $(proc)_platform_t
 * This opaque type allows to represent an hardware and software platform.
 * It includes information about:
 * @li the hardware memories,
 * @li the module data (including system calls and interruption support).
 */
/* typedef struct $(proc)_platform_t $(proc)_platform_t;*/


/**
 * Build a new platform for the platform $(proc).
 * @return	Created platform or null if there is no more memory (see errno).
 * @note To release the platform, use $(proc)_unlock_platform().
 */
$(proc)_platform_t *$(proc)_new_platform(void) {
	$(proc)_platform_t *pf;

	/* allocation */
	pf = ($(proc)_platform_t *)calloc(1, sizeof($(proc)_platform_t));
	if(pf == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	/* the new platform is not locked yet */
	pf->usage = 0;

	/* other init */
	/*pf->entry = pf->sp = pf->argv = pf->envp = pf->aux = pf->argc = 0;*/
	pf->sys_env = calloc(1, sizeof($(proc)_env_t));
	if(pf->sys_env == NULL) {
		errno = ENOMEM;
		free(pf);
		return NULL;
	}

	/* memory initialization */
$(foreach memories)
	pf->mems.named.$(name) = $(proc)_mem_new();
	if(pf->mems.named.$(name) == NULL) {
		free(pf->sys_env);
		free(pf);
		return NULL;
	}
$(end)

	/* module initialization */
$(foreach modules)
	$(PROC)_$(NAME)_INIT(pf);
$(end)

	/* return platform */
	return pf;
}


/**
 * Get a memory in the platform.
 * @param platform	Platform to get memory from.
 * @param index		Index of the memory to get.
 * @return			Requested memory.
 */
$(proc)_memory_t *$(proc)_get_memory($(proc)_platform_t *platform, int index) {
	if (platform == NULL)
		return NULL;

	return platform->mems.array[index];
}


/**
 * return the system info (argc, argv ...), meaningless
 * if used when no program loaded
 * @param	platform	Platform to get system info from.
 * @return		Requested system info.
 */
$(proc)_env_t *$(proc)_get_sys_env($(proc)_platform_t *platform)
{
	if (platform == NULL)
		return NULL;

	return platform->sys_env;
}


/**
 * Ensure that the platform is not released before a matching
 * $(proc)_platform_unlock() is performed.
 * @param platform	Platform to lock.
 */
void $(proc)_lock_platform($(proc)_platform_t *platform) {
	if (platform == NULL)
		return;

	platform->usage++;
}


/**
 * Unlock the platform. If it the last lock on the platform,
 * the platform is freed.
 * @param platform	Platform to unlock.
 */
void $(proc)_unlock_platform($(proc)_platform_t *platform) {
	if (platform == NULL)
		return;

	/* unlock */
	if(--platform->usage != 0)
		return;

	/* destroy the modules */
$(foreach modules)
	$(PROC)_$(NAME)_DESTROY(platform);
$(end)

	/* free the memories */
$(foreach memories)
	$(proc)_mem_delete(platform->mems.named.$(name));
$(end)

	/* free system info */
	free(platform->sys_env);

	/* free the platform */
	free(platform);
	platform = NULL;
}


/**
 * Load the given program in the platform and initialize stack
 * @param platform	Platform to load in.
 * @param path		Path of the file to load.
 * @return			0 for success, -1 for error (in errno).
 */
int $(proc)_load_platform($(proc)_platform_t *platform, const char *path) {
	$(proc)_loader_t *loader;
	if (platform == NULL)
		return -1;

	/* open the file */
	loader = $(proc)_loader_open(path);
	if(loader == NULL)
		return -1;

	/* load in platform's memory */
	$(proc)_loader_load(loader, platform);

	/* initialize system information */
	platform->entry = $(proc)_loader_start(loader);

	/* !!TODO!! add argc,argv... init */
	/* stack initialization */
	$(proc)_stack_fill_env(loader, platform, platform->sys_env);

	/* close the file */
	$(proc)_loader_close(loader);

	/* return success */
	return 0;
}



/* state management function */


/**
 * Create a new initialized state depending on a given platform
 * @param	platform	the platform helping the simulation on the created state
 * @return			a new initialized state suitable for the given platform
 */
$(proc)_state_t *$(proc)_new_state($(proc)_platform_t *platform)
{
	$(proc)_state_t *state;

	/* allocation */
	state = ($(proc)_state_t *)malloc(sizeof($(proc)_state_t));
	if(state == NULL)
	{
		errno = ENOMEM;
		return NULL;
	}

	/* zeroing of all registers and memory references */
	memset(state, 0, sizeof($(proc)_state_t));

	/* creating platform */
	if(platform == NULL)
	{
		free(state);
		return NULL;
	}
	state->platform = platform;

	/* locking it */
	$(proc)_lock_platform(state->platform);

	//!!DEBUG!!
	//printf("new_state, before init code, NIA=%08X\n", state->NIA);

	/* memory initialization */
$(foreach memories)
	state->$(NAME) = state->platform->mems.named.$(name);
$(end)
	/* proper state initialization (from the op init) */
$(gen_init_code)

	/* Pcs initialization */
	state->$(NPC_NAME) = platform->entry;

	/* system registers initialization (argv, envp...) */
	$(proc)_registers_fill_env(platform->sys_env, state);

	return state;
}


/**
 * Delete a state
 * @param	state	the state to delete
 */
void $(proc)_delete_state($(proc)_state_t *state)
{
	if (state == NULL)
		return;

	/* unlock the platform */
	$(proc)_unlock_platform(state->platform);

	/* free the state */
	free(state);
	state = NULL;
}


/**
 * Return a copy of a given state, both state will share the same memory
 * and each new state's register will have the same value as in the given state.
 * the copied state may not be suitable for further simulation as the stack pointers
 * and similar address registers will be identical in two different states
 * sharing the same memory
 *
 * @param	state	the state to copy
 * @return		a fresh allocated copy (to be freed by the caller)
 */
$(proc)_state_t *$(proc)_copy_state($(proc)_state_t *state)
{
	int i;

	if (state == NULL)
		return NULL;

	/* allocate a new state */
	$(proc)_state_t *new_state = ($(proc)_state_t *)malloc(sizeof($(proc)_state_t));
	if(new_state == NULL)
	{
		errno = ENOMEM;
		return NULL;
	}

	/* copy the platform and lock it */
	/* !!WARNING!! is this needed? as we may not wish to simulate with this state */
	new_state->platform = state->platform;
	$(proc)_lock_platform(new_state->platform);

	/* copy all the registers */
$(foreach registers)$(if !aliased)$(if array)
	for (i = 0; i < $(size); i++)
		new_state->$(name)[i] = state->$(name)[i];
$(else)
	new_state->$(name) = state->$(name);
$(end)$(end)$(end)
	/* copy the references to the memory */
$(foreach memories)$(if !aliased)
	new_state->$(NAME) = state->$(NAME);
$(end)$(end)

	return new_state;
}


/**
 * Return a copy of a given state which will be intended for a further simulation,
 * both state will share the same memory
 * and each new state's register will have the same value as in the given state.
 *
 * TODO: we want to be able to simulate on this forked state
 * so we should really care about stack pointer and other address registers
 * used to write data in memory, these should have a different value
 * for each state created as we share the same memory
 *
 * @param	state	the state to fork
 * @return		a fresh allocated copy (to be freed by the caller)
 */
$(proc)_state_t *$(proc)_fork_state($(proc)_state_t *state)
{
	int i;

	if (state == NULL)
		return NULL;

	/* allocate a new state */
	$(proc)_state_t *new_state = ($(proc)_state_t *)malloc(sizeof($(proc)_state_t));
	if(new_state == NULL)
	{
		errno = ENOMEM;
		return NULL;
	}

	/* copy the platform and lock it */
	new_state->platform = state->platform;
	$(proc)_lock_platform(new_state->platform);

	/* copy all the registers */
$(foreach registers)$(if !aliased)$(if array)
	for (i = 0; i < $(size); i++)
		new_state->$(name)[i] = state->$(name)[i];
$(else)
	new_state->$(name) = state->$(name);
$(end)$(end)$(end)
	/* copy the references to the memory */
$(foreach memories)$(if !aliased)
	new_state->$(NAME) = state->$(NAME);
$(end)$(end)
}


/**
 * Dump all register values of a given state to the given output
 *
 * @param	state	the state whose registers we wish to dump
 * @param	out	the file to dump within, typically stderr or stdout
 */
void $(proc)_dump_state($(proc)_state_t *state, FILE *out)
{
	int i;

	/* dump all the registers */
$(foreach registers)$(if !aliased)$(if array)
	fprintf(out, "$(name)\n");
	for (i = 0; i < $(size); i++)
		fprintf(out, "\t[%d] = $(printf_format)\n", i, state->$(name)[i]);
$(else)
	fprintf(out, "$(name) = $(printf_format)\n", state->$(name));
$(end)$(end)$(end)


//!!DEBUG!! for PPC
// fprintf(out, "NIA = %08X, CIA = %08X, PIA = %08X\n", state->NIA, state->CIA, state->PIA);
// fprintf(out, "LR = %08X, CTR = %08X, L2CR = %08X, L2PM = %08X\n", state->LR, state->CTR, state->L2CR, state->L2PM);
// fprintf(out, "XER = %08X, MSR = %08X\n", state->XER, state->MSR);
// fprintf(out, "GPR\n");
// for (i=0; i<8; i++)
// 	fprintf(out, "\t[%2d]=%08X\t[%2d]=%08X\t[%2d]=%08X\t[%2d]=%08X\n", i*4, state->GPR[i*4], i*4+1, state->GPR[i*4+1], i*4+2, state->GPR[i*4+2], i*4+3, state->GPR[i*4+3]);
// fprintf(out, "CR\n");
// for (i=0; i<2; i++)
// 	fprintf(out, "\t[%2d]=%08X\t[%2d]=%08X\t[%2d]=%08X\t[%2d]=%08X\n", i*4, state->CR[i*4], i*4+1, state->CR[i*4+1], i*4+2, state->CR[i*4+2], i*4+3, state->CR[i*4+3]);
// fprintf(out, "SR\n");
// for (i=0; i<2; i++)
// 	fprintf(out, "\t[%2d]=%08X\t[%2d]=%08X\t[%2d]=%08X\t[%2d]=%08X\n", i*4, state->SR[i*4], i*4+1, state->SR[i*4+1], i*4+2, state->SR[i*4+2], i*4+3, state->SR[i*4+3]);
// fprintf(out, "\n\n");

}


/**
 * return a reference (a pointer in fact) towards the platform of a given state
 *
 * @param	state	the state we want to access the platform
 * @return		the address of the platform of the given state
 */
$(proc)_platform_t *$(proc)_platform($(proc)_state_t *state)
{
	if (state == NULL)
		return NULL;

	/* return the platform */
	return state->platform;
}


/* simulation functions */


/**
 * Create a new simulator structure with the given state
 * @param	state	the state on which we intend to simulate
 * @param	start_addr	the beginning of the execution (useful for executables compiled with no _start symbol),
 *				if null we leave the NPC in its previous state given by the loader
 * @param	exit_addr	the explicitly given last instruction address to simulate, if null we will stop running in another way
 */
$(proc)_sim_t *$(proc)_new_sim($(proc)_state_t *state, $(proc)_address_t start_addr, $(proc)_address_t exit_addr)
{
	$(proc)_sim_t *sim;

	if (state == NULL)
		return NULL;

	/* allocate a new simulator */
	sim = ($(proc)_sim_t *)malloc(sizeof($(proc)_sim_t));
	if(sim == NULL)
	{
		errno = ENOMEM;
		return NULL;
	}

	/* link the state to the new simulator */
	sim->state = state;

	/* create a new decoder */
	sim->decoder = $(proc)_new_decoder($(proc)_platform(state));
	if (sim->decoder == NULL)
		return NULL;

	if (exit_addr)
		sim->addr_exit = exit_addr;
	if (start_addr)
		sim->state->$(NPC_NAME) = start_addr;


	return sim;
}


/**
 * Return the next instruction to be executed by the given simulator
 *
 * !!WARNING!! we assume the address is given by the NPC,
 * we must do otherwise if there is no NPC declared
 *
 * @param	sim	the simulator which we simulate within
 * @return		the next instruction to be executed, fully decoded
 */
$(proc)_inst_t *$(proc)_next($(proc)_sim_t *sim)
{
	/*$(proc)_state_t *state;*/
	$(proc)_address_t addr_next_inst;

	if (sim == NULL)
		return NULL;

	/* retrieving address of the next instruction */
	/* the macros allowing register access refer always to a state called "state" */
	/*state = sim->state;*/
	// !!DEBUG!!
	addr_next_inst = sim->state->$(NPC_NAME);

	/* retrieving the instruction (which is allocated by the decoder) */
	/* we let the caller check for error */
	return $(proc)_decode(sim->decoder, addr_next_inst);
}


/**
 * Execute the next instruction in the given simulator.
 * It doesn't check if we reached the last instruction, so it should be done
 * separately using the function $(proc)_is_sim_ended
 * @param	sim	the simulator which we simulate within
 */
void $(proc)_step($(proc)_sim_t *sim)
{
	$(proc)_inst_t *inst;

	if (sim == NULL)
	/* may be print an error? */
		return;

	/* retrieving next instruction */
	inst = $(proc)_next(sim);

			// !!BEGIN DEBUG!! for PPC
			/*char buff[100];
			$(proc)_disasm(buff, inst);
			uint32_t code = $(proc)_mem_read32($(proc)_get_memory(sim->state->platform, 0), sim->state->NIA);
			printf("@%08X:\t%08X\t%s\n", sim->state->NIA, code, buff);
			fflush(stdout);*/
			// !!END DEBUG!!

	/* execute it */
	$(proc)_execute(sim->state, inst);

	/* finally free it */
	$(proc)_free_inst(inst);
}


/**
 * Indicate if the simulation is finished on the given simulator
 * @param	sim	the simulator which we simulate within
 * @return	whether or not the simulation is ended (0 => false, anything!=0 => true)
 */
int $(proc)_is_sim_ended($(proc)_sim_t *sim)
{
	/* we want to stop right before the exit address */
	return (sim->addr_exit == sim->state->$(NPC_NAME));
}


/**
 * destruction of a simulator object, the associated state and decoder are also destroyed
 * @param	sim	the simulator to be deleted
 */
void $(proc)_delete_sim($(proc)_sim_t *sim)
{
	if (sim == NULL)
		return;

	/* delete the decoder */
	$(proc)_delete_decoder(sim->decoder);

	/* delete the state */
	$(proc)_delete_state(sim->state);

	/* delete the sim */
	free(sim);
}
