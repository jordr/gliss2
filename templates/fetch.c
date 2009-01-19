/* Generated by gep ($(date)) copyright (c) 2008 IRIT - UPS */

#include <stdlib.h>
#include <stdio.h>
/* #include <math.h>  needed for affiche_valeur_binaire (which is not well coded) */

#include "$(proc)_fetch.h" /* will this file be in the same directory */

/* fetch structure */
struct $(proc)_fetch_t {
	$(proc)_memory_t *mem;
};

/* Extern Modules */
/* Constants */
#ifndef $(PROC)_NO_CACHE_FETCH
//# define $(PROC)_HASH_DEBUG
#	ifndef $(PROC)_HASH_NUM_INSTR
#		define $(PROC)_HASH_NUM_INSTR 0xFFFF
#	endif
#	ifndef $(PROC)_HASH_FUNC
#		define $(PROC)_HASH_FUNC(a) (((a)>>2)&$(PROC)_HASH_NUM_INSTR)
#	endif
#	ifndef $(PROC)_MALLOC_BUF_MAX_SIZE
#		define $(PROC)_MALLOC_BUF_MAX_SIZE (1024*1024)
#	endif
#else /* $(PROC)_NO_CACHE_FETCH */
#	ifndef $(PROC)_MAX_INSTR_FETCHED
#		define $(PROC)_MAX_INSTR_FETCHED 100
#	endif
#endif /* $(PROC)_NO_CACHE_FETCH */

/* Variables & Fonctions (for cache & no cache) */

#ifndef $(PROC)_NO_CACHE_FETCH

typedef struct hash_node_type
{
	$(proc)_ident_t instr_id;
	$(proc)_address_t id;
	struct hash_node_type *next;
} hash_node_t;

hash_node_t *hash_table[$(PROC)_HASH_NUM_INSTR+1];

typedef struct malloc_buf_type
{
	/* keep buffer at the first elt, for having a good alignment */
	char buffer[$(PROC)_MALLOC_BUF_MAX_SIZE];
	size_t size;
	struct malloc_buf_type *next;
} malloc_buf_t;

malloc_buf_t *last_malloc_buf = NULL;

#	ifdef $(PROC)_HASH_DEBUG

unsigned long int count_hits = 0, count_miss = 0, count_instr = 0,
count_hits_total = 0, count_miss_total = 0,
count_move = 0, count_move_total = 0;
unsigned long int move_total = 0, move_intern = 0;

#	endif /* $(PROC)_HASH_DEBUG */

void *mymalloc(size_t size)
{
	void *buf;
	/* align size on 64 bits boundary, that is 8 bytes */
	size = (size +7) & ~(size_t)0x07;
	if ((last_malloc_buf == NULL) || ((last_malloc_buf->size + size) > $(PROC)_MALLOC_BUF_MAX_SIZE))
	{
		malloc_buf_t *malloc_buf = (malloc_buf_t *) malloc(sizeof(malloc_buf_t));
		malloc_buf->size = 0;
		malloc_buf->next = last_malloc_buf;
		last_malloc_buf = malloc_buf;
	}
	buf = &last_malloc_buf->buffer[last_malloc_buf->size];
	last_malloc_buf->size += size;
	return buf;
}

void $(proc)_free_instr($(proc)_inst_t *instr)
{
	/* nop : keep all instr in cache */
}

void cache_halt(void)
{
	malloc_buf_t *ptr;
	ptr = last_malloc_buf;
	while (ptr != NULL)
	{
		ptr = ptr->next;
		free(last_malloc_buf);
		last_malloc_buf = ptr;
	}
	/* here, last_malloc_buf==NULL */
}

#else /* $(PROC)_NO_CACHE_FETCH */

int instr_is_free[$(PROC)_MAX_INSTR_FETCHED];
$(proc)_inst_t instr_tbl[$(PROC)_MAX_INSTR_FETCHED];

void $(proc)_free_instr($(proc)_inst_t *instr)
{
	instr_is_free[(int) ((instr-instr_tbl) / sizeof($(proc)_inst_t *))]=1;
	return;
}

#endif /* $(PROC)_NO_CACHE_FETCH */
 
 
/* data structures */

typedef enum
{
   INSTRUCTION,
   TABLEFETCH
} Type_Decode;

typedef struct
{
	Type_Decode type;
	void *ptr; /* will be used to store an instruction ID or the address of a Table_Decodage */
} Decode_Ent;

typedef struct
{
	uint32_t mask0;
	Decode_Ent *table;
} Table_Decodage;



/*void affiche_valeur_binaire(int valeur)
{
	int bib, bit_fac, i;
	char binaire[255];
	if (valeur == 0)
		return;

	bib = log(valeur) / log(2); // remplacer par une impl�mentation de log2 (a priori facile)
	for (i = 0; i < bib +1; ++i)
	{
		bit_fac = pow(2, bib - i); // remplacer par un d�calage
		binaire[i] = ( valeur / bit_fac > 0? '1' : '0');
		valeur = (valeur / bit_fac > 0 ? valeur - bit_fac : valeur );
	}
	for (i = 0; i < bib + 1; ++i) 
		printf("%c", binaire[i]);
	printf("\n");
}*/

int first_bit_on(int x)
{
	int i = 0, j = 0;
	if (x != 0)
	{
		while (i != 1)
		{
			i = x & 1;
			j++;
			x >>=  1;
		}
	}
	return j;
}


/*
	donne la valeur d'une zone m�moire (une instruction) en ne prenant
	en compte que les bits indiqu�s par le mask

	on fait un ET logique entre l'instruction et le masque,
	on conserve seulement les bits indiqu�s par le masque
	et on les concat�ne pour avoir un nombre sur 32 bits

	on suppose que le masque n'a pas plus de 32 bits � 1,
	sinon d�bordement

	instr : adresse du debut de l'instruction //TODO: donner l'address_t plutot (elimine le besoin de code_t)
	mask  : adresse du debut du mask
	nb_bloc : nombre de bloc de 32 bits sur lesquels on fait l'op�ration
*/
uint32_t valeur_sur_mask_bloc(uint32_t *instr, uint32_t *mask, int nb_bloc)
{
	int i;
	int nb;
	int32_t tmp_mask, j = 0;
	uint32_t res = 0;

	/* on fait un parcours du bit de fort poids de instr[0]
	� celui de poids faible de instr[nb_bloc-1], "de gauche � droite" */
	tmp_mask = *instr; /* $(proc)_mem_read32(ppc_memory_t *, ppc_address_t) */
	/*printf(" decodage de instruction : ");
	affiche_valeur_binaire(tmp_mask);*/
	for (nb = 0; nb < nb_bloc; nb++)
	{
		tmp_mask = mask[nb];
		j += tmp_mask;        
		for (i = (sizeof(instr)*8)-1; i >= 0; i--)
		{
			/* le bit i du mask est 1 ? */
			if (tmp_mask < 0)
			{
				/* si oui, recopie du bit i de l'instruction
				� droite du resultat avec decalage prealable */
				res <<= 1;
				res |= ((instr[nb] >> i) & 0x01);
			}
			tmp_mask <<= 1;
		}
		/*printf(" valeur de tmp_mask %d : ", nb);
		affiche_valeur_binaire(tmp_mask);*/
	}
	/*printf(" valeur du resultat  : %d\n",res);
	affiche_valeur_binaire(res);*/
	return res;
}



/* Fonctions Principales */

$(proc)_ident_t *$(proc)_fetch($(proc)_state_t *state, $(proc)_address_t addr, $(proc)_code_t *code /* param � virer, l'acc�s � l'instr se fera via la m�moire du state_t */)
{
	int valeur;
	Table_Decodage *ptr;
	Table_Decodage *ptr2;
	uint32_t tab_mask[1];

#ifndef $(PROC)_NO_CACHE_FETCH
	unsigned int index;
	hash_node_t *node;
#else
	$(proc)_ident_t instr_id;
#endif /* $(PROC)_NO_CACHE_FETCH */


#ifndef $(PROC)_NO_CACHE_FETCH

#	ifdef $(PROC)_HASH_DEBUG
	if ((count_instr % 10000000) == 0)
	{
		fprintf(stderr,
		"\ncount = %lu\thits = %lu\tmiss = %lu\tmove=%lu\n"
		"\ttotal hits = %lu\ttotal miss = %lu\ttotal move=%lu\n"
		"\tmax move = %lu\n",
		count_instr,count_hits,count_miss,count_move,
		count_hits_total,count_miss_total,count_move_total,
		move_total);
		count_hits = 0;
		count_miss = 0;
		count_move = 0;
		move_total = 0;
	}
#	endif /* $(PROC)_HASH_DEBUG */

	index = $(PROC)_HASH_FUNC(addr);

#	ifdef $(PROC)_HASH_DEBUG
	count_miss++;
	count_instr++;
	count_miss_total++;
#	endif /* $(PROC)_HASH_DEBUG */

	if (hash_table[index] != NULL)
	{
		hash_node_t *prev;
		hash_node_t *act;
		if (hash_table[index]->id == addr)
		{
#	ifdef $(PROC)_HASH_DEBUG
			count_hits++;
			count_hits_total++;
			count_miss--;
			count_miss_total--;
#	endif /* $(PROC)_HASH_DEBUG */
			return hash_table[index]->instr_id;
		}
		prev = hash_table[index];
		act = hash_table[index]->next;
#       ifdef $(PROC)_HASH_DEBUG
		move_intern = 0;
#       endif /* $(PROC)_HASH_DEBUG */
		for ( ; act != NULL; )
		{
#	ifdef $(PROC)_HASH_DEBUG
			move_intern++;
#	endif /* $(PROC)_HASH_DEBUG */
			if (act->id == addr)
			{
				hash_node_t *next = act->next;
				prev->next = next;
				act->next = hash_table[index];
				hash_table[index] = act;
#	ifdef $(PROC)_HASH_DEBUG
				count_move++;
				count_move_total++;
				count_miss--;
				count_miss_total--;
				if (move_intern > move_total)
					move_total = move_intern;
#	endif /* $(PROC)_HASH_DEBUG */
				return act->instr_id;
			}
#	ifdef $(PROC)_HASH_DEBUG
			if(move_intern > move_total)
				move_total = move_intern;
#	endif /* $(PROC)_HASH_DEBUG */
			prev = act;
			act = act->next;
		}
	}
	node = (hash_node_t *)mymalloc(sizeof(hash_node_t));
	node->next = hash_table[index];
	node->id = addr;
	/* node->instr_id = smtg; is done further below */

#else /* $(PROC)_NO_CACHE_FETCH */

#endif /* $(PROC)_NO_CACHE_FETCH */
	ptr2 = table;

	do
	{
		int j1 = 0, k = 0;
		tab_mask[0] = ptr2->mask0;
		j1 = tab_mask[0];
		if (k == 0)
			k = first_bit_on(j1);
		/*valeur = valeur_sur_mask_code(code, tab_mask, 1,k);*/
		valeur = valeur_sur_mask_bloc(code, tab_mask, 1);
		ptr = ptr2;
		ptr2 = ptr->table[valeur].ptr;
	}
	while (ptr->table[valeur].type == TABLEFETCH);

#ifndef $(PROC)_NO_CACHE_FETCH
	node->instr_id = ($(proc)_ident_t)ptr->table[valeur].ptr;
	hash_table[index] = node;
	return node->instr_id;
#else /* $(PROC)_NO_CACHE_FETCH */
	return ($(proc)_ident_t)ptr->table[valeur].ptr;
#endif /* $(PROC)_NO_CACHE_FETCH */
}


/* And now the tables */

$(INIT_FETCH_TABLES)


void $(proc)_halt_fetch(void)
{
#ifndef $(PROC)_NO_CACHE_FETCH
	cache_halt();
#else /* $(PROC)_NO_CACHE_FETCH */
	/* nop */
#endif /* $(PROC)_NO_CACHE_FETCH */
}

void $(proc)_init_fetch(void)
{
	int i;
#ifndef $(PROC)_NO_CACHE_FETCH
	for (i = 0; i < $(PROC)_HASH_NUM_INSTR + 1; i++)
		hash_table[i] = NULL;
#else /* $(PROC)_NO_CACHE_FETCH */
	for (i = 0; i < $(PROC)_MAX_INSTR_FETCHED; i++)
		instr_is_free[i] = 1;
#endif /* $(PROC)_NO_CACHE_FETCH */
}


/* End of file $(proc)_fetch.c */
