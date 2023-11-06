#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <filesystem>
#include <string>
#include <vector>

#include "debug.h"
#include "commands.h"

const std::filesystem::path PLOX_DEVICE = "/dev/plox";

int 
CreateServerSocket()
{
	std::filesystem::remove(SOCK_PATH);

	struct sockaddr_un address;
	memset(&address, 0, sizeof(struct sockaddr_un));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, SOCK_PATH, sizeof(address.sun_path) - 1);

	int serverSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

	// Bind the socket to the specified path
	if (bind(serverSocket, (struct sockaddr*)&address, sizeof(struct sockaddr_un)) == -1) {
		close(serverSocket);
		return -1;
	}

	// Listen for incoming connections
	if (listen(serverSocket, 1) == -1) {
		close(serverSocket);
		return -1;
	}

	return serverSocket;
}

void 
HandleConnection(int connection)
{
	char buffer[4096];
	int pairs[2];

	memset(buffer, '\0', 4096);
	int readin = read(connection, buffer, 4096);
	if (readin < 0) {
		perror("issue");
		close(connection);
		return;
	}

	std::string program = std::string(buffer, strlen(buffer));

	int error = socketpair(AF_LOCAL, SOCK_STREAM, 0, pairs);
	if (error) {
		perror("Issue with socket");
		close(connection);
		return;
	}


	std::vector<std::string> args;
	std::vector<char *> argv;

	args.push_back(program);

	// Place the socket to ploxd as the last argument
	args.push_back(std::to_string(pairs[1]));

	for (auto &arg: args) {
		argv.push_back((char *)arg.c_str());
	}

	argv.push_back(nullptr);

	fcntl(pairs[0], F_SETFD, FD_CLOEXEC);

	int pid = fork();
	if (pid) {
		memset(buffer, '\0', 4096);
		int readin = read(pairs[0], buffer, 4096);
		if (readin < 0) {
			perror("issue");
			close(connection);
			return;
		}

		INFO("Message Recieved: {}", buffer);
	} else {
		if (execve("/usr/home/ryan/ploxd/build/src/ploxd/containerd", argv.data(), NULL) == -1) {
			perror("execve");
			return;
		}
		return;
	}

	return;
}

int 
main(int argc, char *argv[])
{
	int kq = kqueue();
	if (kq == -1) {
		ERROR("Could not create kqueue");
		return -1;
	}

	if (!std::filesystem::exists(PLOX_DEVICE)) {
		ERROR("PLOX Device not present, please load plox.ko");
		return -1;
	}

	auto serverSocket = CreateServerSocket();
	// Define the event structure
	struct kevent change;
	EV_SET(&change, serverSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);

	// Register the event
	if (kevent(kq, &change, 1, NULL, 0, NULL) == -1) {
		ERROR("Could not register event");
		perror("Error");
		return -1;
	}

	struct kevent event;
	while (true) {
		int nev = kevent(kq, NULL, 0, &event, 1, NULL);
		if (nev == -1) {
			ERROR("Main loop error");
			return -1;
		}

		if (nev > 0) {
			if (event.filter == EVFILT_READ) {
				int connection = accept4(serverSocket, NULL, NULL, SOCK_CLOEXEC);
				INFO("Information available on FD {}", connection);
				HandleConnection(connection);
			}
		}
	}

	return 0;
}
