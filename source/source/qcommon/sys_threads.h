/*
Copyright (C) 2013 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef SYS_THREADS_H
#define SYS_THREADS_H

typedef struct qbufQueue_s qbufQueue_t;

int Sys_Thread_Create( qthread_t **pthread, void *(*routine) (void*), void *param );
void Sys_Thread_Join( qthread_t *thread );

int Sys_Mutex_Create( qmutex_t **pmutex );
void Sys_Mutex_Destroy( qmutex_t *mutex );
void Sys_Mutex_Lock( qmutex_t *mutex );
void Sys_Mutex_Unlock( qmutex_t *mutex );
int Sys_Atomic_Add( volatile int *value, int add, qmutex_t *mutex );

qbufQueue_t *Sys_BufQueue_Create( size_t bufSize, int flags );
void Sys_BufQueue_Destroy( qbufQueue_t **pqueue );
void Sys_BufQueue_Finish( qbufQueue_t *queue );
void Sys_BufQueue_EnqueueCmd( qbufQueue_t *queue, const void *cmd, unsigned cmd_size );
int Sys_BufQueue_ReadCmds( qbufQueue_t *queue, unsigned (**cmdHandlers)( const void * ) );

#endif // SYS_THREADS_H
