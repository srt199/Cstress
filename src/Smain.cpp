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
#include <ctime>
		
using namespace std;

// -----------------------------
int running_threads=0, correct_answers=0, wrong_answers=0, sockfd, portno, n, execution_mode;
int threads_create=1000, nthreads=10, total_threads=10, m_bytes=5000, k_chunks=5, conn_error=0, restante=0, ngroup1, ngroup2, ngroup3, itgroup1, itgroup2;
float t=50;
bool ex_answer = true, end_msg=true, force=false, input_file = false;
std::string expected_answer = "I got your image";
std::string delimiter = "nnn";					// default
std::string ifilepath = "  ";

				
pthread_mutex_t	 	clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t		thread_variables_mutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t		thread_end_cond = PTHREAD_COND_INITIALIZER;		
sockaddr_in serv_addr;
hostent *server;
// -----------------------------

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

// argv[1]=hostname ;  argv[2]=port ; argv[3]=mode_number ; argv[4]= n_conn ; argv[5]= expected_answer ; argv[6]= n_bytes ;
// argv[7]=n_chunks ; argv[8]= simulation_time; argv[9]=input file

int main(int argc, char *argv[])
{
 // Wrap everything in a try block. 
	try {  
		TCLAP::CmdLine cmd("Command description message", ' ', "0.9");	
		// modes:
			TCLAP::SwitchArg manualSwitch("m","manual","The data is sent through all the connections at once", false);
			TCLAP::SwitchArg iteraSwitch("i","iterative","K chunks of data are sent one after another", false);
			TCLAP::SwitchArg realSwitch("r","realistic","It subjects the host to a simulation of n concurrent cl/srv communications", false);
		// make the 3 options exclusive
			 vector<TCLAP::Arg*>  xorlist;
			 xorlist.push_back(&manualSwitch);
                         xorlist.push_back(&iteraSwitch);
                         xorlist.push_back(&realSwitch);
			 cmd.xorAdd( xorlist );		
			
		// non-exclusive option:
			TCLAP::SwitchArg forceSwitch("f","force_simultaneity","With >1000 connections, forces to send bytes through all of them simultaneously (It can cause SIGSEGV depending on the config of your OS", false);

			TCLAP::UnlabeledValueArg<std::string>  hostnameArg( "hostname", "hostname", true, "localhost", "std::string" );	// localhost = default
		// Add the second ( argv[2]) argument hostname to the CmdLine object. 
			cmd.add( hostnameArg );
		// argv[3]=port
			TCLAP::UnlabeledValueArg<int>  portArg( "port", "port", true, 2009, "int" ); // 2009 = default
			cmd.add( portArg );
				
		// argv[4]= n_conn // not mandatory
			 TCLAP::ValueArg<int> connArg("c","connections","Number of concurrent connections to be made", false, 10 , "int");
			 cmd.add( connArg );
		
		// argv[5]= expected_answer
			 TCLAP::ValueArg<std::string> answerArg("a","answer","Expected answer",false,"","string");
			 cmd.add( answerArg );
			 
		// argv[6]= n_bytes
			 TCLAP::ValueArg<int> bytesArg("b","bytes","Number of bytes sent on every connection[-m] or every iteration[-i][-r]", false, 5000 , "int");
			 cmd.add( bytesArg );
			 
		// argv[7]= k_chunks
			 TCLAP::ValueArg<int> chunksArg("k","k_chunks","Number of chunks of bytes sent on every connection[-m][-i]", false, 5 , "int");
		         cmd.add( chunksArg );
		         
		// argv[8]= simulation_time
			 TCLAP::ValueArg<float> timeArg("t","time","Duration in seconds of the simulation[-r]", false, 50 , "float");
		         cmd.add( timeArg );
		         
	        // argv[8]= delimiter        
			 TCLAP::ValueArg<std::string> delimArg("d","delimiter","delimiter (marks when the client has finished sending data)",false,"","string");
			 cmd.add( delimArg );
			 
		// argv[9]= send data from file        
			 TCLAP::ValueArg<std::string> ifileArg("z","ifile","Filepath (data to be sent)",false,"","string");
			 cmd.add( ifileArg );
		
		// Parse the argv array.
			 cmd.parse( argc, argv );

	// --- Get every parsed arg: ---
	// MODE
		 if (manualSwitch.isSet()) execution_mode=1;
		 else if (iteraSwitch.isSet()) execution_mode=2;
		      else if (realSwitch.isSet()) execution_mode=3;
	// force simultaneity
	if (forceSwitch.isSet()) force=true;
	
	// TCP	
		server = 	gethostbyname(hostnameArg.getValue().c_str()) ;      
		portno = 	portArg.getValue();	
		nthreads = 	connArg.getValue();
	// exp_answer:
		if (answerArg.getValue()=="")  ex_answer = false;
		else expected_answer.assign(answerArg.getValue().c_str());
	// delimiter:
		if (delimArg.getValue()=="")  end_msg = false;
		else delimiter.assign(delimArg.getValue().c_str());
	// Input file:
		if (ifileArg.getValue()=="")  input_file = false;
		else {
			input_file = true;
			ifilepath.assign(ifileArg.getValue().c_str());
		}
		
		m_bytes = bytesArg.getValue();					
		k_chunks = chunksArg.getValue();
		t = timeArg.getValue();
	
/*-------------------------------------------------*/	
//  default connection
	 
	 cout << "Testing connection..." << endl;
	 
	  sockfd = socket(AF_INET, SOCK_STREAM, 0);		 
	  if (sockfd < 0) 
		    error("ERROR opening socket");
	 // hostname:
	  if (server == NULL) {
		    fprintf(stderr,"ERROR, no such host\n");
		    exit(0);
	       }
	  bzero((char *) &serv_addr, sizeof(serv_addr));
	  serv_addr.sin_family = AF_INET;
	  bcopy((char *)server->h_addr, 	 
	       (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	  serv_addr.sin_port = htons(portno); 
     
	  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
	       close(sockfd);  
	       error("ERROR connecting");
       }
	  
	  cout << "Connection OK!" << endl; 
	  close(sockfd);

	 if (execution_mode==1) 
		printf("Manual mode.\n Configuration: hostname=%s, port=%i, n_conn=%i, m_bytes=%i, force=%i \n-------------------------------\n", hostnameArg.getValue().c_str(), portno, nthreads , m_bytes, (int)force);
	 else if (execution_mode==2) 
		    printf("Iterative Mode.\n Configuration: hostname=%s, port=%i,  n_conn=%i, m_bytes=%i, k_chunks=%i, force=%i\n-------------------------------\n",hostnameArg.getValue().c_str(), portno, nthreads, m_bytes, k_chunks, (int)force);
	       else if (execution_mode==3) 
			  printf("Realist mode.\n Configuration: hostname=%s, port=%i,  n_conn=%i, m_bytes=%i, k_chunks=%i, time=%g\n-------------------------------\n",hostnameArg.getValue().c_str() , portno,  nthreads, m_bytes, k_chunks, t);
			 else error("ERROR, Insert a correct mode number");
	
	 if (ex_answer) printf("expected_answer = %s \n", (char*) expected_answer.c_str());	
	 if (end_msg) printf("delimiter = %s \n", (char*) delimiter.c_str());		 
	 if (input_file) printf("input filepath = %s \n", (char*) ifilepath.c_str());		
	 
    total_threads = nthreads;
    /* Prethreading*/
    printf("Wait until all connections are ready. (It can take some time...) \n");
   
    if (execution_mode==3) {		// realistic mode
	    if (nthreads>1000) {
		    printf("Warning: More than 1000 threads are about to be spawned. \n Type 'a' to abort or whatever else to continue");
		    std::string m3_input;
		    cin >> m3_input;
		    if (m3_input.compare("a")==0) exit(0);
	    }
	    
	    int i=0;
	    threads_create = nthreads;
	   // distribute ths on 3 groups
	    ngroup3 = nthreads%1000;
	    ngroup1 = (nthreads - ngroup3)/2;
	    ngroup2 = ngroup1;
	    
	    thread_real_init();	
	    
	    // generating personal thread delay
	    for (i = 0; i < ngroup1; i++) 
				thread_make_real(i, 0);				
     
	    for (i = 0; i < ngroup2; i++) 		// loop threads group 2
				thread_make_real(i, 1);	
				
	    for (i = 0 ; i < ngroup3; i++) 		// loop threads group 3
				thread_make_real(i, 2);	
	}
	        
	else {	    
	    if (nthreads>1000) {
		 
		    if (not force) {  
			    int i=0;
			    // preparing values to distribute "num iterations per thread"
			    ngroup2 = nthreads%1000;	     // n of threads belonging to the group2
			    itgroup1= (nthreads-ngroup2)/1000;
			    itgroup2= itgroup1+1;
			    ngroup1=  1000-ngroup2;	     
			    thread_iter_init();		     // different struct with iters' values.
			    for (i = 0; i < ngroup1; i++) 
					thread_make_iter(i, itgroup1);				
	     
			    for (; i < 1000; i++) 	    // loop threads group 2
					thread_make_iter(i, itgroup2);				
			    
	}     
		    else {
			    threads_create = nthreads;	     		// if -f option: create thread per conn
			    thread_info_init();				// allocates space for the struct
			    for (int i = 0; i < threads_create; i++) 
				thread_make(i);				
			}
		}
	     else {	// nthreads < 1000
			threads_create = nthreads;			
			if (input_file) {
				thread_iter_init();					
				itgroup1=10;
				for (int i = 0; i < threads_create; i++) 
					thread_make_iter(i, itgroup1);     		// second param. not used	
			}
			else {
			thread_info_init();
			for (int i = 0; i < threads_create; i++) 
				thread_make(i);				
			}
	     }
}
   // Main thread
    static char const spin_chars[] = "/-\\|"; // progress display:
    int r = 0;
    while (running_threads<threads_create) { 
	    putchar(spin_chars[r % sizeof(spin_chars)]);
	    fflush(stdout);
            usleep(500000); 	
            putchar('\r');	
	    r++;
    }
	    
	    
    printf(" \n %i simultaneous connections ready. Press any key to start sending data.\n\n", running_threads);
    cin.clear();
    cin.get();
     
    pthread_cond_broadcast(&clifd_cond);			// awakens all the threads
   
    const clock_t begin_time = clock();
    // boost::timer t; // start timing
    if (execution_mode==3) start_chrono_real(nthreads+1);
    else start_chrono_thread(nthreads+1);			 
    cout << " Starting... " << endl;
    
    // waiting for all the threads to finish their execution
    if ( (pthread_cond_wait(&thread_end_cond, &clifd_mutex)!=0 ) ) {	
	  cout<< "Error pthread_cond_wait ";
     }	
	 
     pthread_mutex_unlock(&clifd_mutex);
     
     // double elapsed_time = t.elapsed();// chrono stopped  
     printf("Execution completed!\n\n");
     
     if (conn_error > 0) { 		// if it happened any error, check server availability
			  printf(" %i connections failed!! Testing availability of server.....\n", conn_error);
	  
	  sockfd = socket(AF_INET, SOCK_STREAM, 0);		
	  if (sockfd < 0) 
		printf("-------------- Server NOT available at the moment!-------------\n\n");
	  else if (server == NULL) 
		 printf("-------------- Server NOT available at the moment!-------------\n\n");			 
		
	  else {
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, 	
	        (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		serv_addr.sin_port = htons(portno);
	   
		if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
			printf("-------------- Server NOT available at the moment!-------------\n\n");
		else printf("------ Server availability: OK\n");
		}  
		close(sockfd);
     }
     else printf("---------- All connections succesful!------------\n\n");
       
     if (ex_answer) { 
	  float percent = ((float) correct_answers/(float) total_threads)*100.0f;
	  printf ("·Number of correct answers: %i (%g %% of success)\n" , correct_answers , percent );
     }
     // printf ("·Elapsed time: %g s\n",  elapsed_time);	// time_t = %lu
     std::cout << "Elapsed time: " << (float)( clock() - begin_time ) /  CLOCKS_PER_SEC << endl;
     return 0;
     
     } catch (TCLAP::ArgException &e)  				// catch any exceptions
	{ std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }	
}
