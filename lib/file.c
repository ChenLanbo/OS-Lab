#include <inc/fs.h>
#include <inc/string.h>
#include <inc/lib.h>
#include <inc/assert.h>

#define DEBUG_FILE 0

extern union Fsipc fsipcbuf;	// page-aligned, declared in entry.S

// Send an inter-environment request to the file server, and wait for
// a reply.  The request body should be in fsipcbuf, and parts of the
// response may be written back to fsipcbuf.
// type: request code, passed as the simple integer IPC value.
// dstva: virtual address at which to receive reply page, 0 if none.
// Returns result from the file server.
static int
fsipc(unsigned type, void *dstva)
{
	// LOG(DEBUG_FILE, "[%08x] fsipc %d %08x\n", env->env_id, type, *(uint32_t *)&fsipcbuf);
	ipc_send(envs[1].env_id, type, &fsipcbuf, PTE_P | PTE_W | PTE_U);
	return ipc_recv(NULL, dstva, NULL);
}

static int devfile_flush(struct Fd *fd);
static ssize_t devfile_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devfile_write(struct Fd *fd, const void *buf, size_t n);
static int devfile_stat(struct Fd *fd, struct Stat *stat);
static int devfile_trunc(struct Fd *fd, off_t newsize);

struct Dev devfile =
{
	.dev_id =	'f',
	.dev_name =	"file",
	.dev_read =	devfile_read,
	.dev_write =	devfile_write,
	.dev_close =	devfile_flush,
	.dev_stat =	devfile_stat,
	.dev_trunc =	devfile_trunc
};

// Open a file (or directory).
//
// Returns:
// 	The file descriptor index on success
// 	-E_BAD_PATH if the path is too long (>= MAXPATHLEN)
// 	< 0 for other errors.
int
open(const char *path, int mode)
{
	int r, l1, l2;
	char real_path[MAXPATHLEN];
	struct Fd *pfd;
	// Find an unused file descriptor page using fd_alloc.
	// Then send a file-open request to the file server.
	// Include 'path' and 'omode' in request,
	// and map the returned file descriptor page
	// at the appropriate fd address.
	// FSREQ_OPEN returns 0 on success, < 0 on failure.
	//
	// (fd_alloc does not allocate a page, it just returns an
	// unused fd address.  Do you need to allocate a page?)
	//
	// Return the file descriptor index.
	// If any step after fd_alloc fails, use fd_close to free the
	// file descriptor.

	if (strlen(path) >= MAXPATHLEN){
		return -E_BAD_PATH;
	}

	memset(real_path, 0, sizeof(real_path));
	if (path[0] == '/'){
		strcpy(real_path, path);
	} else {
		sys_env_get_curdir(0, real_path);
		l1 = strlen(real_path);
		l2 = strlen(path);
		if (l1 + l2 >= MAXPATHLEN){
			return -E_BAD_PATH;
		}
		if (real_path[l1-1] == '/'){
			strcpy(real_path + l1, path);
		} else {
			real_path[l1] = '/';
			strcpy(real_path + l1 + 1, path);
		}
	}
	LOG(DEBUG_FILE, "*** OPEN FILE: %s\n", real_path);

	if ((r = fd_alloc(&pfd)) < 0){
		return -E_MAX_OPEN;
	}
	memset(fsipcbuf.open.req_path, 0, MAXPATHLEN);
	strncpy(fsipcbuf.open.req_path, real_path, strlen(real_path));
	fsipcbuf.open.req_omode = mode;

	if ((r = fsipc(FSREQ_OPEN, pfd)) < 0){
		fd_close(pfd, 1);
		return r;
	}
	pfd->fd_dev_id = 'f';
	return fd2num(pfd);
}

// Flush the file descriptor.  After this the fileid is invalid.
//
// This function is called by fd_close.  fd_close will take care of
// unmapping the FD page from this environment.  Since the server uses
// the reference counts on the FD pages to detect which files are
// open, unmapping it is enough to free up server-side resources.
// Other than that, we just have to make sure our changes are flushed
// to disk.
static int
devfile_flush(struct Fd *fd)
{
	fsipcbuf.flush.req_fileid = fd->fd_file.id;
	return fsipc(FSREQ_FLUSH, NULL);
}

// Read at most 'n' bytes from 'fd' at the current position into 'buf'.
//
// Returns:
// 	The number of bytes successfully read.
// 	< 0 on error.
static ssize_t
devfile_read(struct Fd *fd, void *buf, size_t n)
{
	// Make an FSREQ_READ request to the file system server after
	// filling fsipcbuf.read with the request arguments.  The
	// bytes read will be written back to fsipcbuf by the file
	// system server.
	int r;
	struct Fd *fd_store;
	// if ((r = fd_lookup(fd->fd_file.id, &fd_store)) < 0){ return r; }

	fsipcbuf.read.req_fileid = fd->fd_file.id;
	fsipcbuf.read.req_n = n;
	if ((r = fsipc(FSREQ_READ, NULL)) < 0){
		return r;
	}
	memmove(buf, fsipcbuf.readRet.ret_buf, r);
	return r;
}

// Write at most 'n' bytes from 'buf' to 'fd' at the current seek position.
//
// Returns:
//	 The number of bytes successfully written.
//	 < 0 on error.
static ssize_t
devfile_write(struct Fd *fd, const void *buf, size_t n)
{
	// Make an FSREQ_WRITE request to the file system server.  Be
	// careful: fsipcbuf.write.req_buf is only so large, but
	// remember that write is always allowed to write *fewer*
	// bytes than requested.
	// LAB 5: Your code here
	int r;
	fsipcbuf.write.req_fileid = fd->fd_file.id;
	fsipcbuf.write.req_n = MIN(PGSIZE - (sizeof(int) + sizeof(size_t)), n);
	memmove(fsipcbuf.write.req_buf, buf, fsipcbuf.write.req_n);

	if ((r = fsipc(FSREQ_WRITE, NULL)) < 0){
		return r;
	}
	return r;
}

static int
devfile_stat(struct Fd *fd, struct Stat *st)
{
	int r;

	fsipcbuf.stat.req_fileid = fd->fd_file.id;
	if ((r = fsipc(FSREQ_STAT, NULL)) < 0)
		return r;
	strcpy(st->st_name, fsipcbuf.statRet.ret_name);
	st->st_size = fsipcbuf.statRet.ret_size;
	st->st_isdir = fsipcbuf.statRet.ret_isdir;
	return 0;
}

// Truncate or extend an open file to 'size' bytes
static int
devfile_trunc(struct Fd *fd, off_t newsize)
{
	fsipcbuf.set_size.req_fileid = fd->fd_file.id;
	fsipcbuf.set_size.req_size = newsize;
	return fsipc(FSREQ_SET_SIZE, NULL);
}

// Delete a file
int
remove(const char *path)
{
	if (strlen(path) >= MAXPATHLEN)
		return -E_BAD_PATH;
	strcpy(fsipcbuf.remove.req_path, path);
	return fsipc(FSREQ_REMOVE, NULL);
}

// Synchronize disk with buffer cache
int
sync(void)
{
	// Ask the file server to update the disk
	// by writing any dirty blocks in the buffer cache.

	return fsipc(FSREQ_SYNC, NULL);
}

