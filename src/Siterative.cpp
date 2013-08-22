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

// #define CDEBUG(text, arg) printf(text, arg)	// uncomment for debugging purposes
#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

using namespace std;

struct thread_info_ext {    		
           pthread_t thread_id;        /* ID returned by pthread_create() */ //  pthread_t = long unsigned int
           int       thread_num, sockfd, portno, n, j,  execution_mode, mbytes, k_chunks, n_iterations, i;
           float t;
	   char read_buffer[256];
	   sockaddr_in serv_addr;
	   fd_set input;		
	   fd_set output;		
	   timeval timeout;
	   FILE *fp;
	   size_t written, read;
	   hostent *server;
	   char* delimiter;
	   char* ifilepath;
	   char* expected_answer;
	   char* send_buffer;
	   char  file_buffer[5000];	// fixed size
	   
       };
       
thread_info_ext *tinfo_ext;     

// for iterative functions 
void thread_iter_init() { 	
	if (input_file) 
	      tinfo_ext = (thread_info_ext * )  calloc(threads_create, ( (sizeof(thread_info_ext)) + (expected_answer.size())+300+5000+(ifilepath.size()) ));	// array with the new mem.
	else 
	      tinfo_ext = (thread_info_ext * )  calloc(threads_create,(sizeof(thread_info_ext)+expected_answer.size()+1+m_bytes));	
}	

void thread_make_iter(int i, int &n_iters)			// thread creation
{
        void * (*func_pointer)(void*);	
        
	// function selection:
	if (input_file) func_pointer = thread_main_file;	// input from file
	else if ((execution_mode == 1) or (execution_mode == 2)) func_pointer = thread_main_ext;			
	
	tinfo_ext[i].n_iterations = 	n_iters;
	tinfo_ext[i].thread_id = 	(pthread_t)i;			
	tinfo_ext[i].thread_num =	i;
	#ifdef CDEBUG
		if (not (input_file)) printf("thread %i. Iterations = %i\n",tinfo_ext[i].thread_num,  tinfo_ext[i].n_iterations);
        #endif  
	tinfo_ext[i].portno =		portno;
	tinfo_ext[i].k_chunks = 	k_chunks;
	tinfo_ext[i].mbytes = 		m_bytes;
	if (not (input_file))		// if it doesn't gets data from file
		tinfo_ext[i].send_buffer = 	new char[m_bytes];	
	
	tinfo_ext[i].execution_mode =	execution_mode;
	tinfo_ext[i].server =		server;
	tinfo_ext[i].t = 		t;	
	tinfo_ext[i].expected_answer =  new char[expected_answer.size()+1];
	strcpy( tinfo_ext[i].expected_answer, expected_answer.c_str());
	tinfo_ext[i].delimiter = new char[delimiter.size()+1];
	strcpy( tinfo_ext[i].delimiter, delimiter.c_str());
	tinfo_ext[i].ifilepath = new char[ifilepath.size()+1];
	strcpy( tinfo_ext[i].ifilepath, ifilepath.c_str());
	
	#ifdef CDEBUG
		if(i==0) printf("The size of the array of structs is: %i \n", sizeof(tinfo_ext[i]));
	#endif 
	pthread_create(&tinfo_ext[i].thread_id,NULL, func_pointer, (void *) &tinfo_ext[i] );	
	return;									
}


void * thread_main_ext(void *arg)						
{
     pthread_detach(pthread_self());
     thread_info_ext *private_tinfo_ext;					
     private_tinfo_ext = (thread_info_ext *) arg;				
     
	
      #ifdef CDEBUG
		printf("thread %i starting\n",private_tinfo_ext->thread_num);
      #endif   
         
for (private_tinfo_ext->i=0; private_tinfo_ext->i < private_tinfo_ext->n_iterations; private_tinfo_ext->i++)  { // simulation >1000 loop
  // conn to host
     private_tinfo_ext->sockfd = socket(AF_INET, SOCK_STREAM, 0);		
     if (private_tinfo_ext->sockfd < 0) {
	  #ifdef CDEBUG
		printf("ERROR opening socket");
	  #endif
	  delete[] private_tinfo_ext->expected_answer;
	  delete [] private_tinfo_ext->send_buffer;											
	  pthread_mutex_lock(&clifd_mutex);	
	  close(private_tinfo_ext->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo_ext->thread_id);
     }
     if (private_tinfo_ext->server == NULL) {
	  #ifdef CDEBUG
		fprintf(stderr,"ERROR, no such host\n");
	  #endif
	  delete[] private_tinfo_ext->expected_answer;
	  delete [] private_tinfo_ext->send_buffer;				
	  pthread_mutex_lock(&clifd_mutex);
	  close(private_tinfo_ext->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo_ext->thread_id);
     }
     bzero((char *) &(private_tinfo_ext->serv_addr), sizeof(private_tinfo_ext->serv_addr));
     private_tinfo_ext->serv_addr.sin_family = AF_INET;
     bcopy((char *)(private_tinfo_ext->server)->h_addr, 
	  (char *)&private_tinfo_ext->serv_addr.sin_addr.s_addr,
	  private_tinfo_ext->server->h_length);
     private_tinfo_ext->serv_addr.sin_port = htons(private_tinfo_ext->portno);
     
     if (connect(private_tinfo_ext->sockfd,(sockaddr *) &private_tinfo_ext->serv_addr,sizeof(private_tinfo_ext->serv_addr)) < 0) {
	  #ifdef CDEBUG
		printf("ERROR connecting");
	  #endif
	  delete[] private_tinfo_ext->expected_answer;
	  delete [] private_tinfo_ext->send_buffer;
	  pthread_mutex_lock(&clifd_mutex);
	  close(private_tinfo_ext->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo_ext->thread_id);
     }
     
     
     memset( private_tinfo_ext->send_buffer,'A',private_tinfo_ext->mbytes); // fill with bytes
     
     
  if (private_tinfo_ext->i==0) {	// just waits on the first iteration
	  #ifdef CDEBUG
		printf("Thread %i waiting orders. Socket =  %i \n", private_tinfo_ext->thread_num, private_tinfo_ext->sockfd);
	  #endif  
	  
	     running_threads++;
	  // thread waiting to be awaken
	     if ( (pthread_cond_wait(&clifd_cond, &clifd_mutex)!=0 ) ) {
		  #ifdef CDEBUG	
			cout<< "Error pthread_cond_wait ";
		  #endif
		  
		  delete [] private_tinfo_ext->send_buffer;		
		  pthread_mutex_lock(&clifd_mutex);
		  close(private_tinfo_ext->sockfd);							
		  running_threads--;
		  nthreads--;					
		  pthread_mutex_unlock(&clifd_mutex);
		  pthread_exit((void*) private_tinfo_ext->thread_id);	
	     }
	     pthread_mutex_unlock(&clifd_mutex);	 

	  #ifdef CDEBUG
	     CDEBUG("Thread %i awaken. \n", private_tinfo_ext->thread_num);
	  #endif
	     if (private_tinfo_ext->execution_mode == 1) private_tinfo_ext->k_chunks=1;	// Mode 1 = 1 iteration
}	

  // Send loop	 	
     for (private_tinfo_ext->j=0; private_tinfo_ext->j < private_tinfo_ext->k_chunks; private_tinfo_ext->j++) {	 // n iterations = k_chunks
	  
	  private_tinfo_ext->n = send(private_tinfo_ext->sockfd,private_tinfo_ext->send_buffer,private_tinfo_ext->mbytes, MSG_NOSIGNAL); // writing buffer
	  if (private_tinfo_ext->n < 0) {
	       #ifdef CDEBUG
		printf("ERROR writing bytes to socket.  Thread exits!!!\n ");
	       #endif
	       delete[] private_tinfo_ext->expected_answer;
	       delete [] private_tinfo_ext->send_buffer;
	       delete [] private_tinfo_ext->ifilepath;		
	       pthread_mutex_lock(&clifd_mutex);	       			 
	       close(private_tinfo_ext->sockfd);						
	       conn_error++;
	       running_threads--;
	       nthreads--;					
	       pthread_mutex_unlock(&clifd_mutex);
	       pthread_exit((void*) private_tinfo_ext->thread_id);		
	  }
	  #ifdef CDEBUG
		printf("Thread %i: %i bytes sent \n", private_tinfo_ext->thread_num , private_tinfo_ext->mbytes );
	  #endif
     }		// end_for
	  
	  if (end_msg) {				               // to alert the server when all the data is sent
	       FD_ZERO(&private_tinfo_ext->output);						// clears the set
	       FD_SET(private_tinfo_ext->sockfd, &private_tinfo_ext->output);			// setting the fd to watch
	       private_tinfo_ext->timeout.tv_sec  = 7; 
	       private_tinfo_ext->timeout.tv_usec = 0;
	     
	       private_tinfo_ext->n = select(private_tinfo_ext->sockfd + 1, NULL , &private_tinfo_ext->output, NULL, &private_tinfo_ext->timeout); // IM: it is always + 1
	       
	       
	       if (private_tinfo_ext->n == -1) 	
		    printf("Thread %i: ERROR sending delim. to server\n" , private_tinfo_ext->thread_num);
		    
	       else if (private_tinfo_ext->n == 0) 	//timeout
			      printf("Thread %i: TIMEOUT in send operation to server\n" , private_tinfo_ext->thread_num);
			      
	       if (!FD_ISSET(private_tinfo_ext->sockfd, &private_tinfo_ext->output))  //again something wrong
		    printf("Thread %i: ERROR sending delim. to server\n" , private_tinfo_ext->thread_num);	 
	       else {       
	//here we can call non-blockable send
		       private_tinfo_ext->n = send(private_tinfo_ext->sockfd,private_tinfo_ext->delimiter ,sizeof(private_tinfo_ext->delimiter)-1, MSG_NOSIGNAL);	
		       if (private_tinfo_ext->n < 0) {
			    #ifdef CDEBUG
				printf("ERROR writing delimiter to socket, probably the other end has gone. Thread exits!!!\n");
			    #endif
			    delete[] private_tinfo_ext->expected_answer;
			    delete [] private_tinfo_ext->send_buffer;
			    delete [] private_tinfo_ext->ifilepath;			
			    pthread_mutex_lock(&clifd_mutex);	
			    close(private_tinfo_ext->sockfd);						
			    conn_error++;
			    running_threads--;
			    nthreads--;					
			    pthread_mutex_unlock(&clifd_mutex);
			    pthread_exit((void*) private_tinfo_ext->thread_id);		// ends the thread immediately
	  }	      
	  }
     }  
	  
	  if (ex_answer) {		// if an answer is expected
	       FD_ZERO(&private_tinfo_ext->input);						
	       FD_SET(private_tinfo_ext->sockfd, &private_tinfo_ext->input);			
	       private_tinfo_ext->timeout.tv_sec  = 7; 
	       private_tinfo_ext->timeout.tv_usec = 0;
	       private_tinfo_ext->n = select(private_tinfo_ext->sockfd + 1, &private_tinfo_ext->input, NULL, NULL, &private_tinfo_ext->timeout); 
	       
	       if (private_tinfo_ext->n == -1) 
		    printf("Thread %i: ERROR reading from server\n" , private_tinfo_ext->thread_num);
		    
	       else if (private_tinfo_ext->n == 0) 	
			      printf("Thread %i: TIMEOUT in read operation from server\n" , private_tinfo_ext->thread_num);
			      
	       if (!FD_ISSET(private_tinfo_ext->sockfd, &private_tinfo_ext->input))  
		    printf("Thread %i: ERROR reading from server\n" , private_tinfo_ext->thread_num);	 
	       else {       
	//here we can call non-blockable read
		    memset(private_tinfo_ext->read_buffer, 0, sizeof(private_tinfo_ext->read_buffer));			// clear buffer
		    private_tinfo_ext->n = read(private_tinfo_ext->sockfd,private_tinfo_ext->read_buffer,255);		
		    if (private_tinfo_ext->n < 0) {		
			 #ifdef CDEBUG
				CDEBUG("Thread %i: ERROR reading from socket\n" , private_tinfo_ext->thread_num);
			 #endif
			 delete[] private_tinfo_ext->expected_answer;
			 delete [] private_tinfo_ext->send_buffer;							
			 pthread_mutex_lock(&clifd_mutex);
			 close(private_tinfo_ext->sockfd);						
			 wrong_answers++;
			 running_threads--;
			 nthreads--;					
			 pthread_mutex_unlock(&clifd_mutex);
			 pthread_exit((void*) private_tinfo_ext->thread_id);
	       }
		#ifdef CDEBUG
			// CDEBUG("Received: %s\n",private_tinfo_ext->read_buffer);				
		#endif
	  }
	  }

	pthread_mutex_lock(&clifd_mutex);	
		if ( strcmp(private_tinfo_ext->read_buffer,private_tinfo_ext->expected_answer)==0) correct_answers++;	// checking answer 
		else wrong_answers++;		
		nthreads--;			// decrease at the end of every it.
	pthread_mutex_unlock(&clifd_mutex); 		
	close(private_tinfo_ext->sockfd);	// close sockfd on every it.
  } 	// end_for
     
     
// ending
     #ifdef CDEBUG
	CDEBUG("Thread %i closing \n", private_tinfo_ext->thread_num );	
     #endif					  
     delete[] private_tinfo_ext->expected_answer;
     delete[] private_tinfo_ext->send_buffer;
     pthread_mutex_lock(&clifd_mutex);	
     running_threads--;
     pthread_mutex_unlock(&clifd_mutex);
     pthread_exit((void*) private_tinfo_ext->thread_id);		
     return 0;
}


/* ----------------------------------------------------- */
		// Input data from file //
//-------------------------------------------------------//
void * thread_main_file(void *arg)						
{
     pthread_detach(pthread_self());
     thread_info_ext *private_tinfo_ext;					
     private_tinfo_ext = (thread_info_ext *) arg;				// pointer to the struct
     #ifdef CDEBUG
		printf("The size of every struct: %i \n", sizeof(private_tinfo_ext));
	#endif 
	
      #ifdef CDEBUG
		printf("thread %i starting\n",private_tinfo_ext->thread_num);
      #endif   
     // host connection
     private_tinfo_ext->i=0;	
     private_tinfo_ext->sockfd = socket(AF_INET, SOCK_STREAM, 0);		
     if (private_tinfo_ext->sockfd < 0) {
	  #ifdef CDEBUG
		printf("ERROR opening socket");
	  #endif
	  delete[] private_tinfo_ext->expected_answer;
	  delete [] private_tinfo_ext->send_buffer;										
	  pthread_mutex_lock(&clifd_mutex);	
	  close(private_tinfo_ext->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo_ext->thread_id);
     }
     if (private_tinfo_ext->server == NULL) {
	  #ifdef CDEBUG
		fprintf(stderr,"ERROR, no such host\n");
	  #endif
	  delete[] private_tinfo_ext->expected_answer;
	  delete [] private_tinfo_ext->send_buffer;				
	  pthread_mutex_lock(&clifd_mutex);
	  close(private_tinfo_ext->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo_ext->thread_id);
     }
     bzero((char *) &(private_tinfo_ext->serv_addr), sizeof(private_tinfo_ext->serv_addr));
     private_tinfo_ext->serv_addr.sin_family = AF_INET;
     bcopy((char *)(private_tinfo_ext->server)->h_addr, 	
	  (char *)&private_tinfo_ext->serv_addr.sin_addr.s_addr,
	  private_tinfo_ext->server->h_length);
     private_tinfo_ext->serv_addr.sin_port = htons(private_tinfo_ext->portno);
     
     if (connect(private_tinfo_ext->sockfd,(sockaddr *) &private_tinfo_ext->serv_addr,sizeof(private_tinfo_ext->serv_addr)) < 0) {
	  #ifdef CDEBUG
		printf("ERROR connecting");
	  #endif
	  delete[] private_tinfo_ext->expected_answer;
	  delete [] private_tinfo_ext->send_buffer;
	  pthread_mutex_lock(&clifd_mutex);
	  close(private_tinfo_ext->sockfd);						
	  conn_error++;
	  nthreads--;					
	  pthread_mutex_unlock(&clifd_mutex);
	  pthread_exit((void*) private_tinfo_ext->thread_id);
     }
      
     //  file opened in binary mode
     private_tinfo_ext->fp = fopen( (const char*) private_tinfo_ext->ifilepath , "rb+"); 
     if( private_tinfo_ext->fp == NULL) {
	      printf("Thread %i: ERROR sending opening file\n" , private_tinfo_ext->thread_num);
	      delete[] private_tinfo_ext->expected_answer;
	      delete [] private_tinfo_ext->send_buffer;		
	      pthread_mutex_lock(&clifd_mutex);	
	      close(private_tinfo_ext->sockfd);						
	      conn_error++;
	      running_threads--;
	      nthreads--;					
	      pthread_mutex_unlock(&clifd_mutex);
	      pthread_exit((void*) private_tinfo_ext->thread_id);	
      }
      fseek ( private_tinfo_ext->fp , 0 , SEEK_SET );	 // beginning of the file
	  
  
  if (private_tinfo_ext->i==0) {	// just waits on the first it.
	  #ifdef CDEBUG
		printf("Thread %i waiting orders. Socket =  %i \n", private_tinfo_ext->thread_num, private_tinfo_ext->sockfd);
	  #endif  
	     pthread_mutex_lock(&clifd_mutex);
	     running_threads++;
	     // thread waiting to be awaken
	     if ( (pthread_cond_wait(&clifd_cond, &clifd_mutex)!=0 ) ) {
		  #ifdef CDEBUG	
			cout<< "Error pthread_cond_wait ";
		  #endif	
		  
		  close(private_tinfo_ext->sockfd);							
		  running_threads--;
		  nthreads--;					
		  pthread_mutex_unlock(&clifd_mutex);
		  pthread_exit((void*) private_tinfo_ext->thread_id);	
	     }
	     pthread_mutex_unlock(&clifd_mutex);	
	  #ifdef CDEBUG
	     CDEBUG("Thread %i awaken. \n", private_tinfo_ext->thread_num);
	  #endif
	     
}	

  // send
     private_tinfo_ext->read=1;	 	
     while (private_tinfo_ext->read > 0 ) {	// while it keeps reading from file 
	  strcpy (private_tinfo_ext->file_buffer, "");
	  pthread_mutex_lock(&clifd_mutex);
		private_tinfo_ext->read = fread(&private_tinfo_ext->file_buffer, 1, 5000, private_tinfo_ext->fp );
	  pthread_mutex_unlock(&clifd_mutex);
	  if (private_tinfo_ext->read <= 0) break;
	  #ifdef CDEBUG
	  printf("Size of send_buffer: %i ", sizeof(private_tinfo_ext->file_buffer));
	  printf("Sending chunk of image. Thread:%i Bytes: %i Content:%s\n" , private_tinfo_ext->thread_num, private_tinfo_ext->read, private_tinfo_ext->file_buffer);
	  #endif
	   
	   if (private_tinfo_ext->i==0) {	// select() only in the first it.
	       FD_ZERO(&private_tinfo_ext->output);						
	       FD_SET(private_tinfo_ext->sockfd, &private_tinfo_ext->output);			
	       private_tinfo_ext->timeout.tv_sec  = 7;
	       private_tinfo_ext->timeout.tv_usec = 0;
	    
	       private_tinfo_ext->n = select(private_tinfo_ext->sockfd + 1, NULL , &private_tinfo_ext->output, NULL, &private_tinfo_ext->timeout); 
	       	       
	       if (private_tinfo_ext->n == -1) //something wrong
		    printf("Thread %i: ERROR sending delim. to server\n" , private_tinfo_ext->thread_num);
		    
	       else if (private_tinfo_ext->n == 0) 	//timeout
			      printf("Thread %i: TIMEOUT in send operation to server\n" , private_tinfo_ext->thread_num);
			      
	       if (!FD_ISSET(private_tinfo_ext->sockfd, &private_tinfo_ext->output))  //again something wrong
		    printf("Thread %i: ERROR sending delim. to server\n" , private_tinfo_ext->thread_num);	 
	       else {       
	//here we can call non-blockable send
	  private_tinfo_ext->n = send(private_tinfo_ext->sockfd, private_tinfo_ext->file_buffer, private_tinfo_ext->read , MSG_NOSIGNAL); // writing buffer
	  if (private_tinfo_ext->n < 0) {
	       #ifdef CDEBUG
		printf("ERROR writing bytes to socket.  Thread exits!!!\n ");
	       #endif
	   	
	       pthread_mutex_lock(&clifd_mutex);	       			 
	       close(private_tinfo_ext->sockfd);						
	       conn_error++;
	       running_threads--;
	       nthreads--;					
	       pthread_mutex_unlock(&clifd_mutex);
	       pthread_exit((void*) private_tinfo_ext->thread_id);		
	  }
	  #ifdef CDEBUG
		printf("Thread %i: %i bytes sent \n", private_tinfo_ext->thread_num , private_tinfo_ext->mbytes );
	  #endif
     }		
  }
	else {		// is not the first it.
		  private_tinfo_ext->n = send(private_tinfo_ext->sockfd, private_tinfo_ext->file_buffer, private_tinfo_ext->read , MSG_NOSIGNAL);
		  if (private_tinfo_ext->n < 0) {
		       #ifdef CDEBUG
			printf("ERROR writing bytes to socket.  Thread exits!!!\n ");
		       #endif
		    	
		       pthread_mutex_lock(&clifd_mutex);	       			 
		       close(private_tinfo_ext->sockfd);							
		       conn_error++;
		       running_threads--;
		       nthreads--;					
		       pthread_mutex_unlock(&clifd_mutex);
		       pthread_exit((void*) private_tinfo_ext->thread_id);		
		  }
		  #ifdef CDEBUG
			printf("Thread %i: %i bytes sent \n", private_tinfo_ext->thread_num , private_tinfo_ext->mbytes );
		  #endif
     }
     private_tinfo_ext->i++;	
}	

	  if (end_msg) {				               
	       FD_ZERO(&private_tinfo_ext->output);						
	       FD_SET(private_tinfo_ext->sockfd, &private_tinfo_ext->output);			
	       private_tinfo_ext->n = select(private_tinfo_ext->sockfd + 1, NULL , &private_tinfo_ext->output, NULL, &private_tinfo_ext->timeout); 
	       
	       
	       if (private_tinfo_ext->n == -1) 
		    printf("Thread %i: ERROR sending delim. to server\n" , private_tinfo_ext->thread_num);
		    
	       else if (private_tinfo_ext->n == 0) 	
			      printf("Thread %i: TIMEOUT in send operation to server\n" , private_tinfo_ext->thread_num);
			      
	       if (!FD_ISSET(private_tinfo_ext->sockfd, &private_tinfo_ext->output))  
		    printf("Thread %i: ERROR sending delim. to server\n" , private_tinfo_ext->thread_num);	 
	       else {       
	//here we can call not blockable send
		       private_tinfo_ext->n = send(private_tinfo_ext->sockfd,private_tinfo_ext->delimiter ,sizeof(private_tinfo_ext->delimiter)-1, MSG_NOSIGNAL);	
		       if (private_tinfo_ext->n < 0) {
			    #ifdef CDEBUG
				printf("ERROR writing delimiter to socket, probably the other end has gone. Thread exits!!!\n");
			    #endif
			    delete[] private_tinfo_ext->expected_answer;	
			    pthread_mutex_lock(&clifd_mutex);	
			    close(private_tinfo_ext->sockfd);						
			    conn_error++;
			    running_threads--;
			    nthreads--;					
			    pthread_mutex_unlock(&clifd_mutex);
			    pthread_exit((void*) private_tinfo_ext->thread_id);		
	  }	      
	  }
     }  
	  
	  if (ex_answer) {		
	  
	  // Set non-blocking read with timeout 	       
	       FD_ZERO(&private_tinfo_ext->input);						
	       FD_SET(private_tinfo_ext->sockfd, &private_tinfo_ext->input);			
	       private_tinfo_ext->timeout.tv_sec  = 48; // server processing may take more time
	       private_tinfo_ext->timeout.tv_usec = 0;
	       private_tinfo_ext->n = select(private_tinfo_ext->sockfd + 1, &private_tinfo_ext->input, NULL, NULL, &private_tinfo_ext->timeout); 
	       
	       if (private_tinfo_ext->n == -1) 
		    printf("Thread %i: ERROR reading from server\n" , private_tinfo_ext->thread_num);
		    
	       else if (private_tinfo_ext->n == 0) 	
			      printf("Thread %i: TIMEOUT in read operation from server\n" , private_tinfo_ext->thread_num);
			      
	       if (!FD_ISSET(private_tinfo_ext->sockfd, &private_tinfo_ext->input))  
		    printf("Thread %i: ERROR reading from server\n" , private_tinfo_ext->thread_num);	 
	       else {       
		    memset(private_tinfo_ext->read_buffer, 0, sizeof(private_tinfo_ext->read_buffer));			
		    private_tinfo_ext->n = read(private_tinfo_ext->sockfd,private_tinfo_ext->read_buffer,255);		
		    if (private_tinfo_ext->n < 0) {	
			 #ifdef CDEBUG
				CDEBUG("Thread %i: ERROR reading from socket\n" , private_tinfo_ext->thread_num);
			 #endif
			 delete[] private_tinfo_ext->expected_answer;							
			 pthread_mutex_lock(&clifd_mutex);
			 close(private_tinfo_ext->sockfd);						
			 wrong_answers++;
			 running_threads--;
			 nthreads--;					
			 pthread_mutex_unlock(&clifd_mutex);
			 pthread_exit((void*) private_tinfo_ext->thread_id);
	       }
		#ifdef CDEBUG
			// CDEBUG("Received: %s\n",private_tinfo_ext->read_buffer);				
		#endif
	  }
	  }
	pthread_mutex_lock(&clifd_mutex);	
		if ( strcmp(private_tinfo_ext->read_buffer,private_tinfo_ext->expected_answer)==0) correct_answers++;	// checking answer
		else wrong_answers++;		
		nthreads--;			
	pthread_mutex_unlock(&clifd_mutex); 		
	close(private_tinfo_ext->sockfd);						
      
 // ending
     #ifdef CDEBUG
	CDEBUG("Thread %i closing \n", private_tinfo_ext->thread_num );	
     #endif	
     fclose(private_tinfo_ext->fp);				  
     delete[] private_tinfo_ext->expected_answer;
     pthread_mutex_lock(&clifd_mutex);	
     running_threads--;
     pthread_mutex_unlock(&clifd_mutex);
     pthread_exit((void*) private_tinfo_ext->thread_id);		// thread exits returning it's id 
     return 0;
}

