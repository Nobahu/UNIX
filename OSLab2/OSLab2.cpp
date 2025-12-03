#include <iostream>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

class SafeServer {
private:
    volatile sig_atomic_t signal_received;
    int server_fd;
    int client_fd;

    void setup_signal_handler() {
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
	    static auto handler = [](int sig) {
	        signal_received = 1;
	    };
        sa.sa_flags = SA_RESTART;
        sigaction(SIGHUP, &sa, nullptr);
    }

    void setup_socket(int port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) throw std::runtime_error("socket failed");

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(server_fd);
            throw std::runtime_error("bind failed");
        }

        if (listen(server_fd, 5) < 0) {
            close(server_fd);
            throw std::runtime_error("listen failed");
        }
    }

public:
    SafeServer(int port) : signal_received(0), client_fd(-1) {
        setup_signal_handler();
        setup_socket(port);
    }

    void run() {
        sigset_t mask, orig_mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGHUP);
        sigprocmask(SIG_BLOCK, &mask, &orig_mask);

        std::cout << "Server started (PID: " << getpid() << ")" << std::endl;
        std::cout << "Send SIGHUP with: kill -HUP " << getpid() << std::endl;

        while (true) {
            // Проверка сигнала - атомарная операция
            if (signal_received) {
                std::cout << "Received SIGHUP signal" << std::endl;
                signal_received = 0;
            }

            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd, &read_fds);
            int max_fd = server_fd;

            if (client_fd != -1) {
                FD_SET(client_fd, &read_fds);
                if (client_fd > max_fd) max_fd = client_fd;
            }

            // pselect атомарно разблокирует сигналы
            int ready = pselect(max_fd + 1, &read_fds, nullptr, nullptr, nullptr, &orig_mask);

            if (ready == -1) {
                if (errno == EINTR) continue;  // Сигнал прервал pselect
                throw std::runtime_error("pselect failed");
            }

            // Новое подключение
            if (FD_ISSET(server_fd, &read_fds)) {
                int new_client = accept(server_fd, nullptr, nullptr);
                if (new_client >= 0) {
                    std::cout << "New connection: fd " << new_client << std::endl;
                    if (client_fd == -1) {
                        client_fd = new_client;
                        std::cout << "Keeping connection: fd " << client_fd << std::endl;
                    }
		    else {
                        close(new_client);
                        std::cout << "Closed extra connection: fd " << new_client << std::endl;
                    }
                }
            }
            // Данные от клиента
            if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
                char buffer[1024];
                ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));

                if (bytes_read > 0) {
                    std::cout << "Received " << bytes_read << " bytes from client" << std::endl;
                }
		else {
                    std::cout << "Client disconnected: fd " << client_fd << std::endl;
                    close(client_fd);
                    client_fd = -1;
                }
            }
        }
    }

    ~SafeServer() {
        if (client_fd != -1) close(client_fd);
        close(server_fd);
    }
};

int main() {
    try {
        SafeServer server(5000);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
