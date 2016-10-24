/*
 * This file is part of eVic SDK.
 *
 * eVic SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eVic SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 ReservedField
 */

#ifndef EVICSDK_QUEUE_H
#define EVICSDK_QUEUE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Queue.
 */
typedef struct {
	/**< First item in queue. NULL when queue is empty. */
	void *head;
	/**< Last item in queue. */
	void *tail;
} Queue_t;

/**
 * Queue item header.
 * This MUST be the first field in every item.
 */
typedef struct {
	/** Next item in queue. */
	void *next;
} Queue_Header_t;

/**
 * Initializes an user-allocated queue.
 *
 * @param queue Queue.
 */
void Queue_Init(Queue_t *queue);

/**
 * Pushes an item to the front of the queue.
 *
 * @param queue Queue.
 * @param item  Item to push.
 */
void Queue_PushFront(Queue_t *queue, void *item);

/**
 * Pushes an item to the back of the queue.
 *
 * @param queue Queue.
 * @param item  Item to push.
 */
void Queue_PushBack(Queue_t *queue, void *item);

/**
 * Pops the first item off the queue.
 *
 * @param queue Queue.
 *
 * @return First item, or NULL if the queue is empty.
 */
void *Queue_PopFront(Queue_t *queue);

/**
 * Removes an item from the queue.
 *
 * @param queue Queue.
 * @param prev  Item preceding the one to be removed,
 *              or NULL if removing the first item.
 * @param item  Item to remove.
 */
void Queue_Remove(Queue_t *queue, void *prev, void *item);

#ifdef __cplusplus
}
#endif

#endif
