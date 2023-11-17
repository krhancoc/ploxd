#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "commands.h"

int 
ConnectToPloxd()
{
	// Create a Unix domain socket
	int client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (client_socket == -1) {
		return 1;
	}

	// Create a sockaddr_un structure to specify the socket path
	struct sockaddr_un address;
	memset(&address, 0, sizeof(struct sockaddr_un));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, SOCK_PATH, sizeof(address.sun_path) - 1);

	// Connect to the server
	if (connect(client_socket, (struct sockaddr*)&address, sizeof(struct sockaddr_un)) == -1) {
		perror("Problem with connection");
		return -1;
	}

	return client_socket;
}

int 
main(int argc, char *argv[])
{
	auto plx = ConnectToPloxd();
	if (plx < 0) {

		return -1;
	}
  struct PloxProgCommand prog;
  for (int i = 1; i < argc; i++) {
    memcpy(prog.args[i-1], argv[i], strlen(argv[i]) + 1);
  }
  prog.NumArgs = argc - 1;
  
	write(plx, &prog, sizeof(prog));

	return 0;
}
