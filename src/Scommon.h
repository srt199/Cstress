/*
 * Cstress Copyright 2013 Sergi Álvarez Triviño

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#ifndef Scommon_H_				
#define Scommon_H_

#include <stdio.h>
#include <ctype.h>
#include <iostream>
#include <memory>					
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 					
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <tclap/CmdLine.h>	 			// CLI parser API
#include <boost/timer.hpp>

extern "C" {
#ifndef _OPEN_THREADS
#define _OPEN_THREADS
#endif

#include <errno.h>
#include <pthread.h>
#include "string.h"
#include <stdlib.h>

}

// shared variables
extern int			correct_answers, wrong_answers, running_threads, nthreads, threads_create, restante, total_threads; 
extern int 			ngroup1, ngroup2, ngroup3, itgroup1, itgroup2;	
extern const int 		send_buffer_size;
extern bool			kill, ex_answer, end_msg, force, input_file;	
extern pthread_mutex_t	clifd_mutex;
extern pthread_mutex_t	thread_variables_mutex;	
extern pthread_cond_t	clifd_cond;
extern pthread_cond_t	thread_end_cond;			// end of all threads
extern int 			sockfd, portno, n, execution_mode,  m_bytes, k_chunks, conn_error;  
extern float 			t ;
extern std::string 		expected_answer, delimiter, ifilepath;
extern sockaddr_in 		serv_addr;
extern hostent 		*server;

// ----------------------
extern "C" {
void* calloc (size_t num, size_t size);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
}

void thread_make(int);
void thread_make_iter(int, int&);
void thread_make_real(int, int);
void * thread_main1(void *);
void * thread_main2(void *);
void * thread_main3(void *);
void * thread_main_ext(void *);
void * thread_main_real(void *);
void * thread_main_file(void *);
void * thread_main1999(void *);
void kill_threads(int);
void thread_info_init();
void thread_iter_init();
void thread_real_init();
void start_chrono_thread(int );
void start_chrono_real(int );
void * chrono_handler(void*);
void * chrono_handler_real(void*);

#endif
