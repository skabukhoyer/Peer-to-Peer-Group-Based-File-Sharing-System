# Peer-to-Peer Group Based File Sharing System

I need to build a group based file sharing system where users
can share, download files from the group they belong to. Download should be parallel
with multiple pieces from multiple peers.

## Pre-requisites:
socket programming, SHA1 hash, multithreading

## Installation
**Linux Operating System:-**
  
**Software Requirement:-** 
1. G++ Compiler
```bash
sudo apt-get install g++
```
2. openssl library
```python
sudo apt-get install openssl
```
## Compilation
1. compile  ```client.cpp ```
```python
g++ -pthread client.cpp -o client -lssl -lcrypto
```
2. compile  ```tracker.cpp ```
```python
g++ -pthread tracker.cpp -o tracker -lssl -lcrypto
```
## Usage

### Tracker
1. Run Tracker:-   
```./tracker tracker_info.txt tracker_no```  
tracker_info.txt - Contains ip,port details of all the trackers
 ```python
./tracker tracker_info.txt 1
``` 
2. Close Tracker: 
```python
quit
```
  

### Client
1. Run Client:   
```./client <IP>:<PORT> tracker_info.txt```
```python
./client 127.0.0.1:8000 tracker_info.txt
```

  
2. Create User Account:   
```python
create_user <user_id> <passwd>
```
3. Login:   
```python  
login <user_id> <passwd>
```
4. Create Group:   
```python  
create_group <group_id>
```
5. Join Group:   
```python  
join_group <group_id>
```
6. Leave Group:   
```python  
leave_group <group_id>
```
7. List pending join:   
```python  
list_requests <group_id>
```
8. Accept Group Joining Request:   
```python  
accept_request <group_id> <user_id>
```
9. List All Group In Network:   
```python  
list_groups
```
10. List All sharable Files In Group:   
```python  
list_files <group_id>
```
11. Upload File:   
```python  
upload_file <file_path> <group_id>
```
12. Download File:   
```python  
download_file <group_id> <file_name> <destination_path>
```
13. Logout:   
```python  
logout
```
14. Show_downloads:   
```python  
show_downloads
```
Output format:
  
[D] [grp_id] filename
  
[C] [grp_id] filename
  
D(Downloading), C(Complete)
  
15. Stop sharing:   
```python  
stop_share <group_id> <file_name>
```

## Assumptions
1. Only one tracker is implemented and that tracker should always be online.  
2. The peer can login from different IP addresses, but the details of his downloads/uploads will not be persistent across sessions.
      
3. File paths should be absolute.

