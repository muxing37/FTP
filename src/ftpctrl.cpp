#include "ftpctrl.h"
#include "ftpserver.h"
#include "ftpclient.h"
#include "ftpls.h"

#define MAX_PATH 1024

int running=0;

struct oldcwd {
    char path[MAX_PATH];
    int set;
};

int cd(std::string s) {
    static struct oldcwd last_path={0};
    char t[MAX_PATH]={0};
    getcwd(t,sizeof(t));
    if(s=="-") {
        if(last_path.set==0) {
            printf("cd: OLDPWD 未设定\n");
            return -1;
        } else {
            printf("%s\n",last_path.path);
            chdir(last_path.path);
            strcpy(last_path.path,t);
            last_path.set=1;
            return 0;
        }
    }
    const char *str=s.c_str();
    if(str[0]=='~') {
        char temp[MAX_PATH]={0};
        strcpy(temp,getenv("HOME"));
        strcat(temp,str+1);
        if(chdir(temp)==-1) {
            perror("cd: ");
        }
        strcpy(last_path.path,t);
        last_path.set=1;
        return 0;
    }

    if(chdir(str)==-1) {
        perror("cd: ");
    }
    strcpy(last_path.path,t);
    last_path.set=1;
    return 0;
}

void handle_SIGINT() {
    struct timespec stop;
    stop.tv_sec=0;
    stop.tv_nsec=2000000;
    nanosleep(&stop,NULL);
    printf("\n");
    if(running==0) {
        rl_replace_line("",0);
        rl_on_new_line();
        rl_redisplay();
    }
}

void handle_SIGTSTP() {
    if(running==1) {
        printf("\n");
        // rl_replace_line("",0);
        // rl_on_new_line();
        // rl_redisplay();
    }
}

void handle_SIGCHLD() {
    int status;
    pid_t pid;
    pid=waitpid(-1,&status,WNOHANG);
}

void handle_SIGINT(int sig) {}
void handle_SIGTSTP(int sig) {}
void handle_SIGCHLD(int sig) {}

void handle_signal(){
    signal(SIGINT,handle_SIGINT);
    signal(SIGTSTP,handle_SIGTSTP);
    signal(SIGCHLD,handle_SIGCHLD);
}

void restore_signal() {
    signal(SIGINT,SIG_DFL);
    signal(SIGTSTP,SIG_DFL);
}

std::vector<std::string> gettoken(std::string input) {
    std::vector<std::string> token;
    std::string current;

    for(char c : input) {
        if(c == ' ') {
            if(!current.empty()) {
                token.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if(!current.empty()) {
        token.push_back(current);
    }

    return token;
}

int run_cmd(TcpSocket* pasv,std::vector<std::string> token) {
    running=1;
    auto path=std::filesystem::current_path();
    std::string now_path=path.string();
    int status;

    if(token[0]=="cd" || token[0]=="CWD") {
        if(token.size()>2) {
            std::cout << "CWD: 参数太多" << std::endl;
        }
        cd(token[1]);
    }

    if(token[0]=="ls" || token[0]=="LIST") {
        std::vector<char*> argv;
        int argc=0;
        now_path=now_path+"/ls";
        argv.push_back(now_path.data());
        argc++;
        for(auto& s : token) {
            if(s=="ls" || s=="LIST") continue;
            argv.push_back(s.data());
            argc++;
        }
        std::vector<std::string> ls_res;
        ls_res=startls(argc,argv.data());

    }
    running=0;
    return 0;
}

int start_client() {
    handle_signal();
    TcpClient client;
    if (!client.connectToHost("127.0.0.1", 2100)) {
        std::cerr << "[FAIL] connectToHost failed\n";
        return 1;
    }
    std::cout << "[PASS] connected to server\n";
    TcpSocket* sock = client.getSocket();
    if (!sock) {
        std::cerr << "[FAIL] socket null\n";
        return 1;
    }
    chdir(getenv("HOME"));
    while(1) {
        // auto path=std::filesystem::current_path();
        // std::string now_path=path.string();
        std::string now_path;
        sock->recvMsg(now_path);


        std::string prompt="ftp >> server:\033[34m" + now_path + "\033[0m ";

        char *inp=NULL;
        inp=readline(prompt.c_str());
        if(inp==NULL) {
            free(inp);
            continue;
        }
        std::string input(inp);
        free(inp);
        if(input.size()==0) {
            continue;
        }
        add_history(input.c_str());

        if(!input.empty()) sock->sendMsg(input);

        std::string res;
        sock->recvMsg(res);
        std::cout << res << std::endl;

        // while(true) {
        //     std::string ech;
        //     sock->recvMsg(ech);
        //     if(ech=="stop") break;
        //     if(ech=="ls_start") {
        //         while(true) {
        //             std::string rec;
        //             sock->recvMsg(rec);
        //             if(rec=="ls_stop") {
        //                 break;
        //             } else {
        //                 std::cout << rec << std::endl;
        //             }
        //         }
        //     } else if(ech=="pasv_start") {

        //     }
        // }

        if(input=="exit") {
            signal(SIGCHLD,SIG_IGN);
            rl_clear_history();
            // exit(0);
            break;
        }
    }
    return 0;
}

int start_server() {
    chdir(getenv("HOME"));
    int if_pasv=0;
    TcpServer server;

    if(!server.setListen(2100)) {
        std::cerr << "[FAIL] setListen failed\n";
        return 1;
    }

    std::cout << "[INFO] server listening on port 2100...\n";

    if(!server.acceptConn()) {
        std::cerr << "[FAIL] acceptConn failed\n";
        return 1;
    }

    std::cout << "[PASS] client connected\n";

    TcpSocket* sock = server.getSocket();
    TcpSocket* pasv;
    if(sock == nullptr) {
        std::cerr << "[FAIL] getSocket returned nullptr\n";
        return 1;
    }

    while(true) {
        auto path=std::filesystem::current_path();
        std::string now_path=path.string();
        sock->sendMsg(now_path);

        std::string msg;
        if(sock->recvMsg(msg) != 0) {
            std::cout << "[INFO] client disconnected or recv failed\n";
            break;
        }
        std::cout << "[INFO] recv: " << msg << "\n";
        if(msg == "QUIT") {
            sock->sendMsg("BYE");
            break;
        }
        if(sock->sendMsg("ACK: " + msg) != 0) {
            std::cerr << "[FAIL] sendMsg failed\n";
            break;
        }

        std::vector<std::string> token;
        token=gettoken(msg);

        if(token.size()==0) {
            continue;
        }
        
        if(token[0]=="PASV") {

        }

        if(token[0]=="exit" || token[0]=="QUIT") {
            signal(SIGCHLD,SIG_IGN);
            rl_clear_history();
            break;
        }

        run_cmd(pasv,token);

        // sock->sendMsg("stop");
    }
    return 0;
}