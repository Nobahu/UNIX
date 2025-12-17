#include <iostream>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

class Server {
private:
    static volatile sig_atomic_t wasSigHup;
    static volatile sig_atomic_t shouldExit;
    int server_fd;
    int client_fd;
    sigset_t origMask;

    static void sigHupHandler(int r) {
        wasSigHup = 1;
    }
    
    static void sigIntHandler(int sig) {
        shouldExit = 1;
    }

    void setupSignalHandler() {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        
        //SIGHUP
        sa.sa_handler = sigHupHandler;
        sa.sa_flags = SA_RESTART;
        sigaction(SIGHUP, &sa, NULL);
        
        //SIGINT
        sa.sa_handler = sigIntHandler;
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);
    }

    void setupSocket() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        
        struct sockaddr_in sock_addr;
        memset(&sock_addr, 0, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(8080);
        sock_addr.sin_addr.s_addr = INADDR_ANY;
        
        bind(server_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
        listen(server_fd, 5);
    }

public:
    Server() : client_fd(-1) {
        setupSignalHandler();
        setupSocket();
        
        sigset_t blockedMask;
        sigemptyset(&blockedMask);
        sigaddset(&blockedMask, SIGHUP);
        sigprocmask(SIG_BLOCK, &blockedMask, &origMask);
    }

    void run() {
        std::cout << "Сервер запущен. PID: " << getpid() << std::endl;
        std::cout << "Отправьте SIGHUP: kill -HUP " << getpid() << std::endl;

        while (!shouldExit) {
            if (wasSigHup == 1) {
                std::cout << "Был получен сигнал SIGHUP!" << std::endl;
                wasSigHup = 0;
            }

            int maxFd = server_fd;
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(server_fd, &fds);
            
            if (client_fd != -1) {
                FD_SET(client_fd, &fds);
                if (maxFd < client_fd) {
                    maxFd = client_fd;
                }
            }

            if (pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask) == -1) {
                if (errno == EINTR) {
                    continue;
                } else {
                    std::cout << "Ошибка pselect!" << std::endl;
                    exit(1);
                }
            }

            if (FD_ISSET(server_fd, &fds)) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int new_client = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            
                if (new_client != -1) {
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                    int client_port = ntohs(client_addr.sin_port);
                    std::cout << "Новое подключение от " << client_ip << ":" << client_port << std::endl;
            
                    if (client_fd != -1) {
                        std::cout << "Закрываем подключение: разрешено только одно активное подключение" << std::endl;
                        close(new_client);
                    } else {
                        client_fd = new_client;
                        std::cout << "Активное подключение установлено" << std::endl;
                    }
                }
            }

            if (client_fd != -1 && FD_ISSET(client_fd, &fds)) {
                char buffer[1024];
                int message_size = read(client_fd, buffer, sizeof(buffer));
                
                if (message_size > 0) {
                    std::cout << "Получено сообщение от " << client_fd << " размером " << message_size << "!" << std::endl;
                } else {
                    std::cout << "Подключение " << client_fd << " закрыто!" << std::endl;
                    close(client_fd);
                    client_fd = -1;
                }
            }
        }
    }

    ~Server() {
        if (client_fd != -1) {
            close(client_fd);
        }
        close(server_fd);
    }
};

volatile sig_atomic_t Server::wasSigHup = 0;
volatile sig_atomic_t Server::shouldExit = 0;

int main() {
    Server server;
    server.run();
    std::cout << "Сервер остановлен." << std::endl;
    return 0;
}
