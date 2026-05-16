#include <iostream>
#include <string>
#include <vector>

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include<limits.h>
#include <readline/readline.h>
#include <readline/history.h>

char cwd[PATH_MAX];
char prev_dir[PATH_MAX];
std::string history_path = (std::string(getenv("HOME")) + "/.shellfish_history");


int runCmd(char *args[])
{
    if (strcmp(args[0], "exit") == 0) 
    {
        write_history(history_path.c_str());
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0)
    {
        if(!args[1] || strcmp(args[1], "~") == 0) 
        {
            strcpy(prev_dir, cwd);
            chdir(getenv("HOME"));
            return 0;
        }
        if(strcmp(args[1], "-") == 0) 
        {
            char temp[PATH_MAX];
            strcpy(temp, prev_dir);
            strcpy(prev_dir, cwd);
            chdir(temp);
            return 0;
        }
        strcpy(prev_dir, cwd);
        chdir(args[1] ? args[1] : getenv("HOME")); 
        return 0;
    }

    if(strcmp(args[0], "echo") == 0)
        if (args[1][0] == '$') { std::cout << getenv(args[1] + 1) << "\n"; return 0; }

    int child = fork();
    if(child < 0) 
    {
        perror("fork");
        return -1;
    }
    else if(child == 0)
    {
        signal(SIGINT, SIG_DFL);
        int r = execvp(args[0], args);
        perror(args[0]);
        exit(0);
    }
    else
    {
        int rc = wait(NULL);
        return 0;
    }
}

int prompt(char *username, char *hostname)
{
    std::string text;
    std::vector<std::string> words;

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char *folder = strrchr(cwd, '/');
        std::string prompt = std::string(username) + "@" + hostname + ":" + (strcmp(cwd, "/") == 0 ? "/" : (folder ? folder + 1 : cwd)) + "$ ";

        char *line = readline(prompt.c_str());      
        if (!line) 
        { 
            write_history(history_path.c_str());
            exit(0); 
        }
        if (*line) add_history(line);
        text = std::string(line);
        free(line);

        size_t pos = 0;
        while ((pos = text.find(' ')) != std::string::npos) 
        {
            std::string token = text.substr(0, pos);
            if(!token.empty()) words.push_back(token);
            text.erase(0, pos + 1);
        }
        if(!text.empty()) words.push_back(text);

        int n = words.size();
        

        char *args[n+1];
        for(int i = 0; i < n; i++)
        {
            args[i] = const_cast<char*>(words[i].c_str());
        }
        args[n] = NULL;

        return runCmd(args);

    }
    return -1;
}

int main()
{
    signal(SIGINT, SIG_IGN);
    FILE *f = fopen(history_path.c_str(), "a");
    if(f) fclose(f);

    read_history(history_path.c_str());
    char hostname[256];
    char *username = getlogin();
    
    gethostname(hostname, sizeof(hostname));

    while(true)
    {
        prompt(username, hostname);
    }
    return 0;
}