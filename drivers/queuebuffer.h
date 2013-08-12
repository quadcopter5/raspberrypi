/**
	Philip Romano
	QueueBuffer data structure

	Stores data byte-per-byte in a FIFO structure.
*/

#ifndef QUEUEBUFFER_H
#define QUEUEBUFFER_H

#define QB_BUFSIZE 4096
struct QueueBuffer {
	struct QueueBufferNode {
		char buffer[QB_BUFSIZE];
		char *front, *back;
		struct QueueBufferNode *next;
	} *head, *tail;
};

/**
	Initialize a QueueBuffer.

	Provide an uninitialized struct QueueBuffer pointer, and this function
	will initialize it to point to a valid QueueBuffer.
*/
void qb_initialize(struct QueueBuffer **qbuf);

/**
	Free resources used by given QueueBuffer.
*/
void qb_free(struct QueueBuffer **qbuf);

/**
	Add len bytes from argument buffer to the provided QueueBuffer.
*/
void qb_push(struct QueueBuffer *qbuf, const void *buffer, size_t len);

/**
	Pop the next numbytes off the QueueBuffer into the given buffer.
	
	Returns the number of bytes actually popped. This could be less than
	numbytes if the number of bytes stored in the QueueBuffer is less than
	numbytes.
*/
int qb_pop(struct QueueBuffer *qbuf, void *buffer, size_t numbytes);

/**
	Returns the number of bytes currently stored by qbuf.
*/
int qb_getSize(struct QueueBuffer *qbuf);

#endif

