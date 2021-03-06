/*
 *  This file is for use by students to define anything they wish.  It is used by the gf server implementation
 */
#ifndef __GF_SERVER_STUDENT_H__
#define __GF_SERVER_STUDENT_H__

#include "gf-student.h"
#include "gfserver.h"
#include "content.h"
#include "steque.h"
#include <fcntl.h>

// Element in the queue
typedef struct steque_package
{
    gfcontext_t *ctx;
    const char *path;
    void* arg;
} steque_package;

void init_threads(size_t numthreads);
void cleanup_threads();

#endif // __GF_SERVER_STUDENT_H__
