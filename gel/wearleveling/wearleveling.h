/******************************************************************************/
/*                                                                            */
/*  HSW snc - Casalecchio di Reno (BO) ITALY                                  */
/*  ----------------------------------------                                  */
/*                                                                            */
/*  Autore: Massimo ZANNA & Maldus (Mattia MALDINI)                           */
/*                                                                            */
/******************************************************************************/

#ifndef WEARLEVELING_H_INCLUDED
#define WEARLEVELING_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "gel_conf.h"

#define MOD_NEXT(x, mod) ((mod + x + 1) % mod)
#define MOD_PREV(x, mod) ((mod + x - 1) % mod)


typedef int (*memory_cb_t)(size_t block_num, uint8_t * buffer);

typedef struct {
    memory_cb_t write_block;
    memory_cb_t read_block;

    size_t blocks_in_page;
    size_t bytes_used_in_block;

} wear_leveled_memory_t;

int search_last(wear_leveled_memory_t *wlm);

int wl_read(wear_leveled_memory_t* wlm, uint8_t* buffer);

int wl_write(wear_leveled_memory_t* wlm, uint8_t* buffer);

void wl_init(wear_leveled_memory_t* wlm,memory_cb_t write_block, memory_cb_t read_block, size_t blocks_in_page, size_t bytes_used_in_block);


#endif