# 🐚 mysh — Custom Unix Shell

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

