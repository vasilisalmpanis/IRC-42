#include "Server.hpp"
#include <iterator>


// find some other solution
void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        return;
    }
}


Server::Server(int port, std::string password) : port(port), password(password)
{
    epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        throw std::runtime_error("Failed to create epoll instance\n");
    }
    this->port = port;
    this->password = password;
    this->serverSocket = -1;
    // this->users = {};
    // this->channels = {};
    // this->connections = {};

    Logger logger;
    std::string logTime = logger.getLogTime();
    logger.info("Server", "Server created", logger.getLogTime());
}

Server::Server(const Server& server)
{
    port = server.port;
    password = server.password;
    serverSocket = server.serverSocket;
    // users = server.users;
    // channels = server.channels;
}

Server::~Server()
{
    close(serverSocket);
    // logger message
    logger.info("~Server", "Server destroyed", logger.getLogTime());
}

void Server::setupSocket()
{
    signal(SIGINT, signalHandler);

    logger.info("setupSocket", "Creating socket...", logger.getLogTime());
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        throw std::runtime_error("Failed to create socket\n");
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    logger.info("setupSocket", "Binding socket...", logger.getLogTime());
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
    {
        throw std::runtime_error("Failed to bind socket\n");
    }

    logger.info("setupSocket", "Listening on socket...", logger.getLogTime());
    if (listen(serverSocket, 10) == -1)
    {
        throw std::runtime_error("Failed to listen on socket\n");
    }
}

void Server::start()
{
    setupSocket();
    logger.info("startServer", "Starting server...", logger.getLogTime());

    struct epoll_event events[MAX_EVENTS];
    while (true)
    {
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numEvents == -1) {
            throw std::runtime_error("Failed to wait for epoll events\n");
        }

        for (int i = 0; i < numEvents; i++)
        {
            if (events[i].data.fd == serverSocket)
            {
                // New connection on server socket
                acceptConnection();
            }
            else
            {
                // Data available to read on a client socket
                Connection* connection = findConnectionBySocket(events[i].data.fd);
                if (connection != NULL)
                {
                    handleConnection(connection);
                }
            }
        }
    }
}

void Server::stop()
{
    close(serverSocket);
}

void Server::acceptConnection()
{
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
    if (clientSocket < 0)
    {
        throw std::runtime_error("Failed to accept connection\n");
    }

    // Make the new socket non-blocking
    int flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags | SOCK_NONBLOCK );

    // Add the new socket to the epoll instance
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
        throw std::runtime_error("Failed to add client socket to epoll\n");
    }

    Connection* connection = new Connection(clientSocket);
    connections.push_back(connection);
}

void Server::handleConnection(Connection* connection)
{
    std::string message = connection->receive();
    if (!message.empty()) {
        std::cout << message << std::endl;
        connection->send_message("Hello from server\n");
    }
}

Connection* Server::findConnectionBySocket(int socket)
{
    for (std::vector<Connection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        if ((*it)->getSocket() == socket)
        {
            return *it;
        }
    }
    return NULL;
}

int Server::getPort()
{
    if(port == 0)
    {
        throw std::runtime_error("Port is not set\n");
    }
    if(port <= 0 || port > 65535)
    {
        throw std::runtime_error("Invalid port number\n");
    }
    return port;
}

std::string Server::getPassword()
{
    if(password.empty() || password.find_first_of(FORBIDDEN_CHARS) != std::string::npos)
    {
        throw std::runtime_error("Password is not set\n");
    }
    return password;
}


int Server::getServerSocket()
{
    return serverSocket;
}

std::vector<User*> Server::getUsers()
{
    return users;
}

std::vector<Channel*> Server::getChannels()
{
    return channels;
}

std::vector<Connection*> Server::getConnections()
{
    return connections;
}

void Server::setPort(int port)
{
    this->port = port;
}

void Server::setPassword(std::string password)
{
    this->password = password;
}

// void Server::setUsers(std::vector<User*> users)
// {
//     this->users = users;
// }

// void Server::setChannels(std::vector<Channel*> channels)
// {
//     this->channels = channels;
// }

void Server::setConnections(std::vector<Connection*> connections)
{
    this->connections = connections;
}

