#include "ftpctrl.h"
#include "socket.h"
#include "ftpserver.h"
#include "ftpclient.h"
#include "ftpls.h"
#include <thread>

#define MAX_PATH 1024
int running=0;

struct oldcwd {
    char path[MAX_PATH];
    int set;
};

class FtpSession {
public:

    FtpSession(std::unique_ptr<TcpSocket> sock) : ctrlSock_(std::move(sock)) {
        pasvReady_ = false;

    }

    void start() {
        pasvReady_ = false;
        while(true) {
            std::string res;
            auto path=std::filesystem::current_path();
            std::string now_path=path.string();
            // ctrlSock_->sendMsg(now_path);
            Msgpack n_path;
            n_path.type = MsgType::PATH_INFO;
            n_path.msg = now_path;
            ctrlSock_->sendMsgpack(n_path);
            ctrlSock_->recvMsg(res);
            // if(res != "yes") {
            //     std::cout << res << std::endl;
            //     continue;
            // }
            // std::cout << res << std::endl;

            std::string msg;
            if(ctrlSock_->recvMsg(msg) != NetResult::OK) {
                std::cout << "[INFO] client disconnected or recv failed\n";
            }
            std::cout << "[INFO] recv: " << msg << "\n";
            if(msg == "QUIT") {
                ctrlSock_->sendMsg("BYE");
            }
            if(ctrlSock_->sendMsg("ACK: " + msg) != NetResult::OK) {
                std::cerr << "[FAIL] sendMsg failed\n";
            }
            if(msg == "skip") continue;
            std::vector<std::string> token;
            token=gettoken(msg);

            if(token.size()==0) {
                continue;
            }

            if(token[0]=="cd" || token[0]=="CWD") {
                if(token.size()>2) {
                    std::cout << "CWD: 参数太多" << std::endl;
                }
                doCWD(token[1]);
                continue;
            }

            if(token[0]=="exit" || token[0]=="QUIT") {
                signal(SIGCHLD,SIG_IGN);
                rl_clear_history();
                break;
            }
        
            if(token[0]=="PASV" || pasvReady_==true) {
                if(pasvReady_==false) {
                    doPASV();
                    continue;
                }
                if(pasvReady_==true) {
                    std::cout << "111\n";
                    if(run_cmd(token)) continue;
                }
            }
        }
    }

private:

    bool run_cmd(std::vector<std::string> token) {
        bool used = false;
        auto path=std::filesystem::current_path();
        std::string now_path=path.string();
        int status;
    // 在这里使用数据连接，实现LIST、STOR、RETR的接收
        
        if(token[0]=="STOR") {
            pasv->sendMsg("start_stor");
            pasv->sendMsg(token[1]);
            pasv->recvFile(token[1]);

            used = true;
        }

        if(token[0]=="RETR") {
            pasv->sendMsg("start_retr");
            pasv->sendMsg(token[1]);
            pasv->sendFile(token[1]);

            used = true;
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

            NetResult a;
            a = pasv->sendMsg("start_ls");

            for (const std::string& s : ls_res) {
                pasv->sendMsg(s);
            }
            pasv->sendMsg("stop");

            used = true;
        }
        if(used) pasvReady_=false;
        else pasv->sendMsg("not used");
        return used;
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

private:
    bool doCWD(std::string s) {
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

    bool doPASV() {
        // TcpServer dataServer;
        // std::unique_ptr<TcpSocket> pasv;
        sockaddr_in addr;
        if(!dataServer.setListen(0)) {
            std::cerr << "data listen failed\n";
            // return 1;
        }

        std::cout << "[INFO] data listening...\n";
        unsigned short dataPort = dataServer.getPort();
        if(dataPort != 0) {
            int p1 = dataPort/256;
            int p2 = dataPort%256;
            std::string reply = "227 entering passive mode (127,0,0,1," 
                + std::to_string(p1) + "," + std::to_string(p2) + ")";

            ctrlSock_->sendMsg(reply);
        }

        pasv = dataServer.acceptConn();

        std::cout << "[PASS] client connected\n";
        std::cout << "[DATA] waiting data connection...\n";

        pasvReady_ = true;
        return true;
    }


private:
    std::unique_ptr<TcpSocket> ctrlSock_;
    std::unique_ptr<TcpSocket> pasv;
    TcpServer dataServer;


private:
    bool pasvReady_;

};


void handle_SIGINT(int sig) {
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

void handle_SIGTSTP(int sig) {
    if(running==1) {
        printf("\n");
    }
}

void handle_signal(){
    signal(SIGINT,handle_SIGINT);
    signal(SIGTSTP,handle_SIGTSTP);
}

void restore_signal() {
    signal(SIGINT,SIG_DFL);
    signal(SIGTSTP,SIG_DFL);
}

int start_client() {
    handle_signal();
    TcpClient client;
    if(!client.connectToHost("127.0.0.1", 2100)) {
        std::cerr << "[FAIL] connectToHost failed\n";
        return 1;
    }
    std::cout << "[PASS] connected to server\n";
    TcpSocket* sock = client.getSocket();
    if (!sock) {
        std::cerr << "[FAIL] socket null\n";
        return 1;
    }
    std::string workpath=std::string(getenv("HOME")) + "/Download";
    mkdir(workpath.c_str(),0755);
    chdir(workpath.c_str());

    bool pasving=false;
    TcpClient dataClient;
    TcpSocket* pasv;
    while(1) {
        std::string now_path;
        Msgpack n_path;
        // if(sock->recvMsg(now_path) != NetResult::OK) continue;
        sock->recvMsgpack(n_path);
        if(n_path.type != MsgType::PATH_INFO) continue;
        // else sock->sendMsg("yes");

        std::string prompt="ftp client >> server:\033[34m" + n_path.msg + "\033[0m ";
        char *inp=NULL;
        inp=readline(prompt.c_str());

        if(inp==NULL) {
            free(inp);
            continue;
        }
        std::string input(inp);
        free(inp);
        if(input.size()==0 || input.empty()) {
            // sock->sendMsg("skip");
            continue;
        }
        add_history(input.c_str());

        if(!input.empty()) {
            sock->sendMsg(input);
        } else {
            continue;
        }

        std::string res;
        sock->recvMsg(res);
        std::cout << res << std::endl;

        if(pasving) {
            std::string msa;
            pasv->recvMsg(msa);

            std::cout << msa << std::endl;

            if(msa=="start_ls") {
                while(true) {
                    std::string ls_res;
                    pasv->recvMsg(ls_res);
                    if(ls_res=="stop") break;
                    std::cout << ls_res << std::endl;
                }
            } else if(msa=="start_stor") {
                std::string path;
                pasv->recvMsg(path);
                pasv->sendFile(path);
            } else if(msa=="start_retr") {
                std::string path;
                pasv->recvMsg(path);
                std::filesystem::path p(path);
                std::string filename = p.filename().string();
                pasv->recvFile(filename);
            }

            pasving = false;
            continue;
        }

        if(input=="PASV") {
            pasving=true;
            std::string reply;
            sock->recvMsg(reply);

            int h1,h2,h3,h4,p1,p2;
            sscanf(reply.c_str(),
                "227 entering passive mode (%d,%d,%d,%d,%d,%d)",
                &h1,&h2,&h3,&h4,&p1,&p2
            );

            int port = p1*256 + p2;
            std::string ip =
                std::to_string(h1) + "." +
                std::to_string(h2) + "." +
                std::to_string(h3) + "." +
                std::to_string(h4);

            std::cout << "ip = " << ip << std::endl;
            std::cout << "port = " << port << std::endl;

            if(!dataClient.connectToHost(ip.c_str(),port)) {
                std::cerr << "data connect failed\n";
                return 1;
            }
            std::cout << "[DATA] connected\n";
            pasv = dataClient.getSocket();
            continue;
        }

        if(input=="exit") {
            signal(SIGCHLD,SIG_IGN);
            rl_clear_history();
            break;
        }
    }
    return 0;
}

int start_server() {
    chdir(getenv("HOME"));
    TcpServer server;

    if(!server.setListen(2100)) {
        std::cerr << "[FAIL] setListen failed\n";
        return 1;
    }
    std::cout << "[INFO] server listening on port 2100...\n";

    while(true) {
        auto sock = server.acceptConn();
        if(!sock) {
            continue;
        } else {
            std::cout << "[PASS] client connected\n";
        }
        std::thread([sock = std::move(sock)]() mutable {
                FtpSession session(std::move(sock));
                session.start();
            }
        ).detach();
    }

    return 0;
}