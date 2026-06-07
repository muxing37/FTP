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

std::string prompt;

class FtpSession {
public:

    FtpSession(std::unique_ptr<TcpSocket> sock) : ctrlSock_(std::move(sock)) {
        pasvReady_ = false;
    }

    void start() {
        while(true) {
            std::string res;
            cwd_ = std::filesystem::current_path();
            oldCwd_ = cwd_;
            // auto path=std::filesystem::current_path();
            std::string now_path=cwd_.string();
            Msgpack n_path;
            n_path.type = MsgType::PATH_INFO;
            n_path.msg = now_path;
            ctrlSock_->sendMsgpack(n_path);
            ctrlSock_->recvMsg(res);
            if(res != "yes") {
                std::cout << res << std::endl;
                continue;
            }
            std::cout << res << std::endl;
            break;
        }
        while(true) {
            std::string msg;
            if(ctrlSock_->recvMsg(msg) != NetResult::OK) {
                std::cout << "[INFO] client disconnected or recv failed\n";
            }
            std::cout << "[INFO] recv: " << msg << "\n";

            std::vector<std::string> token;
            token=gettoken(msg);

            if(token.size()==0) {
                continue;
            } else if(token.size()==1 && (token[0] == "RETR" || token[0] == "STOR")) {
                ctrlSock_->sendMsg("请输入文件名");
                continue;
            } else {
                ctrlSock_->sendMsg("yes");
            }

            if(token[0]=="PASV" || pasvReady_==true) {
                if(pasvReady_==false) {
                    doPASV();
                    continue;
                }
                if(pasvReady_==true) {
                    if(run_cmd(token)) continue;
                }
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
    
        }
    }

private:

    bool run_cmd(std::vector<std::string> token) {
        bool used = false;
        // auto path=std::filesystem::current_path();
        std::string now_path=cwd_.string();
        int status;
    // 在这里使用数据连接，实现LIST、STOR、RETR的接收
        
        if(token[0]=="STOR") {
            pasv->sendMsg("start_stor");
            std::string path=now_path;
            if(token[1].size()>=2 && token[1].substr(0,2) == "./") {
                token[1].erase(0,2);
                path += "/" + token[1];
            } else if (token[1].size()>=1 && token[1].substr(0,1) == "/") {
                path.clear();
                path = token[1];
            } else {
                path += "/" + token[1];
            }
            uint64_t offset = 0;
            struct stat st;
            if(stat(path.c_str(), &st) == 0) {
                offset = st.st_size;
            }

            pasv->sendMsg(token[1] + "|" + std::to_string(offset));

            pasv->recvFile(path,offset);
            used = true;
        }

        if(token[0]=="RETR") {
            pasv->sendMsg("start_retr");
            std::string path=now_path;
            if(token[1].size()>=2 && token[1].substr(0,2) == "./") {
                token[1].erase(0,2);
                path += "/" + token[1];
            } else if (token[1].size()>=1 && token[1].substr(0,1) == "/") {
                path.clear();
                path = token[1];
            } else {
                path += "/" + token[1];
            }
            struct stat st;
            if(stat(path.c_str(), &st) != 0) {
                pasv->sendMsg("error:file_not_found");
                return false;
            } else {
                pasv->sendMsg("ok");
            }
            pasv->sendMsg(path);
            uint64_t filesize = st.st_size;
            std::string offsetMsg;
            pasv->recvMsg(offsetMsg);

            uint64_t offset = std::stoull(offsetMsg);

            if(offset > filesize) {
                offset = 0;
            }

            pasv->sendFile(path,offset);
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
                if(s.size()>=2 && s.substr(0,2) == "./") {
                    s.erase(0,2);
                }
                argv.push_back(s.data());
                argc++;
            }
            std::vector<std::string> ls_res;
            ls_res=startls(argc,argv.data(),cwd_);

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

    bool doCWD(const std::string& s) {
        namespace fs = std::filesystem;
        fs::path newPath;
        while(true) {
            if(s == "-") {
                if(oldCwd_.empty()) {
                    std::cout << "OLDPWD not set\n";
                    ctrlSock_->sendMsg("OLDPWD not set");
                    return false;
                }
                std::cout << oldCwd_ << "\n";
                ctrlSock_->sendMsg("ok");
                std::swap(cwd_,oldCwd_);
                break;
            }

            if(!s.empty() && s[0] == '~') {
                const char* home = getenv("HOME");
                if(home == nullptr) {
                    ctrlSock_->sendMsg("home == nullptr");
                    return false;
                }
                newPath = fs::path(home);
                if(s.size() > 1) {
                    newPath /= s.substr(1);
                }
            } else if(fs::path(s).is_absolute()) {
                newPath = fs::path(s);
            } else {
                newPath = cwd_ / s;
            }

            try {
                newPath = fs::weakly_canonical(newPath);
            }
            catch(...) {
                ctrlSock_->sendMsg("error");
                return false;
            }

            if(!fs::exists(newPath)) {
                std::cout << "No such file or directory\n";
                ctrlSock_->sendMsg("No such file or directory");
                return false;
            }

            if(!fs::is_directory(newPath)) {
                std::cout << "Not a directory\n";
                ctrlSock_->sendMsg("Not a directory");
                return false;
            }
            ctrlSock_->sendMsg("ok");
            oldCwd_ = cwd_;
            cwd_ = newPath;
            break;
        }

        while(true) {
            std::string res;
            // auto path=std::filesystem::current_path();
            std::string now_path=cwd_.string();
            Msgpack n_path;
            n_path.type = MsgType::PATH_INFO;
            n_path.msg = now_path;
            ctrlSock_->sendMsgpack(n_path);
            ctrlSock_->recvMsg(res);
            if(res != "yes") {
                std::cout << res << std::endl;
                continue;
            }
            std::cout << res << std::endl;
            break;
        }
        return true;
    }

    bool doPASV() {
        sockaddr_in addr;
        if(!dataServer.setListen(0)) {
            std::cerr << "data listen failed\n";
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
    std::filesystem::path cwd_;
    std::filesystem::path oldCwd_;
    bool pasvReady_;
};

void handle_SIGINT(int sig) {
    std::cout << "\n" << prompt << std::flush;
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

    while(true) {
        std::string now_path;
        Msgpack n_path;
        sock->recvMsgpack(n_path);
        if(n_path.type != MsgType::PATH_INFO) {
            sock->sendMsg("unexpected");
            continue;
        }else {
            sock->sendMsg("yes");
        }
        prompt.clear();
        prompt="ftp client >> server:\033[34m" + n_path.msg + "\033[0m ";
        break;
    }

    while(true) {
        char *inp=NULL;
        inp=readline(prompt.c_str());
        if(inp==NULL) {
            free(inp);
            continue;
        }
        std::string input(inp);
        free(inp);

        if(input.size()==0 || input.empty()) {
            continue;
        }

        add_history(input.c_str());
// input.substr(0,4) == "RETR" || input.substr(0,4) == "STOR"
        if(input.empty()) {
            continue;
        } else {
            sock->sendMsg(input);
        }

        std::string res;
        sock->recvMsg(res);
        if(res != "yes") {
            std::cout << res << std::endl;
            continue;
        }
        // std::cout << res << std::endl;
        if(pasving == false && (input.size() >= 4 && (input.substr(0,4) == "RETR" || input.substr(0,4) == "STOR" || input.substr(0,4) == "LIST"))) {
            std::cout << "请使用 PASV 建立数据连接" << std::endl;
            continue;
        }

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
                pasving = false;
            } else if(msa=="start_stor") {
                std::string server_path;
                pasv->recvMsg(server_path);

                size_t sep = server_path.find('|');
                std::string filename = server_path.substr(0, sep);
                uint64_t offset = 0;
                if(sep != std::string::npos) {
                    offset = std::stoull(server_path.substr(sep + 1));
                }

                std::cout << "Uploading from offset: " << offset << std::endl;

                pasv->sendFile(filename,offset);

                pasving = false;

            } else if(msa=="start_retr") {
                std::string res;
                pasv->recvMsg(res);
                if(res != "ok") {
                    std::cout << res << std::endl;
                    continue;
                }
                std::string server_path;
                pasv->recvMsg(server_path);

                std::filesystem::path p(server_path);
                std::string filename = p.filename().string();

                uint64_t offset = 0;
                struct stat st;
                if(stat(filename.c_str(), &st) == 0) {
                    offset = st.st_size;
                }

                pasv->sendMsg(std::to_string(offset));
                pasv->recvFile(filename, offset);

                pasving = false;
                continue;
            }
        }

        if((input.size()>=2 && input.substr(0, 2) == "cd") || (input.size()>=3 && input.substr(0,3) == "CWD")) {
            while(true) {
                std::string now_path;
                Msgpack n_path;
                std::string res;
                sock->recvMsg(res);
                if(res != "ok") {
                    std::cout << res <<std::endl;
                    break;
                }
                sock->recvMsgpack(n_path);
                if(n_path.type != MsgType::PATH_INFO) {
                    sock->sendMsg("unexpected");
                    continue;
                }else {
                    sock->sendMsg("yes");
                }
                prompt.clear();
                prompt="ftp client >> server:\033[34m" + n_path.msg + "\033[0m ";
                break;
            }
        }

        if(input=="PASV" && pasving==false) {
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