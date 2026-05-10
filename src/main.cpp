#include "ftpserver.h"
#include "ftpclient.h"
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <chrono>

static std::string makePayload(int id, int size)
{
    std::string s;
    s.resize(size);

    for(int i = 0; i < size; i++)
    {
        s[i] = 'A' + (i + id) % 26;
    }

    return s;
}

static bool verifyPayload(const std::string& s, int id)
{
    for(size_t i = 0; i < s.size(); i++)
    {
        if(s[i] != 'A' + (i + id) % 26)
            return false;
    }
    return true;
}

void runServer()
{
    TcpServer server;

    if(!server.setListen(19000))
    {
        std::cout << "[FAIL] listen\n";
        return;
    }

    std::cout << "[INFO] server listening...\n";

    if(!server.acceptConn())
    {
        std::cout << "[FAIL] accept\n";
        return;
    }

    std::cout << "[PASS] client connected\n";

    auto sock = server.getSocket();

    const int TESTS = 10000;

    for(int i = 0; i < TESTS; i++)
    {
        std::string msg;

        if(sock->recvMsg(msg) != 0)
        {
            std::cout << "[FAIL] recv at " << i << "\n";
            return;
        }

        if(!verifyPayload(msg, i))
        {
            std::cout << "[FAIL] corrupt at " << i
                      << " size=" << msg.size() << "\n";
            return;
        }

        if(i % 1000 == 0)
        {
            std::cout << "[INFO] checked " << i << "\n";
        }

        // 回显压力（增加send压力）
        if(sock->sendMsg(msg) != 0)
        {
            std::cout << "[FAIL] send at " << i << "\n";
            return;
        }
    }

    std::cout << "[PASS] SERVER OK\n";
}

void runClient()
{
    TcpClient client;

    if(!client.connectToHost("127.0.0.1", 19000))
    {
        std::cout << "[FAIL] connect\n";
        return;
    }

    std::cout << "[PASS] connected\n";

    auto sock = client.getSocket();

    std::mt19937 rng(time(nullptr));
    std::uniform_int_distribution<int> sizeDist(1, 1024 * 1024);

    const int TESTS = 10000;

    for(int i = 0; i < TESTS; i++)
    {
        int size;

        // 混合压力：
        if(i % 100 == 0)
            size = 1024 * 1024;   // 1MB 极限包
        else if(i % 10 == 0)
            size = 64 * 1024;     // 中大包
        else
            size = sizeDist(rng);  // 随机小包

        std::string data = makePayload(i, size);

        if(sock->sendMsg(data) != 0)
        {
            std::cout << "[FAIL] send at " << i << "\n";
            return;
        }

        std::string reply;

        if(sock->recvMsg(reply) != 0)
        {
            std::cout << "[FAIL] recv reply at " << i << "\n";
            return;
        }

        if(reply != data)
        {
            std::cout << "[FAIL] mismatch at " << i << "\n";
            return;
        }

        if(i % 1000 == 0)
        {
            std::cout << "[INFO] sent " << i << "\n";
        }
    }

    std::cout << "[PASS] CLIENT OK\n";
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout << "./test server|client\n";
        return 0;
    }

    std::string mode = argv[1];

    if(mode == "server")
    {
        runServer();
    }
    else if(mode == "client")
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(300));

        runClient();
    }

    return 0;
}