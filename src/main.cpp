#ifdef _WIN32
    // windows.h инклудит winsock.h, а я хочу иметь возможность использовать новый winsock2 (1994)
    #define WIN32_LEAN_AND_MEAN
#endif

#include "useful_stuff.h"
#include "network_stuff.h"
#include <cstring>

#include <print>

extern LogLvl LOGLVL;

// Хочу гарантировать вызов деструктора, ничего более
struct Application{
    explicit Application(int argc, char* argv[]);
    ~Application();
    void run();
private:
    NetworkStuff network;
};

int main(int argc, char* argv[]){
    Application app(argc, argv);
    app.run();
    return 0;
}

Application::Application(int argc, char* argv[]){
    if (argc > 1){
        if (strcmp(argv[1], "TRACE") == 0)
            LOGLVL = LogLvl::TRACE;
        else if (strcmp(argv[1], "DEBUG") == 0)
            LOGLVL = LogLvl::DEBUG;
        else if (strcmp(argv[1], "INFO") == 0)
            LOGLVL = LogLvl::INFO;
        else if (strcmp(argv[1], "WARN") == 0)
            LOGLVL = LogLvl::WARN;
        else if (strcmp(argv[1], "ERROR") == 0)
            LOGLVL = LogLvl::ERR;
        else if (strcmp(argv[1], "CRITICAL") == 0)
            LOGLVL = LogLvl::CRITICAL;
       
        LOG(TRACE, "LOGLVL: {}", LOGLVL);
    }
    NetworkMode MODE = NetworkMode::CLIENT;
    if (argc > 2){
        if (strcmp(argv[2], "SERVER") == 0)
            MODE = NetworkMode::SERVER;
        else if (strcmp(argv[2], "CLIENT") == 0)
            MODE = NetworkMode::CLIENT;
        else if (strcmp(argv[2], "CLIENTANDSERVER") == 0)
            MODE = NetworkMode::CLIENTANDSERVER;

        LOG(INFO, "MODE: {}", MODE);
    }
    network.Init(MODE);
}

Application::~Application(){
    
}

void Application::run(){
    network.run();
    std::println("Hello World!");
}
