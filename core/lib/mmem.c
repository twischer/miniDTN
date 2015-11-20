/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \addtogroup mmem
 * @{
 */

/**
 * \file
 *         Implementation of the managed memory allocator
 * \author
 *         Adam Dunkels <adam@sics.se>
 * 
 */

#include "mmem.h"

#include <stdbool.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "list.h"
#include "contiki-conf.h"
#include "lib/logging.h"
#include "debugging.h"


#ifdef MMEM_CONF_SIZE
#define MMEM_SIZE MMEM_CONF_SIZE
#else
#define MMEM_SIZE 0x8000
#endif

#ifdef MMEM_CONF_ALIGNMENT
#define MMEM_ALIGNMENT MMEM_CONF_ALIGNMENT
#else
#define MMEM_ALIGNMENT 4
#endif

static SemaphoreHandle_t mutex = NULL;
LIST(mmemlist);
static size_t avail_memory;
static char memory[MMEM_SIZE];


size_t mmem_avail_memory(void)
{
	/* only allow reading of this variable
	 * Otherwise locking is needed
	 */
	return avail_memory;
}


/*---------------------------------------------------------------------------*/
/**
 * \brief      Allocate a managed memory block
 * \param m    A pointer to a struct mmem.
 * \param size The size of the requested memory block
 * \return     Non-zero if the memory could be allocated, zero if memory
 *             was not available.
 * \author     Adam Dunkels
 *
 *             This function allocates a chunk of managed memory. The
 *             memory allocated with this function must be deallocated
 *             using the mmem_free() function.
 *
 *             \note This function does NOT return a pointer to the
 *             allocated memory, but a pointer to a structure that
 *             contains information about the managed memory. The
 *             macro MMEM_PTR() is used to get a pointer to the
 *             allocated memory.
 *
 */
int
mmem_alloc(struct mmem *m, unsigned int size)
{
	/* enter the critical section */
	if ( !xSemaphoreTake(mutex, portMAX_DELAY) ) {
		return -1;
	}

	LOG(LOGD_CORE, LOG_MMEM, LOGL_DBG, "%p %lu %lu", m, size, avail_memory);

	/* Check if we have enough memory left for this allocation. */
	if(avail_memory < size) {
		xSemaphoreGive(mutex);
		return 0;
	}

	/* fail, if the memory was already allocated */
	for (struct mmem* n=list_head(mmemlist); n != NULL; n=list_item_next(n)) {
		configASSERT(n != m);
	}

	/* We had enough memory so we add this memory block to the end of
	 the list of allocated memory blocks. */
	list_add(mmemlist, m);

	/* Set up the pointer so that it points to the first available byte
	 in the memory block. */
	m->ptr = &memory[MMEM_SIZE - avail_memory];

	/* Remember the size of this memory block. */
	m->size = size;
	m->real_size = size;

	while( m->real_size % MMEM_ALIGNMENT != 0 ) {
		m->real_size ++;
	}

	/* Decrease the amount of available memory. */
	avail_memory -= m->real_size;

	LOG(LOGD_CORE, LOG_MMEM, LOGL_DBG, "%p %p %lu %p %lu", m, m->ptr, m->real_size, m->next, avail_memory);

	xSemaphoreGive(mutex);

	/* Return non-zero to indicate that we were able to allocate
	 memory. */
	return 1;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Deallocate a managed memory block
 * \param m    A pointer to the managed memory block
 * \author     Adam Dunkels
 *
 *             This function deallocates a managed memory block that
 *             previously has been allocated with mmem_alloc().
 *
 */
int
mmem_free(struct mmem *m)
{
	/* enter the critical section */
	if ( !xSemaphoreTake(mutex, portMAX_DELAY) ) {
		return -1;
	}

	LOG(LOGD_CORE, LOG_MMEM, LOGL_DBG, "%p %p %lu %p %lu", m, m->ptr, m->real_size, m->next, avail_memory);

#if (configASSERT_DEFINED == 1)
	/* fail if the memory is not in this list */
	bool is_in_list = false;
	for (struct mmem* n=list_head(mmemlist); n != NULL; n=list_item_next(n)) {
		if (n == m) {
			is_in_list = true;
			break;
		}
	}
	configASSERT(is_in_list);
#endif /* configASSERT_DEFINED */

	struct mmem *n;

	if(m->next != NULL) {
		/* if the real_size is not set correctly,
		 * the pointer movment will fail.
		 * A reason for an incorecct real_size
		 * could be an stack overflow
		 * which overwrites the real_size value.
		 */
		configASSERT( (m->next->ptr - m->ptr) == m->real_size );

		/* Compact the memory after the allocation that is to be removed
		 * by moving it downwards.
		 */
		memmove(m->ptr, m->next->ptr,
				&memory[MMEM_SIZE - avail_memory] - (char *)m->next->ptr);

		/* Update all the memory pointers that points to memory that is
	   after the allocation that is to be removed. */
		for(n = m->next; n != NULL; n = n->next) {
			n->ptr = (void *)((char *)n->ptr - m->real_size);
		}
	}

	avail_memory += m->real_size;

	/* Remove the memory block from the list. */
	list_remove(mmemlist, m);

	LOG(LOGD_CORE, LOG_MMEM, LOGL_DBG, "%lu", avail_memory);

	xSemaphoreGive(mutex);
	return 0;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Change the size of allocated memory
 * \param mem  mmem chunk whose size should be changed
 * \param size Size to change the chunk to
 * \return     1 on success, 0 on failure
 * \author     Daniel Willmann
 *
 *             This function is the mmem equivalent of realloc(). If the size
 *             could not be changed the original chunk is preserved.
 */
int
mmem_realloc(struct mmem *mem, unsigned int size)
{
	int mysize = size;

	while( mysize % MMEM_ALIGNMENT != 0 ) {
		mysize ++;
	}

	/* enter the critical section */
	if ( !xSemaphoreTake(mutex, portMAX_DELAY) ) {
		return 0;
	}

	LOG(LOGD_CORE, LOG_MMEM, LOGL_DBG, "%p %p %lu %p %lu %lu", mem, mem->ptr, mem->real_size, mem->next, size, avail_memory);

	int diff = (int)mysize - mem->real_size;

	/* Already the correct size */
	if (diff == 0) {
		xSemaphoreGive(mutex);
		return 1;
	}

	/* Check if request is too big */
	if (diff > 0 && diff > avail_memory) {
		xSemaphoreGive(mutex);
		return 0;
	}

	/* We need to do the same thing as in mmem_free */
	struct mmem *n;
	if (mem->next != NULL) {
		memmove((char *)mem->next->ptr+diff, (char *)mem->next->ptr,
				&memory[MMEM_SIZE - avail_memory] - (char *)mem->next->ptr);

		/* Update all the memory pointers that points to memory that is
	after the allocation that is to be moved. */
		for(n = mem->next; n != NULL; n = n->next) {
			n->ptr = (void *)((char *)n->ptr + diff);
		}
	}

	mem->size = size;
	mem->real_size = mysize;
	avail_memory -= diff;

	LOG(LOGD_CORE, LOG_MMEM, LOGL_DBG, "%p %p %lu %p %lu %lu", mem, mem->ptr, mem->real_size, mem->next, avail_memory);

	xSemaphoreGive(mutex);
	return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Initialize the managed memory module
 * \author     Adam Dunkels
 *
 *             This function initializes the managed memory module and
 *             should be called before any other function from the
 *             module.
 *
 */
int
mmem_init(void)
{
	/* Only execute the initalisation before the scheduler was started.
	 * So there exists only one thread and
	 * no locking is needed for the initialisation
	 */
	configASSERT(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED);

	/* cancle, if initialisation was already done */
	if(mutex != NULL) {
		return -1;
	}
	/* Do not use an recursive mutex,
	 * because mmem_check() will not work vital then.
	 */
	mutex = xSemaphoreCreateMutex();
	if(mutex == NULL) {
		return -2;
	}

	list_init(mmemlist);
	avail_memory = MMEM_SIZE;

	return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * @brief mmem_check checks the memory system for consistency
 */
void
mmem_check(void)
{
	if (mutex == NULL) {
		/* initalization not done,
		 * so data could be wrong
		 */
		return;
	}

	/* In ISR the mutex state could not be checked,
	 * therefore cancle when called from ISR.
	 */
	const uint16_t irq_nr = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);
	if (irq_nr > 0) {
		return;
	}

	/* mutex check is only needed, if the scheduler is running */
	if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
		/* do not check, if any function is in the critical region */
		if (xSemaphoreGetMutexHolder(mutex) != NULL) {
			return;
		}
	}


	if (avail_memory > MMEM_SIZE) {
		/* only last two entries needed.
		 * On first check was successfully and on
		 * second entry check fails
		 */
		print_stack_trace_part(2);
	}

	struct mmem* m = list_head(mmemlist);
	if (m != NULL) {
		/* only check the list, if there is an element */
		for (; m->next != NULL; m = m->next) {
			if (m->real_size < m->size) {
				/* only last two entries needed.
				 * On first check was successfully and on
				 * second entry check fails
				 */
				print_stack_trace_part(2);
			}

			const size_t offset = m->next->ptr - m->ptr;
			if (offset != m->real_size) {
				/* only last two entries needed.
				 * On first check was successfully and on
				 * second entry check fails
				 */
				print_stack_trace_part(2);
			}

		}

		/* last element uses different checks */
		if (m->real_size < m->size) {
			/* only last two entries needed.
			 * On first check was successfully and on
			 * second entry check fails
			 */
			print_stack_trace_part(2);
		}

		const size_t offset = &memory[MMEM_SIZE - avail_memory] - (char*)m->ptr;
		if (offset != m->real_size) {
			/* only last two entries needed.
			 * On first check was successfully and on
			 * second entry check fails
			 */
			print_stack_trace_part(2);
		}
	}
}

/** @} */
