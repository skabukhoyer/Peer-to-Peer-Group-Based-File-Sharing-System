#include <unistd.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <thread>
#include <openssl/sha.h>
#include <fstream>
#include <bits/stdc++.h> 
#include <fcntl.h>
#include<sys/types.h>
#include <stdio.h>
#include <string.h>

using namespace std;

void error(char *msg){
	perror(msg);
	exit(EXIT_FAILURE);
}
string user_name;
unordered_map<string,string> fnameToPath;
unordered_map<string,string> fnameToGroup;
bool isLoggedIn;
int server_sock;
vector<string> downloading;
void log_out(){
	user_name = "";
	//fnameToPath.clear();
	isLoggedIn = 0;
}
void log_in(string str){
	user_name = str;
	isLoggedIn = 1;
}
void serving_peer_until(int client_socket){
	char buffer[2048];
	memset(buffer,0,2048*sizeof(char));
	int bytes_read; 
	if((bytes_read=read(client_socket,buffer,2048)) ==0){
		cout<<"No arguments passed from the peer\n";
	}
	vector<string> commands;
	char *token = strtok(buffer, " ");
	while(token != NULL){
		commands.push_back(token);
		token = strtok(NULL," ");
	}
	if(commands[0] == "share"){
		string request_file_path = fnameToPath[commands[1]];
		int chunk_no = stoi(commands[2]);
		int chunk_size = stoi(commands[3]);
		char * buffer = new char[chunk_size];
		memset(buffer,0,chunk_size*(sizeof(char)));
		int fd = open(request_file_path.c_str(),O_RDONLY);
		if(fd==-1){
			perror("Error in opening file\n");
		}
		int bytes_read = pread(fd, buffer, chunk_size, (chunk_no-1)*524288);
		
		//////
		/*unsigned char obuf[20];
	    size_t len = strlen(reinterpret_cast<const char*>(buffer));
	    SHA1(reinterpret_cast<const unsigned char*>(buffer), len, obuf);
	    int i;
	    char buff[40];
	    string str="";
	    for (i = 0; i < 20; i++) {
		 snprintf(buff, sizeof(buff),"%x",obuf[i]);
		 str +=string(buff);
	    }
	    cout<<chunk_no<<" calculated sha: "<<str<<endl;
	   */
		////
		write(client_socket, buffer, chunk_size);
		
		delete[] buffer;
		if(close(fd)== -1){
			perror("Error in closing file\n");
		}
		
	}
	else{ cout<<"Invalid command from the peer for downloading\n";}
	close(client_socket);
}
void peer_connection(string ip,string port){
	int sock;
	int option = 1;
	struct  sockaddr_in address;
	
	if((sock = socket(AF_INET,SOCK_STREAM,0)) == 0){
		cout<<"socket creation error"<<endl; return;
	}
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&option, sizeof(option))){ 
		cout<<"Error in setscokopt"<<endl; return;
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(stoi(port)); 
	if(bind(sock, (struct sockaddr *)&address,sizeof(address))<0){ 
		perror("bind error");
		exit(EXIT_FAILURE);
	}
	if(listen(sock, 20) < 0){ 
		perror("listen");
		exit(EXIT_FAILURE);
	}	
	int len = sizeof(address);
	int new_sock;
	while(1){
		if ((new_sock = accept(sock, (struct sockaddr *)&address,(socklen_t*)&len))<0){
			cout<<"Error in accepting"<<endl;
			continue; 
		}
		thread serv(serving_peer_until,new_sock);
		serv.join();

	}
	
	return ;

}
string command_send(int sock, string cmd){
	send(sock , cmd.c_str() , cmd.size() , 0 );
	char buffer[512000];
	memset(buffer,0,512000*(sizeof(char)));
	int reply = read(sock,buffer,512000);
	string rep = buffer;
	return rep;
}
long long findFileSize(const char f_path[]){
	FILE* fp = fopen(f_path, "r");   
	if (fp == NULL) { 
	    printf("File Not Exist in this path\n"); 
	    return -1; 
	}
	fseek(fp, 0L, SEEK_END); 
	long long sz = ftell(fp);   
	fclose(fp);   
	return sz; 
}
void download_chunk_func(string file_name,string sha1,int chunk_no,int chunk_size, string ip, string port,string dest){
	int peer_sock = 0;
	struct sockaddr_in server_addr; 
	if ((peer_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
		printf("\n Socket creation error \n"); 
		return ; 
	} 
	server_addr.sin_family = AF_INET; 
	server_addr.sin_port = htons(stoi(port.c_str())); 
	if(inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr)<=0) { 
		printf("\nInvalid IP address for the peer \n"); 
		return; 
	}
	if (connect(peer_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){ 
		printf("\nConnection Failed \n"); 
		return; 
	}
	string msg = "share "+file_name+" "+to_string(chunk_no) +" "+to_string(chunk_size);
	send(peer_sock,msg.c_str() ,msg.size(),0);
	cout<<"downloading chunk_no: "<<chunk_no<<" from peer: "<<ip<<":"<<port<<endl;
	char buffer[chunk_size];
	memset(buffer,0,chunk_size*(sizeof(char)));
	int offset = 0;
	while(1){
	    int n = read(peer_sock, buffer+offset, chunk_size);
	    if(n <= 0){
	      break;
	    }
	    offset += n;
	}
/////sha1 checking
	/*unsigned char obuf[20];
	    size_t len = strlen(reinterpret_cast<const char*>(buffer));
	    SHA1(reinterpret_cast<const unsigned char*>(buffer), len, obuf);
	    int i;
	    char buff[40];
	    string str="";
	    for (i = 0; i < 20; i++) {
		 snprintf(buff, sizeof(buff),"%x",obuf[i]);
		 str +=string(buff);
	    }
	    cout<<"calculated sha: "<<str<<endl;
	    cout<<"passed sha: "<<sha1<<endl;
	    
	    if(str != sha1){
		*df=1;
		return;
		
	}*/
	
	
	int fd = open(dest.c_str(), O_WRONLY, 00777);
		if(fd == -1){
		    perror("[error in open]\n");
		}
		int nw = pwrite(fd, &buffer, chunk_size, (chunk_no-1)*524288);

		if(nw == -1){
		    perror("[error in write]\n");
		}
		if(close(fd) == -1){
			perror("Error in closing\n");
		}
		
		return ;

}

void download_func(string file_name,string dest_path,long long file_size,vector<string> original_sha1,vector<pair<string,string>>ip_ports,int *flag){
	int no_of_peers = ip_ports.size();
	int m = file_size/524288;
	int no_of_chunks = m;
	int last_chunk_size = 524288;
	if(file_size%524288 != 0){
		no_of_chunks += 1;
		last_chunk_size = file_size%524288;
	}
	cout<<"Number of chunks need to be downloaded: "<<no_of_chunks<<endl;	
	cout<<"Number of peers available for this file: "<<no_of_peers<<endl;
	downloading.push_back(file_name);
	cout<<"Download in progress... "<<file_name<<endl;
	
	vector<thread> downloaded_chunks;
	int chunk_size = 524288;
	int fd = open(dest_path.c_str(),O_CREAT|O_WRONLY,00777);
	if(fd == -1){
		perror("File creation error\n");
	}
	if(close(fd) == -1){
		perror("File closing error\n");
	}
	//int df=0;
	if(no_of_chunks > no_of_peers){		
		int n = no_of_chunks/no_of_peers;
		int chunk_no = 1;
		for(int i=1;i<=no_of_peers;i++){
			//df =0;
			for(int j=0;j<n;j++){
				if( chunk_no == no_of_chunks){
					chunk_size = last_chunk_size;
				}
				
				downloaded_chunks.push_back(thread(download_chunk_func,file_name,original_sha1[chunk_no-1], chunk_no,chunk_size ,ip_ports[i-1].first,ip_ports[i-1].second,dest_path));
				
				chunk_no++;
			}
			//if(df==1){
			//	cout<<"Download Failed !"<<endl;
			//	break;
			//}
		}
		int r = no_of_chunks % no_of_peers;
		for(int i = 0; i<r;i++){
			//df=0;
			if( chunk_no == no_of_chunks){
				chunk_size = last_chunk_size;
			}
			downloaded_chunks.push_back(thread(download_chunk_func,file_name,original_sha1[chunk_no-1], chunk_no,chunk_size , ip_ports[i].first,ip_ports[i].second,dest_path));
			//if(df==1){
			//	cout<<"Download Failed !"<<endl;
			//	break;
			//}
			chunk_no++;			
		}

		while(downloaded_chunks.size() > 0){
			downloaded_chunks[0].join();
			downloaded_chunks.erase(downloaded_chunks.begin());
		}
		
	}
	else{
		for(int i = 1; i <= no_of_chunks; i++){
			//df=0;
			if( i == no_of_chunks){
				chunk_size = last_chunk_size;
			}
			downloaded_chunks.push_back(thread(download_chunk_func,file_name,original_sha1[i-1], i ,chunk_size,ip_ports[i-1].first,ip_ports[i-1].second,dest_path));
			//if(df==1){
			//	cout<<"Download Failed !"<<endl;
			//	break;
			//}
		}	
		while(downloaded_chunks.size() > 0){
			downloaded_chunks[0].join();
			downloaded_chunks.erase(downloaded_chunks.begin());
		}
	}
	//if(df==0){
		downloading.erase(find(downloading.begin(),downloading.end(),file_name));
		//fnameToPath[file_name] = dest_path;
		cout<<"Download Completed, file: "<<file_name<<endl;
		*flag=1;
	//}
	return;
}

int main(int argc, char *argv[])
{
	if(argc != 3){cout<<"Invalid Arguments passed!\n";return 0;}
	log_out();
	string client_ip, client_port;
	string server_ip, server_port;
	int index=0;
	for(;argv[1][index]!=':';++index){
		client_ip.push_back(argv[1][index]);
	}
	index++;
	while(argv[1][index] != '\0'){
		client_port.push_back(argv[1][index]); index++;
	}
	server_sock = 0; 
	struct sockaddr_in server_addr; 
	fstream file;
	file.open(argv[2]);
	file >> server_ip;
	file >> server_port;	
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
		cout<<"Error in socket creation"<<endl; return 0;
	} 
	server_addr.sin_family = AF_INET;
	int port= stoi(server_port.c_str());
	server_addr.sin_port = htons(port); 
	if(inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr)<=0) { 
		cout<<"Invalid adress found"<<endl; return 0;
	}
	if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){ 
		cout<<"Connection error"<<endl; return 0;
	}
	thread help_object(peer_connection,client_ip,client_port);  //for peer connecction as server
	
	while(1){
		cout<<">>";
		string command;
		getline(cin,command);
		vector<string> cmds;
		char *token = strtok(const_cast<char*>(command.c_str()), " ");
		while(token != NULL){
			cmds.push_back(token);
			token = strtok(NULL," ");
		}
		int len = cmds.size();
		if(len == 0)
		{	cout<<"please enter a valid command"<<endl; continue;}

		if(cmds[0] == "create_user" && len==3){
			string msg= "create_user "+cmds[1]+" "+cmds[2] ;
			string rev_msg = command_send(server_sock,msg);
			cout<<rev_msg<<endl;
		}
		else if(cmds[0] == "logout" && len == 1){
			if(!isLoggedIn){
				cout<<"you are not Logged in! "<<endl;
				continue;
			}
			string rev_msg = command_send(server_sock,"logout "+user_name);
			cout<<rev_msg<<endl;
			log_out();
		}
		else if(cmds[0] == "login" && len == 3){
			if(isLoggedIn){
				cout<<"you are already logged in!!!!"<<endl;
				continue;
			}
			string msg = "login "+cmds[1]+" "+cmds[2]+" "+client_ip+" "+client_port ;
			string rev_msg = command_send(server_sock,msg);
			if(rev_msg[0] == 'S'){
				log_out();
				log_in(cmds[1]);
				cout<<cmds[1]<<" logged in successfully "<<endl;
				vector<string> tokens;
				char *token = strtok(const_cast<char*>(rev_msg.c_str()), " ");
				while(token != NULL){
					tokens.push_back(token);
					token = strtok(NULL," ");
				}
				//for(int i=5;i<tokens.size();i+=2){
				//	fnameToPath[tokens[i]] = tokens[i+1];
				//}
			}
			else { cout<<rev_msg<<endl;}
		}
	
		else if(cmds[0] == "create_group" && len == 2){
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string msg = "create_group "+cmds[1]+" "+user_name ;
			string rev_msg = command_send(server_sock,msg);
			cout<<rev_msg<<endl;
		}
		else if(cmds[0] == "join_group" && len == 2){
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string msg = "join_group "+cmds[1]+" "+user_name ;
			string rev_msg = command_send(server_sock,msg);
			cout<<rev_msg<<endl;			
		}
		else if(cmds[0] == "leave_group" && len == 2){
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string msg = "leave_group "+cmds[1]+" "+user_name ;
			string rev_msg = command_send(server_sock,msg);
			cout<<rev_msg<<endl;			
		}
		else if(cmds[0] == "list_requests" && len == 2){
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string msg = "list_requests "+cmds[1]+" "+user_name ;
			string rev_msg = command_send(server_sock,msg);
			cout<<rev_msg<<endl;			
		}
		
		else if(cmds[0] == "accept_request" && len == 3){
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string msg = "accept_request "+cmds[1]+" "+cmds[2]+" "+user_name ;
			string rev_msg = command_send(server_sock,msg);
			cout<<rev_msg<<endl;			
		}
		else if(cmds[0] == "list_groups" && len == 1){
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string rev_msg = command_send(server_sock,"list_groups");
			cout<<rev_msg<<endl;						
		}
		else if(cmds[0] == "list_files" && len == 2){
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string msg = "list_files "+cmds[1]+" "+user_name;
			string rev_msg = command_send(server_sock,msg);
			cout<<rev_msg<<endl;						
		}
	
	 	else if(cmds[0] == "show_downloads" && len == 1){
			
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			for(auto d: downloading){
				cout<<"[D] "<<"["<<fnameToGroup[d]<<"] "<<d<<endl;
			}
			for(auto it = fnameToPath.begin();it != fnameToPath.end();it++){
				cout<<"[C] "<<"["<<fnameToGroup[it->first]<<"] "<<it->first<<endl;
			}
			
		}
		else if(cmds[0] == "upload_file" && len==3){			
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string file_path = cmds[1];
			long long file_size = findFileSize(file_path.c_str());
			if(file_size != -1){
				size_t chunk_size = 524288; //512KB
				cout<<"File size = "<<file_size<<endl;
				int n  = file_size/524288 ;
				int rem = file_size%524288 ;
				int no_of_chunks = n;
				size_t last_chunk_size = 524288;
				if(rem != 0){
					no_of_chunks += 1;
					last_chunk_size = rem;
				}
				//cout<<"Each chunk size = "<<chunk_size<<endl;				
				string file_name;
				int m = file_path.size();
				int i;
				for(i=m-1;file_path[i] != '/'; i--);
				i++;
				while(i<m){ file_name.push_back(file_path[i]); i++; }
				fnameToPath[file_name] = file_path;
				vector<string> sha1(no_of_chunks);
				//ifstream fin;
				//fin.open(file_path);
				///
				//char * buffer = new char[chunk_size];
		//memset(buffer,0,chunk_size*(sizeof(char)));
		int fd = open(file_path.c_str(),O_RDONLY);
		if(fd==-1){
			perror("Error in opening file\n");
		}

		
				///
				int j = 1;
				while(j<=no_of_chunks){
					if(j == no_of_chunks){ chunk_size = last_chunk_size ;}
					char* chunk = new char[chunk_size];
					memset(chunk,0,chunk_size*(sizeof(char)));
					int bytes_read = pread(fd, chunk, chunk_size, (j-1)*524288);
					//fin.read(chunk,chunk_size);
					cout<<"chunk no= "<<j<<" chunk_size= "<<chunk_size<<endl;
					unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
					SHA1(reinterpret_cast<const unsigned char *>(chunk), chunk_size, hash);
					//string c_hash(reinterpret_cast<char*>(hash));
					/////

					    char buffer[40];
					    string str="";
					    for (int i = 0; i < 20; i++) {
						 snprintf(buffer, sizeof(buffer),"%x",hash[i]);
						 str +=string(buffer);
					    }
					////
					sha1[j-1] = str;
					delete[] chunk;
					j++;
				}
				if(close(fd)== -1){
			perror("Error in closing file\n");
		}
				string msg="upload_file "+cmds[2]+" "+user_name+" "+file_name+" "+file_path+" "+to_string(file_size)+" ";
				for(int j=0;j<no_of_chunks;j++){
					msg += sha1[j] + " " ;
					//cout<<sha1[j]<<endl;
				}

				cout<<command_send(server_sock,msg)<<endl;
				fnameToGroup[file_name] = cmds[2];
			}
		}
		else if(cmds[0] == "download_file" && len == 4){
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string group_id=cmds[1],file_name = cmds[2], dest_path = cmds[3];
			string msg= "download_file "+file_name+" "+group_id+" "+user_name ;
			string peers = command_send(server_sock,msg);						
			if(peers[0] == 'Y'){
				cout<<"You are not a member of the group, "<<group_id<<endl;
				continue;
			}				
			if(peers[0] == 'R'){
				cout<<file_name<<" file is not present in this group, "<<group_id<<endl;
				continue;
			}
			vector<pair<string,string>> peerIpPorts;
			vector<string> original_sha1;
			vector<string> tokens;
			char *token = strtok(const_cast<char*>(peers.c_str()), " ");
			while(token != NULL){
				tokens.push_back(token);
				token = strtok(NULL," ");
			}
			
			long long file_size = stoi(tokens[0]);
			int index = 1;
			while(tokens[index] != "end"){
				original_sha1.push_back(tokens[index++]);
			}
			//cout<<"No of sha1 chunk: "<<index-1<<endl;
			index++;
			for(;index<tokens.size();index+=2){
				peerIpPorts.push_back(make_pair(tokens[index],tokens[index+1]));
			}
			if(peerIpPorts.size() == 0){
				cout<<file_name<<" file is not sharing any peer"<<endl;
				continue;
			}
			cout<<"Total size: "<<file_size<<endl;
			cout<<"Peer IP Port which are sharing this file :"<<endl;
			for(auto s:peerIpPorts){
				cout<<s.first<<" "<<s.second<<endl;
			}
			fnameToGroup[file_name] = group_id;
			int flag=0;
			thread th(download_func,file_name,dest_path,file_size,original_sha1,peerIpPorts,&flag);
			th.join();
			msg="fail ";
			if(flag==1){
				msg="done "+dest_path;
				fnameToPath[file_name] = dest_path;
				
				
			}
			else{
				auto itr = fnameToGroup.find(file_name);
				fnameToGroup.erase(itr);
			}
			send(server_sock , msg.c_str() , msg.size() , 0 );
		}
		else if(cmds[0] == "stop_share" && len == 3){		
			if(!isLoggedIn){
				cout<<"you are not Logged in yet!"<<endl;
				continue;
			}
			string msg = "stop_share "+cmds[1]+" "+cmds[2]+" "+user_name ;
			string rev_msg = command_send(server_sock,msg);
			cout<<rev_msg<<endl;	
		}

		else{
			cout<<"Invalid Command Entered!!!\n";
		}
	}
	help_object.join();
	return 0;
	
	
}
