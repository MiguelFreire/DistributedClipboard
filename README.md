# DistributedClipboard

Distributed Clipboard is a project made for Systems' Programming Course for Spring Semester 17/18 of MEEC - IST. It uses UNIX sockets for internal communication and TCP Sockets for internet communication, uses Google's protobuffers for message serialization and linux syscalls for thread creation, syncronization and memory management. It allows you to have a network of syncronized clipboards.

### Prerequisites

 * GCC 4.2.x
 * Make
 * Google Protobuffers: https://github.com/google/protobuf
 * Protobuffers C Wrapper: https://github.com/protobuf-c/protobuf-c

### Installing

``` 
git clone 
make install
make
```

## Examples
### Single Mode
```
./clipboard 
# a socket will be created with the name SOCKET_CLIPBOARD, you can change the name in clipboard_protocol.h
# a port will be printed in STDIO
# you can now connect using the api or any other clipboard
```

### Connected Mode
Connected mode allows you to connect to remote clipboard and stay in sync with them
```
./clipboard -c <ip> <port> #grab the port returned by ./clipboard
# you can now connect with the api and all clipboard will be sync
```

### API

#### Connect
```c
int clipboard_connect(char *clipboard_dir)
```
Connects you to a local clipboard

|               | Type  | Desc                                                                            |
|---------------|-------|---------------------------------------------------------------------------------|
| clipboard_dir | char* | Directory where the clipboard was executed                                      |
| return        | int   | Clipboard_id (positive integer) if the connection was successfully. 0 otherwise |

#### Copy
Copies the data pointed by <buf> to <region> up to a <count> of bytes

```c
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count)
```

|              | Type   | Desc                                       |
|--------------|--------|--------------------------------------------|
| clipboard_id | int    | Clipboard_id return from clipboard_connect |
| region       | int    | Number of the region to transfer data to   |
| buf          | void*  | Pointer to the data you want to transfer   |
| count        | size_t | Size of the data                           |
| return       | int    | Number of bytes copied to region. 0 in case of error           |

#### Paste
Pastes data from the clipboard <region> to <buf> up to a <count> of bytes


```c
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count)
```

|              | Type   | Desc                                       |
|--------------|--------|--------------------------------------------|
| clipboard_id | int    | Clipboard_id return from clipboard_connect |
| region       | int    | Number of the region to transfer data from |
| buf          | void*  | Pointer to the data where you want to save the data|
| count        | size_t | Size of the data                           |
| return       | int    | Number of bytes copied from clipboard region. 0 in case of error           |

#### Wait
Waits for a copy in <region> then pastes it to <buf> up to a <count> of bytes

```c
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count)
```

|              | Type   | Desc                                       |
|--------------|--------|--------------------------------------------|
| clipboard_id | int    | Clipboard_id return from clipboard_connect |
| region       | int    | Number of the region wait for              |
| buf          | void*  | Pointer to the data you want to transfer   |
| count        | size_t | Size of the data                           |
| return       | int    | Number of bytes copied from region. 0 in case of error           |

#### Close
Closes the connection to the local clipboard

```c
int clipboard_close(int clipboard_id)
```

|               | Type  | Desc                                                            |
|---------------|-------|-----------------------------------------------------------------|
| clipboard_id  | int   | Clipboard_id return from clipboard_connect                      |
| return        | int   | 1 in case of success. 0 otherwise |
## Authors

* **Micaela Serôdio** - 84139 MEEC - micaela.serodio@tecnico.ulisboa.pt
* **Miguel Freire** - 84145 MEEC - miguel.freire@tecnico.ulisboa.pt

