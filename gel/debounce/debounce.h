/******************************************************************************/
/*                                                                            */
/*  HSW snc - Casalecchio di Reno (BO) ITALY                                  */
/*  ----------------------------------------                                  */
/*                                                                            */
/*  Autore: Massimo ZANNA & Maldus (Mattia MALDINI)                           */
/*                                                                            */
/******************************************************************************/

#ifndef DEBOUNCE_H_INCLUDED
#define DEBOUNCE_H_INCLUDED

#define NUM_INPUTS ((int)sizeof(unsigned int) * 8)
#define NTH(x, i)  ((x >> i) & 0x1)

typedef struct {
    unsigned int old_input;
    unsigned int filters[NUM_INPUTS];
    unsigned int value;
} debounce_filter_t;

/*
 *  Returns the current value of the i-th input
 *
 *  i : input to return
 *  filter: pointer to the debounce_filter_t struct
 *  return: 0 or 1
 */
static inline int debounce_read(int i, debounce_filter_t *filter) {
    if (i < 0 || i >= NUM_INPUTS)
        return -1;
    return NTH(filter->value, i);
}

/*
 *  Initializes a debounce filter struct. The struct holds the necessary data to
 * filter fluctuations in the input. The input is received in the form of a bitmap as
 * bit as an unsigned int (typically a word in the target system). The struct fields
 * are not encapsulated to avoid using dynamic memory.
 *
 *  filter: pointer to the debounce_filter_t struct
 */
void debounce_filter_init(debounce_filter_t *filter);

/*
 *  Filters fluctuations in the received input. The value held into the structure is modified
 * only after a change that lasted through at least debounce+1 calls.
 * The debounce effect is calculated on number of calls instead of time because the
 * focus of this tool is on stability, not on duration. Therefore, this function should
 * be called at more-or-less regular intervals.
 *
 *  filter: pointer to the debounce_filter_t struct
 *  input: bitmap containing the current input
 *  debounce: number of consecutive calls where the input must be different for its
 * value to change.
 *  return: returns 1 if there was a change
 */
int debounce_filter(debounce_filter_t *filter, unsigned int input, int debounce);

/*
 *  Sets the saved value for all inputs. Useful to reset the reading.
 *
 *  filter: pointer to the debounce_filter_t struct
 *  set: input values to reset to
 */
void debounce_filter_set(debounce_filter_t *filter, unsigned int set);

#endif
