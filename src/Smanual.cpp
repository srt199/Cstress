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

#include "Scommon.h"

// Manual mode			
// #define CDEBUG(text, arg) printf(text, arg)// uncomment for debugging purposes
#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
               
               
using namespace std;

const int send_buffer_size = const_cast<const int&>(m_bytes);

struct thread_info {    		/* thread local data */
           pthread_t thread_id;
           int       thread_num, sockfd, portno, n, j,  execution_mode, mbytes, k_chunks;
           float t;
	   char read_buffer[256];
	   sockaddr_in serv_addr;
	   fd_set input;		
	   fd_set output;	
	   timeval timeout;
	   hostent *server;
	   char* expected_answer;
	   char* delimiter;
	   char *send_buffer;		   
       };
       
thread_info *tinfo;     
bool kill=false;				// alerts of cancellation actions
pthread_once_t once_control = PTHREAD_ONCE_INIT;
pthread_key_t tlskey;
int s;
pthread_attr_t attr;

void thread_info_init() {		
		tinfo = (thread_info * )  calloc(nthreads,(sizeof(thread_info)+expected_answer.size()+1+m_bytes+15000));// array 
		 #ifdef CDEBUG
			CDEBUG("The size of the array of structs is: %i \n", sizeof(tinfo));
		 #endif
}  
     
void kill_threads(int k)
/* Function that manages self-cancellation of threads depending on the variable k received. (unused)
 *  k=0 : Cancelation of all ths
 */
{
   if (k==0) {				
	kill = true;
	while(nthreads!=0) {		
	     sleep(1);			// sleeps for a second after checking again if all running threads were killed 
	  }
	kill = false;			
	printf("Alert: All worker-threads were killed succesfully");
   }
}
	  
void thread_make(int i)			// thread creation
{
        void * (*func_pointer)(void*);	
	
	if ((execution_mode == 1) or (execution_mode == 2)) func_pointer = thread_main1;			

	tinfo[i].thread_id= (pthread_t)i;			
	tinfo[i].thread_num=i;
	tinfo[i].portno=portno;
	tinfo[i].k_chunks=k_chunks;
	tinfo[i].mbytes=m_bytes;
	tinfo[i].send_buffer= new char[m_bytes];	
	tinfo[i].execution_mode=execution_mode;
	tinfo[i].server=server;
	tinfo[i].t=t;	
	tinfo[i].expected_answer = new char[expected_answer.size()+1];
	strcpy( tinfo[i].expected_answer, expected_answer.c_str());
	tinfo[i].delimiter = new char[delimiter.size()];
	strcpy( tinfo[i].delimiter, delimiter.c_str());
	
	#ifdef CDEBUG
		if(i==0) printf("The size of the array of structs is: %i \n", sizeof(tinfo));
	#endif
	pthread_create(&tinfo[i].thread_id,NULL, func_pointer, (void *) &tinfo[i] );	// param = address to it's part of the array
	return;									
}

// CHRONO
void start_chrono_thread(int i) {
     pthread_t cht_id;
     void * (*func_pointer)(void *);
     func_pointer = chrono_handler;
     int* p = &i;
     pthread_create(&cht_id, NULL, func_pointer,(void *) &p );	// if returns EAGAIN = max. number threads
     pthread_detach(cht_id);		
     return;									
}

void * chrono_handler(void* arg) {
     usleep(500);				
     while (nthreads > 0) usleep(35000);	// every 0.35 s. polls the number of living threads
     sleep(1);					// in the case of -f, to let the last th finish correctly
     free(tinfo);				
     int *p_id = (int*) arg;
     pthread_cond_signal(&thread_end_cond);	// signal to the main thread
     pthread_exit((void*) p_id);		
     return 0;
}

void * thread_main1(void *arg)						
{
     pthread_detach(pthread_self());
     thread_info *private_tinfo;					
     private_tinfo = (thread_info *) arg;				
     
     #ifdef CDEBUG
		printf("thread %i starting\n",private_tinfo->thread_num);
     #endif      
     
  // host conn
     private_tinfo->sockfd = socket(AF_INET, SOCK_STREAM, 0);	 
     if (private_tinfo->sockfd < 0) {
	  #ifdef CDEBUG
		printf("ERROR opening socket");
	  #endif
	  delete[] private_tinfo->expected_answer;
	  delete [] private_tinfo->send_buffer;											
	  pthread_mutex_lock(&clifd_mutex);	
	  close(private_tinfo->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo->thread_id);
     }
     if (private_tinfo->server == NULL) {
	  #ifdef CDEBUG
		fprintf(stderr,"ERROR, no such host\n");
	  #endif
	  delete[] private_tinfo->expected_answer;
	  delete [] private_tinfo->send_buffer;				
	  pthread_mutex_lock(&clifd_mutex);
	  close(private_tinfo->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo->thread_id);
     }
     bzero((char *) &(private_tinfo->serv_addr), sizeof(private_tinfo->serv_addr));
     private_tinfo->serv_addr.sin_family = AF_INET;
     bcopy((char *)(private_tinfo->server)->h_addr, 	 
	  (char *)&private_tinfo->serv_addr.sin_addr.s_addr,
	  private_tinfo->server->h_length);
     private_tinfo->serv_addr.sin_port = htons(private_tinfo->portno);
    
     if (connect(private_tinfo->sockfd,(sockaddr *) &private_tinfo->serv_addr,sizeof(private_tinfo->serv_addr)) < 0) {
	  #ifdef CDEBUG
		printf("ERROR connecting");
	  #endif
	  delete[] private_tinfo->expected_answer;
	  delete [] private_tinfo->send_buffer;
	  pthread_mutex_lock(&clifd_mutex);
	  close(private_tinfo->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo->thread_id);
     }
     
  
     memset( private_tinfo->send_buffer,'A',private_tinfo->mbytes); // fill with bytes
	  
 
     #ifdef CDEBUG
	printf("Thread %i waiting orders. Socket =  %i \n", private_tinfo->thread_num, private_tinfo->sockfd);
     #endif  
  
     running_threads++;
     if ( (pthread_cond_wait(&clifd_cond, &clifd_mutex)!=0 ) ) {
	  #ifdef CDEBUG	
		cout<< "Error pthread_cond_wait ";
	  #endif
	  delete [] private_tinfo->send_buffer;		
          pthread_mutex_lock(&clifd_mutex);
	  close(private_tinfo->sockfd);							
          running_threads--;
          nthreads--;					
          pthread_mutex_unlock(&clifd_mutex);
          pthread_exit((void*) private_tinfo->thread_id);	
     }
     pthread_mutex_unlock(&clifd_mutex);	
      
  #ifdef CDEBUG
     CDEBUG("Thread %i awaken. \n", private_tinfo->thread_num);
  #endif
     if (private_tinfo->execution_mode == 1) private_tinfo->k_chunks=1;	// Mode 1 = only 1 iteration
	
  // send loop
     for (private_tinfo->j=0; private_tinfo->j < private_tinfo->k_chunks; private_tinfo->j++) {	// n iterations = k_chunks
	  
	  private_tinfo->n = send(private_tinfo->sockfd,private_tinfo->send_buffer,private_tinfo->mbytes, MSG_NOSIGNAL); 
	  if (private_tinfo->n < 0) {
	       #ifdef CDEBUG
		printf("ERROR writing bytes to socket.  Thread exits!!!\n ");
	       #endif
	       delete[] private_tinfo->expected_answer;
	       delete [] private_tinfo->send_buffer;		
	       pthread_mutex_lock(&clifd_mutex);	       			 
	       close(private_tinfo->sockfd);							
	       conn_error++;
	       running_threads--;
	       nthreads--;					
	       pthread_mutex_unlock(&clifd_mutex);
	       pthread_exit((void*) private_tinfo->thread_id);		// ends with the thread immediately
	  }
	  #ifdef CDEBUG
		printf("Thread %i: %i bytes sent \n", private_tinfo->thread_num , private_tinfo->mbytes );
	  #endif
     }		// fi_for
	  
	  if (end_msg) {				               // to alert the server when all the data is sent
	       FD_ZERO(&private_tinfo->output);						// clears the set
	       FD_SET(private_tinfo->sockfd, &private_tinfo->output);			// setting the fd to watch
	       private_tinfo->timeout.tv_sec  = 7; 
	       private_tinfo->timeout.tv_usec = 0;
	    
	       private_tinfo->n = select(private_tinfo->sockfd + 1, NULL , &private_tinfo->output, NULL, &private_tinfo->timeout); // IM: it is always + 1
	       
	       
	       if (private_tinfo->n == -1) //something wrong
		    printf("Thread %i: ERROR sending delim. to server\n" , private_tinfo->thread_num);
		    
	       else if (private_tinfo->n == 0) 	//timeout
			      printf("Thread %i: TIMEOUT in send operation to server\n" , private_tinfo->thread_num);
			      
	       if (!FD_ISSET(private_tinfo->sockfd, &private_tinfo->output))  //again something wrong
		    printf("Thread %i: ERROR sending delim. to server\n" , private_tinfo->thread_num);	 
	       else {       
	//here we can call non-blockable send
	       private_tinfo->n = send(private_tinfo->sockfd,private_tinfo->delimiter ,sizeof(private_tinfo->delimiter)-1, MSG_NOSIGNAL);	
	       if (private_tinfo->n < 0) {
		    #ifdef CDEBUG
			printf("ERROR writing delimiter to socket, probably the other end has gone. Thread exits!!!\n");
	            #endif
	            delete[] private_tinfo->expected_answer;
		    delete [] private_tinfo->send_buffer;		
		    pthread_mutex_lock(&clifd_mutex);	
		    close(private_tinfo->sockfd);						
		    conn_error++;
		    running_threads--;
		    nthreads--;					
		    pthread_mutex_unlock(&clifd_mutex);
		    pthread_exit((void*) private_tinfo->thread_id);		
	  }	      
	  }
     }  
	  
	  if (ex_answer) {	// if an answer is expected  	       
	       FD_ZERO(&private_tinfo->input);						
	       FD_SET(private_tinfo->sockfd, &private_tinfo->input);			
	       private_tinfo->timeout.tv_sec  = 7; 
	       private_tinfo->timeout.tv_usec = 0;
	       private_tinfo->n = select(private_tinfo->sockfd + 1, &private_tinfo->input, NULL, NULL, &private_tinfo->timeout); // IM: it is always + 1
	       
	       if (private_tinfo->n == -1) 
		    printf("Thread %i: ERROR reading from server\n" , private_tinfo->thread_num);
		    
	       else if (private_tinfo->n == 0) 	
			      printf("Thread %i: TIMEOUT in read operation from server\n" , private_tinfo->thread_num);
			      
	       if (!FD_ISSET(private_tinfo->sockfd, &private_tinfo->input))  
		    printf("Thread %i: ERROR reading from server\n" , private_tinfo->thread_num);	 
	       else {       
	//here we can call not blockable read
		    memset(private_tinfo->read_buffer, 0, sizeof(private_tinfo->read_buffer));			
		    private_tinfo->n = read(private_tinfo->sockfd,private_tinfo->read_buffer,255);    // reads answer
		    if (private_tinfo->n < 0) {		
			 #ifdef CDEBUG
				CDEBUG("Thread %i: ERROR reading from socket\n" , private_tinfo->thread_num);
			 #endif
			 delete[] private_tinfo->expected_answer;
			 delete [] private_tinfo->send_buffer;							
			 pthread_mutex_lock(&clifd_mutex);
			 close(private_tinfo->sockfd);						
			 wrong_answers++;
			 running_threads--;
			 nthreads--;					
			 pthread_mutex_unlock(&clifd_mutex);
			 pthread_exit((void*) private_tinfo->thread_id);
	       }
		#ifdef CDEBUG
		         CDEBUG("Received: %s\n",private_tinfo->read_buffer);				
		#endif
	  }
	  }		
     close(private_tinfo->sockfd);						
     
     pthread_mutex_lock(&clifd_mutex);			
	  if ( strcmp(private_tinfo->read_buffer,private_tinfo->expected_answer)==0) correct_answers++;	// checking answer 
	  else wrong_answers++;
     pthread_mutex_unlock(&clifd_mutex);
     
  // ending
     #ifdef CDEBUG
	CDEBUG("Thread %i closing \n", private_tinfo->thread_num );	
     #endif					  
     delete[] private_tinfo->expected_answer;
     delete[] private_tinfo->send_buffer;
     pthread_mutex_lock(&clifd_mutex);	
     running_threads--;
     nthreads--;
     pthread_mutex_unlock(&clifd_mutex);
     pthread_exit((void*) private_tinfo->thread_id);		
     return 0;
}

