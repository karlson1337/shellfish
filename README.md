# 🐚 shellfish

A lightweight Unix shell written in C++ with pipeline support, I/O redirection, and persistent history.

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square) ![Platform](https://img.shields.io/badge/platform-Linux-lightgrey?style=flat-square)

---

## Features

- **Pipelines** — chain commands with `|`
- **I/O redirection** — `>`, `>>`, `<`
- **Environment variable expansion** — `$VAR` in arguments
- **Persistent history** — saved to `~/.shellfish_history` across sessions (powered by GNU readline)
- **`cd` with directory stack** — `cd -` returns to the previous directory, `cd` / `cd ~` goes home
- **Script execution** — pass a script file as an argument to run it non-interactively
- **`exit`** — saves history and exits cleanly
- **`SIGINT` handling** — Ctrl+C interrupts the current command without killing the shell

---

## Build

**Install dependencies:** `clang++` and `libreadline`

Note: can also use `g++` or other compilers instead of `clang++`

```bash
# Ubuntu / Debian
sudo apt install libreadline-dev

# Arch
sudo pacman -S readline
```
 
**Build:** 
```bash
clang++ -std=c++17 -O2 -o shellfish shellfish.cpp -lreadline
```

or

```bash
g++ -std=c++17 -O2 -o shellfish shellfish.cpp -lreadline
```

---

## Usage

### Interactive mode
```bash
./shellfish
```

```
user@host:user$ echo hello | tr a-z A-Z > output.txt
user@host:user$ cat < output.txt
HELLO
user@host:user$ cd /tmp
user@host:tmp$ cd -
user@host:user$
```

### Script mode
```bash
./shellfish script.sh
```

Lines in the script are executed sequentially. Pipelines and redirection work the same as in interactive mode.

---

## Built-in Commands

| Command | Description |
|---------|-------------|
| `cd [dir]` | Change directory (`~` or no arg → `$HOME`, `-` → previous dir) |
| `exit` | Save history and exit |

All other commands are executed via `execvp`.

---

## History

Command history is stored at `~/.shellfish_history` and loaded automatically on startup. History is written on `exit` or EOF (Ctrl+D).

---

## Limitations

- No job control (`&`, `fg`, `bg`)
- No quoting / glob expansion
- Single-level pipeline only (no subshells)
- No `&&` / `||` operators

---