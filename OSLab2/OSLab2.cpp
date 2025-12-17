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
    int connection_counter;

    static void sigHupHandler(int r) {
        wasSigHup = 1;
    }
    
    static void sigIntHandler(int sig) {
        shouldExit = 1;
    }

    void setupSignalHandler() {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sigaction(SIGHUP, NULL, &sa);
        
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
    Server() : client_fd(-1), connection_counter(0) {
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
                int new_client = accept(server_fd, NULL, NULL);
                if (new_client != -1) {
                    std::cout << "Принято новое подключение: " << new_client << "!" << std::endl;
                    connection_counter += 1;
                    
                    if (connection_counter > 1) {
                        close(new_client);
                        std::cout << "Подключение " << new_client << " закрыто!" << std::endl;
                        connection_counter -= 1;
                    } else {
                        client_fd = new_client;
                    }
                }
            }

            if (client_fd != -1 && FD_ISSET(client_fd, &fds)) {
                char buffer[256];
                int msg_size = read(client_fd, buffer, sizeof(buffer));
                
                if (msg_size > 0) {
                    std::cout << "Получено сообщение от " << client_fd << " размером " << msg_size << "!" << std::endl;
                } else {
                    std::cout << "Подключение " << client_fd << " закрыто!" << std::endl;
                    close(client_fd);
                    client_fd = -1;
                    connection_counter -= 1;
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

private:
    sigset_t origMask;
};

volatile sig_atomic_t Server::wasSigHup = 0;
volatile sig_atomic_t Server::shouldExit = 0;

int main() {
    Server server;
    server.run();
    std::cout << "Сервер остановлен." << std::endl;
    return 0;
}
