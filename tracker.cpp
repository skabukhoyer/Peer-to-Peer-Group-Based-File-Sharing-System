#include<unistd.h>
#include<sys/types.h>
#include<thread>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h> 
#include<iostream>
#include<vector>
#include<string.h>
#include<string>
#include<unordered_map>
#include<utility>
#include<iterator>
#include<stdlib.h>
#include<algorithm>
#include<fstream>
using namespace std;

void error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

class client
{
public:
	string username,password;
	string client_ip,client_port;
	
	bool isLoggedIn;
	unordered_map<string,string> fnameToPath;
	//unordered_map<string,vector<bool>> fnameToBitmap;
	client(string name,string pw){
		username = name;
		password = pw;
		isLoggedIn = 0;
	}
	void login(string ip,string port){
		client_ip = ip;
		client_port = port;
		isLoggedIn = 1;
	}
	void logout(){
		isLoggedIn = 0;
		client_ip = "";
		client_port = "";
		
	}
};
class file
{
public:
	string file_name;
	long long file_size;
	//int no_of_chunk;
	vector<pair<string,bool>> usersFiles;	
	vector<string> sha1;
	
	file(string name, long long f_size){
		file_name = name;
		file_size = f_size;
	}
	void addUser(string s){
		usersFiles.push_back(make_pair(s,1));
	}
	bool isUser(string uname){
		for(auto s: usersFiles){
			if(s.first == uname){ return true; }
		}
		return false;
	}
};

class group
{
public:
	string groupID;
	string group_owner;
	vector<string> members;
	vector<string>filesinGroup;
	vector<string>pendingRequests;
	
	group(string id, string name){
		groupID = id;
		group_owner = name;
		members.push_back(name);
	}
	bool isGroupMember(string s){
		for(int i=0;i<members.size();++i){
			if(members[i] == s){ return true; }
		}
		return false;
	}
	bool isFileInGroup(string s){
		for(int i=0;i<filesinGroup.size(); i++){
			if(filesinGroup[i] == s){ return true; }
		}
		return false;
	}
	bool isUserInPendingRequests(string s){
		for(int i=0;i<pendingRequests.size(); i++){
			if(pendingRequests[i] == s){ return true; }
		}
		return false;
	}
	void acceptRequest(string s){
		pendingRequests.erase(find(pendingRequests.begin(),pendingRequests.end(),s));
		members.push_back(s);
	}
	void removeMember(string s){
		members.erase(find(members.begin(),members.end(),s));
	}
	
};

unordered_map<string,client*> users;
unordered_map<string,group*> groups;		
unordered_map<string,file*> files;	
bool isUserExists(string str){
	if(users.find(str) == users.end()){ return false; }
	return true;
}
bool isGroupExists(string str){
	if(groups.find(str) == groups.end()) { return false ;}
	return true;
}

bool isFileExists(string str){
	if(files.find(str) == files.end()) { return false ;}
	return true;
}

void client_func(int client_fd){
	while(1){
		char buffer[512000];
		memset(buffer,0,512000*(sizeof(char)));
		
		int bytes_read = read(client_fd,buffer,512000);
		if(bytes_read == 0){
			cout<<"No byte has been passed, in socket: "<<client_fd<<endl;
			return;
		}
		cout<<"command received from socket "<<client_fd<<": "<<buffer<<endl;

		char *token = strtok(buffer, " ");
		vector<string> commands;
		while(token != NULL){
			commands.push_back(token);
			token = strtok(NULL," ");
		}
		memset(buffer,0,512000*(sizeof(char)));
		
		if(commands[0] == "create_user"){
			if(isUserExists(commands[1])){
				char msg[] = "User already exists at this ID";
				send(client_fd, msg , strlen(msg) , 0 );
			}
			else if(commands.size() != 3){
				char msg[] = "Inavlid Argument";
				send(client_fd, msg , strlen(msg) , 0 );
			}
			else{
				client* peer = new client(commands[1],commands[2]);
				users[commands[1]] = peer;
				string msg = "Registered ID number: "+commands[1]+" Successfully!";
				send(client_fd , msg.c_str() , msg.size() , 0 );
				cout<<"New user rgeistered ID: "<<commands[1]<<endl;
			}
		}
		if(commands[0] == "login"){
			
			if(isUserExists(commands[1])){
				if(users[commands[1]]->password != commands[2]){
					string msg = "Incorrect password for ID: "+commands[1];
					send(client_fd, msg.c_str() , msg.size() , 0 );
				}
				else{
					users[commands[1]]->login(commands[3],commands[4]);
					string msg = "Successfully "+commands[1]+" user Id Loggedin! \n";
					//int flag=0;	
					vector<pair<string,bool>> temp;
					for(auto it = users[commands[1]]->fnameToPath.begin(); it != users[commands[1]]->fnameToPath.end(); it++){
						msg += it->first+" "+it->second+"\n";
						
						temp = files[it->first]->usersFiles;
						for(auto itr: temp){
							if(itr.first == commands[1]){
								itr.second = true;
								break;
							}
						}
						
						temp.clear();
					}
									
					send(client_fd, msg.c_str() ,msg.size() , 0 );
				}
			}
			else{
				string msg = commands[1] + " User ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );
			}
		}
		if(commands[0] == "logout"){
			if(isUserExists(commands[1])){
				users[commands[1]]->logout();
				char msg[] = "Logged Out successfully";
				vector<pair<string,bool>> temp;
				for(auto it = users[commands[1]]->fnameToPath.begin(); it != users[commands[1]]->fnameToPath.end(); it++){
						
						temp = files[it->first]->usersFiles;
						for(auto itr: temp){
							if(itr.first == commands[1]){
								itr.second = false;
								break;
							}
						}
						temp.clear();
				}
				send(client_fd, msg , strlen(msg) , 0 );
				
			}
			else{
				string msg = commands[1] + " User ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );
			}
		}
		if(commands[0] == "create_group"){
			if(!isUserExists(commands[2])){
				string msg = commands[2] + " User ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );
			}
			else if(isGroupExists(commands[1])){
				char msg[] = "This Group ID already exists";
				send(client_fd, msg , strlen(msg) , 0 );				
			}
			else{
				group * new_gp = new group(commands[1],commands[2]);
				groups[commands[1]] = new_gp;				
				string msg = "Group created successfully , ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
		}
		if(commands[0] == "join_group"){
			if(!isUserExists(commands[2])){
				string msg = commands[2] + " User ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );
			}
			else if(!isGroupExists(commands[1])){
				string msg = commands[1] + " group ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );			
			}
			else if(groups[commands[1]]->isGroupMember(commands[2])){
				string msg = "You are already a member of this group ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
			else{
				groups[commands[1]]->pendingRequests.push_back(commands[2]);
				string msg = "Join group request is sent, for Group ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );			
			}
		}
		if(commands[0] == "leave_group"){
			if(!isUserExists(commands[2])){
				string msg = commands[2] + " User ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );
			}
			else if(!isGroupExists(commands[1])){
				string msg = commands[1] + " group ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );			
			}
			else if(!groups[commands[1]]->isGroupMember(commands[2])){
				string msg = "Sorry! You are not a member of this group ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
			else{
				groups[commands[1]]->removeMember(commands[2]);
				string msg = "You left the group, ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );			
			}
		}
		if(commands[0] == "list_requests"){
			if(!isUserExists(commands[2])){
				string msg = commands[2] + " User ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );
			}
			else if(!isGroupExists(commands[1])){
				string msg = commands[1] + " group ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );			
			}
			else if(groups[commands[1]]->group_owner != commands[2]){
				string msg = "You are not the owner of this group ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
			else{
				string msg = "";
				vector<string> pen;
				pen= groups[commands[1]]->pendingRequests;
				for(int i=0;i< pen.size(); ++i){ msg += pen[i]+"\n"; }
				if(msg == "")
					msg = "No pending requests in this group Id: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
		}
		if(commands[0] == "accept_request"){
			if(!isUserExists(commands[2])){
				string msg = commands[2] + " User ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );
			}
			else if(!isGroupExists(commands[1])){
				string msg = commands[1] + " group ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );			
			}
			else if(groups[commands[1]]->group_owner != commands[3]){
				string msg = "You are not the owner of this group ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
			else if(!groups[commands[1]]->isUserInPendingRequests(commands[2])){
				string msg = "No pending request found for this user ID: "+commands[2];
				send(client_fd, msg.c_str() , msg.size() , 0 );					
			}
			else{
				groups[commands[1]]->acceptRequest(commands[2]);
				string msg = "Request Accepted,for user ID: "+commands[2];
				send(client_fd, msg.c_str() , msg.size() , 0 );	
			}
		}
		if(commands[0] == "list_groups"){
			string msg = "Groups in the network:";
			
			for(auto it = groups.begin(); it != groups.end();it++){
				msg += "\n"+it->first;
			}
			if(msg == "")
				msg = "No Groups is created till now ";
			send(client_fd, msg.c_str() , msg.size() , 0 );
		}
		if(commands[0] == "list_files"){
			if(!isGroupExists(commands[1])){
				string msg = commands[1] + " group ID doesn't exist";
				send(client_fd, msg.c_str() , msg.size() , 0 );			
			}
			else if(!groups[commands[1]]->isGroupMember(commands[2])){
				string msg = "Sorry! You are not a member of this group ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
			else{
				string msg = "";
				for(string s: groups[commands[1]]->filesinGroup){
					for(auto user:files[s]->usersFiles){
						if(user.second == 1)
						{
							msg += s+"\n";
							break; 
						}
					}
				}
				if(msg == "")
					msg = "No files present in this group ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
		}
		if(commands[0] == "upload_file"){
			if(!isUserExists(commands[2])){
				string msg = commands[2] + " User ID doesn't exist\n";
				send(client_fd, msg.c_str() , msg.size() , 0 );
			}
			else if(!isGroupExists(commands[1])){
				string msg = commands[1] + " group ID doesn't exist\n";
				send(client_fd, msg.c_str() , msg.size() , 0 );			
			}
			else{
				if(!groups[commands[1]]->isFileInGroup(commands[3])){
					groups[commands[1]]->filesinGroup.push_back(commands[3]);
				}
				if(files.find(commands[3]) != files.end()){
					if(files[commands[3]]->isUser(commands[2])){
						string msg=commands[3] +" file already has been uploaded by user ID: "+commands[2];
						send(client_fd, msg.c_str() , msg.size() , 0 );	
					}
					else{
						files[commands[3]]->addUser(commands[2]);
						users[commands[2]]->fnameToPath[commands[3]] = commands[4];
						string msg = commands[3]+" file uploaded successfully by user ID: "+commands[2];
						send(client_fd, msg.c_str() , msg.size() , 0 );						
					}		
				}
				else{
					file* new_file = new file(commands[3],stoi(commands[5]));
					new_file->addUser(commands[2]);
					files[commands[3]] = new_file;
					users[commands[2]]->fnameToPath[commands[3]] = commands[4];
					for(int i=6;i<commands.size();i++){
						new_file->sha1.push_back(commands[i]);           //SHA storing in  vector of string for each chunk
					}
					
					string msg = commands[3]+" file uploaded successfully by user ID: "+commands[2];
					send(client_fd, msg.c_str() , msg.size() , 0 );
				}
			}
		}
		if(commands[0] == "download_file"){
			string file_search = commands[1];
			string file_grp = commands[2];
			string user_id= commands[3];
			if(!groups[file_grp]->isGroupMember(user_id)){
				char msg[] = "You are not a Member of this group";
				send(client_fd, msg , strlen(msg) , 0 );
				continue;				
			}
			if(!groups[file_grp]->isFileInGroup(file_search)){
				string msg = file_search;
				msg += " file is not present in this group";
				send(client_fd, msg.c_str(), msg.size(), 0);
				continue;
			}
			string msg=to_string(files[file_search]->file_size)+" ";
			vector<string> it;
			it=files[file_search]->sha1 ;
			for(int i=0;i<it.size();++i){
				msg += it[i]+" ";
			}
			msg += "end ";
			
			for(auto share_user: files[file_search]->usersFiles){
				if(users[share_user.first]->isLoggedIn && share_user.second){	
					msg += users[share_user.first]->client_ip+" "+users[share_user.first]->client_port+" ";
				}
			}
			send(client_fd, msg.c_str(), msg.size(), 0);
			
			memset(buffer,0,512000*(sizeof(char)));
			int bytes_read = read(client_fd,buffer,512000);
			if(bytes_read == 0){
				cout<<"No byte has been passed, in socket: "<<client_fd<<endl;
				return;
			}
			char *token = strtok(buffer, " ");
			vector<string> com;
			while(token != NULL){
				com.push_back(token);
				token = strtok(NULL," ");
			}
			memset(buffer,0,512000*(sizeof(char)));
			if(com[0] == "done"){
				files[file_search]->addUser(user_id);
				users[user_id]->fnameToPath[file_search] = com[1];
			}
			
		}
		if(commands[0] == "stop_share"){
			if(!isFileExists(commands[2])){
				string msg = commands[2]+" file does not exist\n";
				send(client_fd, msg.c_str(), msg.size(), 0);
			}
			else if(!isGroupExists(commands[1])){
				string msg = commands[1]+" group ID doesn't exist";
				send(client_fd, msg.c_str(), msg.size(), 0);			
			}
			else if(!groups[commands[1]]->isGroupMember(commands[3])){
				string msg = "Sorry! You are not a member of this group ID: "+commands[1];
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}
			else{
				for(auto uf :files[commands[2]]->usersFiles){
					if(uf.first == commands[3]){
						uf.second = false;
						break;
					}
				}
				string msg = commands[2]+" file has stopped sharing by user ID: "+commands[3];
				
				send(client_fd, msg.c_str() , msg.size() , 0 );				
			}			

		}
		
	}
		


}
void exit_func(int i){
	string inp;
	while(1){
		getline(cin,inp);
		if(inp == "quit"){
			exit(0);
		}
	}
}
int main(int argc, char *argv[])
{
	if(argc != 3){ cout<<"Invalid argument passed"<<endl; return 0; }
	int tracker_no = stoi(argv[2]);
	fstream fp;
	fp.open(argv[1]);
	string server_ip, server_port;
	fp>>server_ip>>server_port;
	int server_sock;
	struct sockaddr_in server_address;
	server_sock = socket(AF_INET,SOCK_STREAM,0);
	if(server_sock == 0){ cout<<"Error on creating socket\n"; return 0; }
	int option=1;
	if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&option, sizeof(option)) != 0)
	{ cout<<"error in setsockopt"<<endl; return 0; }
	
	server_address.sin_family = AF_INET; 
	server_address.sin_addr.s_addr = INADDR_ANY; 
	int port= stoi(server_port);
	server_address.sin_port = htons(port); 
	
	if(bind(server_sock,(struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{ cout<<"Error in binding "<<endl; return 0;}
	
	if(listen(server_sock,20) < 0)
	{ cout<<"Error in listening socket"<<endl ; return 0;}
	int new_sock;
	vector<thread> clients;
	int len = sizeof(server_address) ;
	thread exit_thread(exit_func,1);
	while(1){
		if((new_sock = accept(server_sock, (struct sockaddr *)&server_address,(socklen_t*)&len))<0)
		{ 
			cout<<"Error in accepting"<<endl;
			continue;
		}
		cout<<"the new client at socket: "<<new_sock<<endl;
		clients.push_back(thread(client_func,new_sock));
	}
	for(int i=0;i<clients.size(); i++){
		clients[i].join();
	}
	
	return 0;
	
}
			
