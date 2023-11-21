#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <filesystem>
#include <string>
#include <vector>

#include "debug.h"
#include "commands.h"
#include "policy.h"

const std::filesystem::path PLOX_DEVICE = "/dev/plox";
pid_t currentApplication = -1;

static void sigint_handler(int signum) 
{
  INFO("Shutting down! {}", currentApplication);
  kill(currentApplication, SIGKILL);
  waitpid(currentApplication, NULL, 0);
}

int ServiceLibCServices(int sock)
{
	return 0;
}

int SendStartMessage(int sock)
{
	int writeout = write(sock, "START", 5);
	if (writeout < 0) {
		perror("issue");
		return -1;
	}

	return 0;
}

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

	auto policy = PloxPolicy("policyfile");

  struct PloxProgCommand prog;
	int readin = read(connection, &prog, sizeof(prog));
	if (readin < 0) {
		perror("issue");
		close(connection);
		return;
	}

	std::string program = std::string(prog.args[0], strlen(prog.args[0]));

	int error = socketpair(AF_LOCAL, SOCK_STREAM, 0, pairs);
	if (error) {
		perror("Issue with socket");
		close(connection);
		return;
	}


	std::vector<std::string> args;
	std::vector<char *> argv;

	std::vector<std::string> env;
	std::vector<char *> envp;

	args.push_back(program);

  for(int i = 1; i < prog.NumArgs; i++) {
    args.push_back(std::string(prog.args[i], strlen(prog.args[i])));
  }

	for (auto &arg: args) {
		argv.push_back((char *)arg.c_str());
	}

	argv.push_back(nullptr);

	fcntl(pairs[0], F_SETFD, FD_CLOEXEC);

	env.push_back("LD_PRELOAD=/home/ryan/ploxd/build/src/shim/libshim.so");
	snprintf(buffer, 1024, "PLOXD_SOCKET=%d", pairs[1]);
	env.push_back(std::string(buffer, strlen(buffer)));
	for (auto &e : env) {
		envp.push_back((char *)e.c_str());
	}
	envp.push_back(nullptr);

	int pid = fork();
	if (pid) {
		int error = policy.setupPolicy(pid);
		if (error) {
			ERROR("Problem setting up policy");
			return;
		}

    currentApplication = pid;

		error = SendStartMessage(pairs[0]);
		if (error) {
			ERROR("Problem with sending message to container");
			return;
		}

		ServiceLibCServices(pairs[0]);
    pid_t wpid;
    int status = 0;
    while((wpid = wait(&status)) > 0);

	} else {
    // Wait for confirmation from containerd
    int readin = read(pairs[1], buffer, 1024);
    if (readin < 0) {
      perror("[Container] Error getting confirmation, shutting down");
      return;
    }

		if (execve(argv[0], argv.data(), envp.data()) == -1) {
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

  if (signal(SIGINT, &sigint_handler) == SIG_ERR) {
    perror("Error setting up sigkill handler");
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
				HandleConnection(connection);
			}
		}
	}

	return 0;
}
