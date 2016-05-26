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

#include <stdlib.h>
#include <Queue.h>

#define QUEUE_HDR(queue) ((Queue_Header_t *) (queue))

void Queue_Init(Queue_t *queue) {
	queue->head = NULL;
	queue->tail = NULL;
}

void Queue_PushFront(Queue_t *queue, void *item) {
	if(queue->head == NULL) {
		// Empty queue, set as tail
		queue->tail = item;
	}

	// Link to head and set as head
	QUEUE_HDR(item)->next = queue->head;
	queue->head = item;
}

void Queue_PushBack(Queue_t *queue, void *item) {
	if(queue->head == NULL) {
		// Empty queue, set as head
		queue->head = item;
	}
	else {
		// Link to old tail
		QUEUE_HDR(queue->tail)->next = item;
	}

	// Make item terminal and set as tail
	QUEUE_HDR(item)->next = NULL;
	queue->tail = item;
}

void *Queue_PopFront(Queue_t *queue) {
	void *front;

	front = queue->head;
	if(front == NULL) {
		// Empty queue
		return NULL;
	}

	// Pop front off head
	queue->head = QUEUE_HDR(front)->next;

	return front;
}

void Queue_Remove(Queue_t *queue, void *prev, void *item) {
	if(prev == NULL) {
		// Remove from front, emptying list
		// if this is the only item in queue
		queue->head = QUEUE_HDR(item)->next;
	}
	else if(QUEUE_HDR(item)->next == NULL) {
		// Remove from back (this isn't the
		// only item in queue)
		QUEUE_HDR(prev)->next = NULL;
		queue->tail = prev;
	}
	else {
		// Middle removal
		QUEUE_HDR(prev)->next = QUEUE_HDR(item)->next;
	}
}
