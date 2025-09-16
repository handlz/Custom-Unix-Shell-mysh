# mysh — Custom Unix Shell

`mysh` is a **custom Unix-style shell** implemented in C for the University of Toronto CSC209 course.  
It extends traditional shell functionality with **built-in commands, variable management, pipelines, background processes, and TCP networking features**.

This project strengthened my skills in **systems programming, process control, signals, socket programming, and memory management**.

---

## 🚀 Features

### Core Shell Functionality
- Command execution with support for standard Linux utilities  
- Pipelines (`|`) to connect processes together  
- Background execution (`&`) with job tracking (`ps`, job IDs, cleanup)  
- Signal handling for `Ctrl+C` and child process completion  
- Variable assignment & expansion (e.g., `X=5` then `echo $X`)  

### Built-in Commands
- `echo` — Print text or variable-expanded values  
- `ls` — Directory listing with support for recursion, depth limiting, and substring filtering  
- `cd` — Change directories (`...` and `....` supported as shortcuts)  
- `cat` — Print file contents or stdin  
- `wc` — Count words, characters, and newlines  
- `kill` — Send signals to processes (supports variable expansion)  
- `ps` — Display active background jobs  

#### Networking Builtins
- `start-server <port>` — Launch a TCP server  
- `close-server` — Shut down server  
- `send <port> <host> <msg>` — Send a message to a server  
- `start-client <port> <host>` — Start interactive TCP client  

### Networking Support
- TCP server with multiple client support  
- TCP client for interactive communication  
- Message broadcasting across connected clients  
- Server activity checks integrated into shell loop  

### Variable System
- Linked-list–based environment variable storage  
- Safe assignment, retrieval, and expansion with `$VAR` syntax  
- Automatic cleanup on exit  

---

## 📂 Project Structure
├── mysh.c # Main shell loop (prompt, parsing, job control, signals)

├── builtins.c/.h # Built-in commands (echo, ls, cd, cat, wc, kill, ps, networking)

├── commands.c/.h # Variable assignment & expansion

├── variables.c/.h # Linked-list environment variable system

├── io_helpers.c/.h # Input/output handling & tokenization

├── network.c/.h # TCP server/client support

├── Makefile # Build automation


---

## ⚡ Usage

### 1. Build
```bash
make

2. Run
./mysh

3. Example Commands
mysh$ echo Hello World
mysh$ X=10
mysh$ echo Value of X is $X
mysh$ ls --rec --d 2 src
mysh$ cat file.txt | wc
mysh$ sleep 10 &
mysh$ ps
mysh$ start-server 8080
mysh$ start-client 8080 localhost

### 🛠️ Technical Highlights

Process control with fork(), execvp(), waitpid()

Pipelines via dup2() and file descriptors

Signal handling with sigaction() (SIGINT, SIGCHLD)

Networking with socket(), bind(), listen(), accept(), select()

Memory management with malloc, free, and linked lists

Robust error handling for invalid inputs, paths, or socket failures

### 📚 What I Learned

How shells parse and execute commands

Managing pipes, background jobs, and signals safely

Building networked applications in C using sockets

Designing a modular system with reusable components

Debugging complex multi-process and multi-client systems

### 🔮 Future Improvements

Add I/O redirection (>, <, >>)

Enhance networking with chat history & private messaging

Persistent variable storage across sessions

Add testing framework for builtins and networking layer
