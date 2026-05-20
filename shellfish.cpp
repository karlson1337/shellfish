#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include<limits.h>
#include <readline/readline.h>
#include <readline/history.h>

char cwd[PATH_MAX];
char prev_dir[PATH_MAX];
char history_path[PATH_MAX];

void io_redir(std::vector<char*>& args)
{
    for(int i = 0; i+1 < args.size(); i++)
    {
        if((strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0) && args[i+1]) //args[i+1] for null check
        {
            int fd;
            if(strcmp(args[i],">") == 0)
                fd = open(args[i+1], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
            else if(strcmp(args[i],">>") == 0)
                fd = open(args[i+1], O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
            if(fd == -1) exit(1);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args.erase(args.begin() + i);
            args.erase(args.begin() + i);
            i--;
        }
        if((strcmp(args[i], "<") == 0) && args[i+1]) //args[i+1] for null check
        {
            int fd;
            fd = open(args[i+1], O_RDONLY);
            if(fd == -1) exit(1);
            dup2(fd, STDIN_FILENO);
            close(fd);
            args.erase(args.begin() + i);
            args.erase(args.begin() + i);
            i--;
        }
    }
}

int builtins(std::vector<char*>& args)
{
    if (strcmp(args[0], "exit") == 0) 
    {
        write_history(history_path);
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0)
    {
        if(!args[1] || strcmp(args[1], "~") == 0) 
        {
            strcpy(prev_dir, cwd);
            chdir(getenv("HOME"));
            getcwd(cwd, sizeof(cwd));
            return 0;
        }
        if(strcmp(args[1], "-") == 0) 
        {
            char temp[PATH_MAX];
            strcpy(temp, prev_dir);
            strcpy(prev_dir, cwd);
            chdir(temp);
            getcwd(cwd, sizeof(cwd));
            return 0;
        }
        strcpy(prev_dir, cwd);
        chdir(args[1] ? args[1] : getenv("HOME")); 
        getcwd(cwd, sizeof(cwd));

        return 0;
    }

    if(strcmp(args[0], "echo") == 0)
    {
        if(!args[1]) {return 0;}
        if (args[1] && args[1][0] == '$') 
        { 
            const char *val = getenv(args[1] + 1);
            std::cout << (val ? val : "") << std::endl;  
            return 0; 
        }
        else for (int i = 1; args[i]; i++) std::cout << args[i] << (args[i+1] ? " " : "\n");
        return 0;
    }
    return 1;
}

int run_cmd(std::vector<char*>& args)
{
    if(!builtins(args)) return 0;

    int child = fork();
    if(child < 0) 
    {
        perror("fork");
        return -1;
    }
    else if(child == 0)
    {
        signal(SIGINT, SIG_DFL);
        io_redir(args);
        int r = execvp(args[0], args.data());
        perror(args[0]);
        exit(1);
    }
    else
    {
        int rc = wait(NULL);
        return 0;
    }
}

int run_pipeline(std::vector<std::vector<std::string>>& split_cmds) {
    int n = split_cmds.size();
    int prev_fd = -1;
    for (int i = 0; i < n; i++) {
        int m = split_cmds[i].size();
        std::vector<char*> args(m + 1);
        for (int j = 0; j < m; j++) args[j] = const_cast<char*>(split_cmds[i][j].c_str());
        args[m] = nullptr;

        int fd[2];
        if (i < n - 1) pipe(fd);
        int child = fork();
        if(child < 0)
        {
            perror("fork");
            return -1;
        }
        else if (child == 0) 
        {
            signal(SIGINT, SIG_DFL);

            if (prev_fd != -1) 
            { 
                dup2(prev_fd, STDIN_FILENO); 
                close(prev_fd); 
            }
            if (i < n - 1)     
            { 
                dup2(fd[1], STDOUT_FILENO); 
                close(fd[1]); close(fd[0]);
            }

            if(strcmp(args[0], "echo") == 0)
            {
                if(!args[1]) {exit(0);}
                if (args[1] && args[1][0] == '$') 
                { 
                    const char *val = getenv(args[1] + 1);
                    std::cout << (val ? val : "") << std::endl;
                    exit(0); 
                }
                else for (int i = 1; args[i]; i++) std::cout << args[i] << (args[i+1] ? " " : "\n");
                exit(0);
            }

            io_redir(args);

            execvp(args[0], args.data());
            perror(args[0]);
            exit(1);
        }
        
        if (prev_fd != -1) close(prev_fd);
        if (i < n - 1) { close(fd[1]); prev_fd = fd[0]; }
    }
    for (int i = 0; i < n; i++) wait(nullptr);
    return 0;
}

std::vector<std::vector<std::string>> splitcmds(std::string& text)
{
    std::vector<std::string> cmds;

    size_t pos = 0;
    while ((pos = text.find('|')) != std::string::npos) 
    {
        std::string token = text.substr(0, pos);
        if(!token.empty()) cmds.push_back(token);
        text.erase(0, pos + 1);
    }
    if(!text.empty()) cmds.push_back(text);

    int n = cmds.size();

    std::vector<std::vector<std::string>> split_cmds;
    
    for (int i = 0; i < n; i++) 
    {
        std::vector<std::string> args;
        std::istringstream iss(cmds[i]);
        std::string token;
        while (iss >> token) args.push_back(token);
        if (!args.empty()) split_cmds.push_back(args);
    }
    return split_cmds;

}

int prompt(char *username, char *hostname)
{
    std::string text;

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char *folder = strrchr(cwd, '/');
        std::string prompt = std::string(username) + "@" + hostname + ":" + (strcmp(cwd, "/") == 0 ? "/" : (folder ? folder + 1 : cwd)) + "$ ";

        char *line = readline(prompt.c_str());      
        if (!line) 
        {
            write_history(history_path);
            exit(0); 
        }
        if (*line) add_history(line);
        text = std::string(line);
        free(line);

        if(text.empty()) return 1;
        std::vector<std::vector<std::string>> split_cmds = splitcmds(text);

        if (split_cmds.size() == 1) 
        {
            int m = split_cmds[0].size();
            std::vector<char*> args(m + 1);
            for (int j = 0; j < m; j++) args[j] = const_cast<char*>(split_cmds[0][j].c_str());
            args[m] = nullptr;
            return run_cmd(args);
        }
        else run_pipeline(split_cmds);
    }
    return -1;
}

int main(int argc, char *argv[])
{   
    strcpy(history_path, getenv("HOME"));
    strcat(history_path, "/.shellfish_history");

    signal(SIGINT, SIG_IGN);
    FILE *f = fopen(history_path, "a");
    if(f) fclose(f);

    read_history(history_path);
    char hostname[HOST_NAME_MAX];
    char *username = getlogin();
    
    gethostname(hostname, sizeof(hostname));

    if(argc > 1 && argv[1])
    {
        FILE *script = fopen(argv[1], "r");
        if (!script) { perror(argv[1]); return 1; }
        char *line = NULL;
        size_t len = 0;
        while (getline(&line, &len, script) != -1) 
        {
            std::string text = std::string(line);
            if (!text.empty() && text.back() == '\n') text.pop_back();

            std::vector<std::vector<std::string>> split_cmds = splitcmds(text);
            if (split_cmds.size() == 1) 
            {
                int m = split_cmds[0].size();
                std::vector<char*> args(m + 1);
                for (int j = 0; j < m; j++) args[j] = const_cast<char*>(split_cmds[0][j].c_str());
                args[m] = nullptr;
                run_cmd(args);
            }
            else run_pipeline(split_cmds);
        }
        free(line);
        fclose(script);
        return 0;
    }

    while(true) prompt(username, hostname);
    return 0;
}