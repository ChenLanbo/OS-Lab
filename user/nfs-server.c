#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

#define debug 0

#define PORT 128
#define MAXPENDING 8
#define BUFFSIZE 512

static void
die(char *m)
{
	cprintf("%s\n", m);
	exit();
}

static void
handle_client(int sockfd)
{
	int r, filefd;
	char buffer[BUFFSIZE];

	r = read(sockfd, buffer, BUFFSIZE);
	cprintf("Get %d bytes: %s\n", r, buffer);

	if ((filefd = open("helloworld", O_RDONLY)) < 0){
		cprintf("open error\n");
		close(sockfd);
		return ;
	}
	while (1){
		memset(buffer, 0, sizeof(buffer));
		if ((r = read(filefd, buffer, BUFFSIZE)) <= 0){
			break;
		}
		// cprintf("File read %d bytes: %s\n", r, buffer);
		write(sockfd, buffer, r);
	}
	close(filefd);
	close(sockfd);
}

int
umain(void)
{
	int srv_sock, cli_sock;
	unsigned int len;
	struct sockaddr_in srvaddr, cliaddr;

	binaryname = "nfs-server";

	if ((srv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		die("Failed to create socket");
	memset(&srvaddr, 0, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(128);

	if (bind(srv_sock, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0){
		die("Failed to bind the server socket");
	}

	// Listen on the server socket
	if (listen(srv_sock, MAXPENDING) < 0)
		die("Failed to listen on server socket");

	cprintf("Waiting for nfs connections...\n");

	while (1) {
		len = sizeof(cliaddr);
		if ((cli_sock = accept(srv_sock, (struct sockaddr *)&cliaddr, &len)) < 0){
			die("Failed to accept client connection");
		}
		handle_client(cli_sock);
	}
	close(srv_sock);
	return 0;
}
