#include <inc/nfsserver.h>
#include <inc/assert.h>
#define DEBUG_NFS_SERVER 0

static void
die(char *m)
{
	cprintf("%s\n", m);
	exit();
}

// convert str to interger
// plen: number of characters read
int
atoi(char *str, int *plen)
{
	int i, ret = 0;
	for (i = 0; str[i] != '\0'; i++){
		if (!isdigit(str[i]))
			break;
		ret = ret * 10 + (str[i] - '0');
	}
	*plen = i;
	return ret;
}

// Mount a file on server to the client
void
exe_mount(int sockfd, char *req)
{
	int r, filefd;
	char buffer[BUFFSIZE];
	struct Stat *pstat = malloc(sizeof(struct Stat));

	LOG(DEBUG_NFS_SERVER, "\nIn exe_mount %s\n", req);
	memset(buffer, 0, sizeof(buffer));
	if (stat(req, pstat) < 0){
		r = 0;
	}
	else {
		r = 1;
	}

	if (r){
		snprintf(buffer, BUFFSIZE, "%d %c %d", r, (pstat->st_isdir ? 'D' : 'F'), pstat->st_size);
	}
	else {
		snprintf(buffer, BUFFSIZE, "%d", r);
	}
	LOG(DEBUG_NFS_SERVER, "\nMount %s: %d --- %s\n", req, r, buffer);

	write(sockfd, buffer, strlen(buffer));
}

void
exe_loopup(int sockfd, char *path)
{
	int r, filefd;
	char buffer[BUFFSIZE];

	memset(buffer, 0, sizeof(buffer));
	if ((filefd = open(path, O_RDONLY)) <= 0){
		buffer[0] = 'N';
		r = 0;
	} else {
		buffer[0] = 'Y';
		r = 1;
	}

	LOG(DEBUG_NFS_SERVER, "\nLookup %s: %d\n", path, r); 
	write(sockfd, buffer, strlen(buffer)); 
	if (filefd > 0){
		close(filefd);
	}
}

void
exe_open(int sockfd, char *path)
{
	int r, filefd;
	char buffer[BUFFSIZE];

	memset(buffer, 0, sizeof(buffer));
	if ((filefd = open(path, O_RDONLY)) <= 0){
		r = 0;
	} else {
		r = 1;
	}

	snprintf(buffer, BUFFSIZE, "%d", r);
	LOG(DEBUG_NFS_SERVER, "\nOpen %s: %d\n", path, r);
	write(sockfd, buffer, strlen(buffer));
	if (filefd > 0){
		close(filefd);
	}
}

// READ
// R VER OFFSET LENGTH PATH
void
exe_read(int sockfd, char *req)
{
	int ver, off, len, cnt;
	int filefd, r, i;
	char *path;
	char buffer[BUFFSIZE];
	char *temp = malloc(BUFFSIZE);

	ver = atoi(req, &cnt);
	req += cnt + 1;

	off = atoi(req, &cnt);
	req += cnt + 1;

	len = atoi(req, &cnt);
	req += cnt + 1;
	path = req;

	LOG(DEBUG_NFS_SERVER, "\nRead %s : off %d, len %d\n", path, off, len);
	assert(len < BUFFSIZE);

	memset(buffer, 0, sizeof(buffer));
	// Open
	if ((filefd = open(path, O_RDONLY)) <= 0){
		r = 0;
	} else {
		r = 1;
	}
	// Seek
	if (seek(filefd, off) < 0){
		r = 0;
	} else {
		r = 1;
	}
	// Read
	if (r){
		r = read(filefd, temp, len);
		cnt = snprintf(buffer, BUFFSIZE, "%d", r);
		buffer[cnt] = ' ';
		for (i = 0; i < r; i++){
			buffer[cnt + 1 + i] = temp[i];
		}
	} else {
		buffer[0] = '-';
		buffer[1] = '1';
	}
	LOG(DEBUG_NFS_SERVER, "\nRead get %s\n", buffer);

	write(sockfd, buffer, sizeof(buffer));
	free(temp);
	close(filefd);
}

// Write
// W VER PATH OFFSET LENGTH CONTENT
void
exe_write(int sockfd, char *req)
{
	int ver, off, len, cnt;
	int filefd, r;
	int flag;
	char *path;
	char buffer[BUFFSIZE];

	// version
	ver = atoi(req, &cnt);
	req += cnt + 1;

	path = req;
	// path
	memset(buffer, 0, sizeof(buffer));
	for (cnt = 0; ; cnt++){
		if (isalpha(path[cnt]) || isdigit(path[cnt]) || path[cnt] == '/' || path[cnt] == '_'){
			buffer[cnt] = path[cnt];
		} else {
			break;
		}
	}
	req += cnt + 1;

	// offset
	off = atoi(req, &cnt);
	req += cnt + 1;

	// length
	len = atoi(req, &cnt);
	req += cnt + 1;
	path = req;

	LOG(DEBUG_NFS_SERVER, "\nIn exe_write %s: ver %d, off %d, len %d\n", buffer, ver, off, len);

	// Open
	if ((filefd = open(buffer, O_RDWR)) <= 0){
		flag = 0;
	} else {
		flag = 1;
	}
	// Seek
	if (seek(filefd, off) < 0){
		flag = 0;
	} else {
		flag = 1;
	}
	// Write
	memset(buffer, 0, sizeof(buffer));
	if (flag){
		r = write(filefd, req, len);
		buffer[0] = 'Y';
	} else {
		buffer[0] = 'N';
	}

	write(sockfd, buffer, strlen(buffer));
	if (filefd > 0)
		close(filefd);
}

// Remove
// D VER PATH
void
exe_remove(int sockfd, char *req)
{
	int r, ver, len;
	char buffer[BUFFSIZE];

	LOG(DEBUG_NFS_SERVER, "\n***INFO***: in exe_remove\n");
	ver = atoi(req, &len);
	req += len + 1;
	LOG(DEBUG_NFS_SERVER, "\n***INFO***: file to remove: %s\n", req);

	memset(buffer, 0, sizeof(buffer));
	if ((r = remove(req)) == 0){
		buffer[0] = 'Y';
	} else {
		buffer[0] = 'N';
	}
	write(sockfd, buffer, sizeof(buffer));
}

void
exe_stat(int sockfd, char *req)
{
	int r, ver, len;
	char buffer[BUFFSIZE];
	struct Stat *pstat = malloc(sizeof(struct Stat));

	ver = atoi(req, &len);
	req += len + 1;

	memset(buffer, 0, sizeof(buffer));
	if ((r = stat(req, pstat)) < 0){
		buffer[0] = 'E';
	} else {
		if (pstat->st_isdir){
			buffer[0] = 'D';
		} else {
			buffer[0] = 'F';
		}
		buffer[1] = ' ';
		snprintf(buffer + 2, BUFFSIZE, "%d", pstat->st_size);
	}
	LOG(DEBUG_NFS_SERVER, "\n***INFO***: Stat buffer %d\n", strlen(buffer));
	write(sockfd, buffer, sizeof(buffer));
	free(pstat);
}

void
exe_trun(int sockfd, char *req)
{
	int ver, off, len, cnt;
	int filefd, r;
	int flag;
	char *path;
	char buffer[BUFFSIZE];

	ver = atoi(req, &cnt);
	req += cnt + 1;

	path = req;
	memset(buffer, 0, sizeof(buffer));
	for (cnt = 0; ; cnt++){
		if (isalpha(path[cnt]) || isdigit(path[cnt]) || path[cnt] == '/' || path[cnt] == '_'){
			buffer[cnt] = path[cnt];
		} else {
			break;
		}
	}
	req += cnt + 1;

	len = atoi(req, &cnt);
	req += cnt + 1;

	LOG(DEBUG_NFS_SERVER, "\nTruncate --- VER %d PATH %s SIZE %d\n", ver, buffer, len);

	if ((filefd = open(buffer, O_RDWR)) < 0){
		r = 0;
	} else {
		r = 1;
	}

	if (ftruncate(filefd, len) < 0){
		r = 0;
	} else {
		r = 1;
	}

	memset(buffer, 0, sizeof(buffer));
	if (r){
		buffer[0] = 'Y';
	} else {
		buffer[0] = 'N';
	}

	write(sockfd, buffer, strlen(buffer));
	if (filefd > 0)
		close(filefd);
}

static void
handle_client(int sockfd)
{
	int r, filefd;
	char buffer[BUFFSIZE];

	memset(buffer, 0, sizeof(buffer));
	if ((r = read(sockfd, buffer, BUFFSIZE)) < 1){
		LOG(DEBUG_NFS_SERVER, "\n***ERROR***: socket read error\n");
		close(sockfd);
		return ;
	}

	LOG(DEBUG_NFS_SERVER, "\n***INFO***: NFS Server Get %d bytes data from client: %s\n", r, buffer);

	switch(buffer[0]){
		// Mount a file on server to the client
		// Format: M_PATH
		case 'M':
			exe_mount(sockfd, buffer + 2);
			break;
		case 'L':
			exe_loopup(sockfd, buffer + 2);
			break;
		case 'O':
			exe_open(sockfd, buffer + 2);
			break;
		case 'R':
			exe_read(sockfd, buffer + 2);
			break;
		case 'W':
			exe_write(sockfd, buffer + 2);
			break;
		case 'T':
			exe_trun(sockfd, buffer + 2);
			break;
		case 'D':
			exe_remove(sockfd, buffer + 2);
			break;
		case 'S':
			exe_stat(sockfd, buffer + 2);
			break;
		default:
			LOG(DEBUG_NFS_SERVER, "\n***ERROR***: unknown nfs command\n");
			break;
	}
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

	// Bind
	memset(&srvaddr, 0, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(SERVER_PORT);
	if (bind(srv_sock, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0){
		die("Failed to bind the server socket");
	}

	// Listen on the server socket
	if (listen(srv_sock, BACKUP) < 0)
		die("Failed to listen on server socket");

	LOG(1, "\n          NFS Server has been setup\n******** Waiting for nfs connections ********\n");

	// Waiting for connections
	while (1) {
		len = sizeof(cliaddr);
		if ((cli_sock = accept(srv_sock, (struct sockaddr *)&cliaddr, &len)) < 0){
			die("Failed to accept client connection");
		}
		LOG(DEBUG_NFS_SERVER, "\n***INFO***: nfs server gets a new connection\n");
		handle_client(cli_sock);
		close(cli_sock);
		LOG(DEBUG_NFS_SERVER, "\n***INFO***: finish handle nfs client\n");
	}
	close(srv_sock);
	return 0;
}
