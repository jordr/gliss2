/* Generated by gep ($(date)) copyright (c) 2008 IRIT - UPS */
#ifndef GLISS_$(PROC)_INCLUDE_$(PROC)_API_H
#define GLISS_$(PROC)_INCLUDE_$(PROC)_API_H

#include <stdint.h>
#include "id.h"


/* $(proc)_state_t type */
typedef struct $(proc)_state_t {
$(foreach registers)$(if !aliased)$(if array)
	$(type) $(name)[$(size)];
$(else)
	$(type) $(name);	
$(end)$(end)$(end)
$(foreach memories)$(if !aliased)
	$(proc)_memory_t *$(NAME);
$(end)$(end)
} $(proc)_state_t;

/* $(proc)_value_t type */
typedef union $(proc)_value_t {
$(foreach values)
	$(type) $(name);
$(end)
} $(proc)_value_t;

/* $(proc)_param_t type */
typedef enum $(proc)_param_t {
	$(PROC)_VOID_T = 0$(foreach params),
	$(PROC)_PARAM_$(NAME)_T$(end)$(foreach registers),
	$(PROC)_$(NAME)_T$(end)
} $(proc)_param_t;

/* $(proc)_ii_t type */
typedef struct $(proc)_ii_t {
	$(proc)_param_t type;
	$(proc)_value_t val;
} $(proc)_ii_t;

/* $(proc)_inst_t type */
typedef struct $(proc)_inst_t {
	$(proc)_ident_t ident;
	$(proc)_ii_t *instrinput;
	$(proc)_ii_t *instroutput;
} $(proc)_inst_t;

/* state management function */
$(proc)_state_t *$(proc)_new_state(void);
$(proc)_state_t *$(proc)_copy_state($(proc)_state_t *state);
void $(proc)_delete_state($(proc)_state_t *state);

/* simple decoding */
typedef struct $(proc)_decoder_t $(proc)_decoder_t;
$(proc)_decoder_t *$(proc)_new_decoder($(proc)_state_t *state);
$(proc)_inst_t *$(proc)_decode($(proc)_decoder_t *decoder, $(proc)_address_t address);
void $(proc)_free_inst($(proc)_inst_t *inst);
void $(proc)_delete_decoder($(proc)_decoder_t *decoder);

/* simulation functions */
typedef struct $(proc)_sim_t $(proc)_sim_t;
$(proc)_sim_t *$(proc)_new_sim($(proc)_state_t *state);
$(proc)_inst_t *$(proc)_next($(proc)_sim_t *sim);
void $(proc)_step($(proc)_sim_t *sim);
void $(proc)_delete_sim($(proc)_sim_t *sim);

#endif /* GLISS_$(PROC)_INCLUDE_$(PROC)_API_H */
