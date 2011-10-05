/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* this file contains system-dependent definitions used by ADB
 * they're related to threads, sockets and file descriptors
 */
#ifndef _ADB_SYSDEPS_H
#define _ADB_SYSDEPS_H

#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#define OS_PATH_SEPARATOR '/'
#define OS_PATH_SEPARATOR_STR "/"

typedef  pthread_mutex_t          adb_mutex_t;
#define  ADB_MUTEX_INITIALIZER    PTHREAD_MUTEX_INITIALIZER
#define  adb_mutex_init           pthread_mutex_init
#define  adb_mutex_lock           pthread_mutex_lock
#define  adb_mutex_unlock         pthread_mutex_unlock
#define  adb_mutex_destroy        pthread_mutex_destroy

#define  ADB_MUTEX_DEFINE(m)      static adb_mutex_t   m = PTHREAD_MUTEX_INITIALIZER

#define  adb_cond_t               pthread_cond_t
#define  adb_cond_init            pthread_cond_init
#define  adb_cond_wait            pthread_cond_wait
#define  adb_cond_broadcast       pthread_cond_broadcast
#define  adb_cond_signal          pthread_cond_signal
#define  adb_cond_destroy         pthread_cond_destroy

typedef  pthread_t                 adb_thread_t;

typedef void*  (*adb_thread_func_t)( void*  arg );

static __inline__ int  adb_thread_create( adb_thread_t  *pthread, adb_thread_func_t  start, void*  arg )
{
    pthread_attr_t   attr;

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

    return pthread_create( pthread, &attr, start, arg );
}

static __inline__ void  adb_sleep_ms( int  mseconds )
{
    usleep( mseconds*1000 );
}

#endif /* _ADB_SYSDEPS_H */
