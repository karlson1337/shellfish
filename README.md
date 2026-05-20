# shellfish
A *nix shell built for fun (using C++)

## Features:
- Command history
- Pipes
- Basic IO redirection (<, >, >>)
- Basic script parsing (just a command list for now)

## Requirements
- Linux x86_64
- readline library (`sudo apt install libreadline-dev` or similar)

## Run
- `chmod +x shellfish`
- `./shellfish`

## Build from source
```
clang++ -std=c++17 -O2 -o shellfish shellfish.cpp -lreadline
```
