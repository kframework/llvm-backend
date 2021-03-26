#include <gmp.h>
#include <cstring>
#include <stdexcept>
#include <map>
#include <fcntl.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <cerrno>
#include <sys/wait.h>
#include <sys/socket.h>
#include <time.h>

#include "runtime/alloc.h"
#include "runtime/header.h"

extern "C" {

#define KCHAR char
#define GETTAG(symbol) "Lbl'Hash'" #symbol "{}"
#define IOBUFSIZE 1024

  mpz_ptr move_int(mpz_t);
  char * getTerminatedString(string * str);

  static block * dotK = leaf_block(getTagForSymbolName("dotk{}"));
  static blockheader kseqHeader = {getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("kseq{}"))};

  static std::map<std::string, std::string> logFiles;

  static block * block_errno() {
    const char * errStr;
    switch (errno) {
    case EOF: errStr = GETTAG(EOF); break;
    case E2BIG: errStr = GETTAG(E2BIG); break;
    case EACCES: errStr = GETTAG(EACCES); break;
    case EAGAIN: errStr = GETTAG(EAGAIN); break;
    case EBADF: errStr = GETTAG(EBADF); break;
    case EBUSY: errStr = GETTAG(EBUSY); break;
    case ECHILD: errStr = GETTAG(ECHILD); break;
    case EDEADLK: errStr = GETTAG(EDEADLK); break;
    case EDOM: errStr = GETTAG(EDOM); break;
    case EEXIST: errStr = GETTAG(EEXIST); break;
    case EFAULT: errStr = GETTAG(EFAULT); break;
    case EFBIG: errStr = GETTAG(EFBIG); break;
    case EINTR: errStr = GETTAG(EINTR); break;
    case EINVAL: errStr = GETTAG(EINVAL); break;
    case EIO: errStr = GETTAG(EIO); break;
    case EISDIR: errStr = GETTAG(EISDIR); break;
    case EMFILE: errStr = GETTAG(EMFILE); break;
    case EMLINK: errStr = GETTAG(EMLINK); break;
    case ENAMETOOLONG: errStr = GETTAG(ENAMETOOLONG); break;
    case ENFILE: errStr = GETTAG(ENFILE); break;
    case ENODEV: errStr = GETTAG(ENODEV); break;
    case ENOENT: errStr = GETTAG(ENOENT); break;
    case ENOEXEC: errStr = GETTAG(ENOEXEC); break;
    case ENOLCK: errStr = GETTAG(ENOLCK); break;
    case ENOMEM: errStr = GETTAG(ENOMEM); break;
    case ENOSPC: errStr = GETTAG(ENOSPC); break;
    case ENOSYS: errStr = GETTAG(ENOSYS); break;
    case ENOTDIR: errStr = GETTAG(ENOTDIR); break;
    case ENOTEMPTY: errStr = GETTAG(ENOTEMPTY); break;
    case ENOTTY: errStr = GETTAG(ENOTTY); break;
    case ENXIO: errStr = GETTAG(ENXIO); break;
    case EPERM: errStr = GETTAG(EPERM); break;
    case EPIPE: errStr = GETTAG(EPIPE); break;
    case ERANGE: errStr = GETTAG(ERANGE); break;
    case EROFS: errStr = GETTAG(EROFS); break;
    case ESPIPE: errStr = GETTAG(ESPIPE); break;
    case ESRCH: errStr = GETTAG(ESRCH); break;
    case EXDEV: errStr = GETTAG(EXDEV); break;
#if EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK: errStr = GETTAG(EWOULDBLOCK); break;
#endif 
    case EINPROGRESS: errStr = GETTAG(EINPROGRESS); break;
    case EALREADY: errStr = GETTAG(EALREADY); break;
    case ENOTSOCK: errStr = GETTAG(ENOTSOCK); break;
    case EDESTADDRREQ: errStr = GETTAG(EDESTADDRREQ); break;
    case EMSGSIZE: errStr = GETTAG(EMSGSIZE); break;
    case EPROTOTYPE: errStr = GETTAG(EPROTOTYPE); break;
    case ENOPROTOOPT: errStr = GETTAG(ENOPROTOOPT); break;
    case EPROTONOSUPPORT: errStr = GETTAG(EPROTONOSUPPORT); break;
    case ESOCKTNOSUPPORT: errStr = GETTAG(ESOCKTNOSUPPORT); break;
    case EOPNOTSUPP: errStr = GETTAG(EOPNOTSUPP); break;
    case EPFNOSUPPORT: errStr = GETTAG(EPFNOSUPPORT); break;
    case EAFNOSUPPORT: errStr = GETTAG(EAFNOSUPPORT); break;
    case EADDRINUSE: errStr = GETTAG(EADDRINUSE); break;
    case EADDRNOTAVAIL: errStr = GETTAG(EADDRNOTAVAIL); break;
    case ENETDOWN: errStr = GETTAG(ENETDOWN); break;
    case ENETUNREACH: errStr = GETTAG(ENETUNREACH); break;
    case ENETRESET: errStr = GETTAG(ENETRESET); break;
    case ECONNABORTED: errStr = GETTAG(ECONNABORTED); break;
    case ECONNRESET: errStr = GETTAG(ECONNRESET); break;
    case ENOBUFS: errStr = GETTAG(ENOBUFS); break;
    case EISCONN: errStr = GETTAG(EISCONN); break;
    case ENOTCONN: errStr = GETTAG(ENOTCONN); break;
    case ESHUTDOWN: errStr = GETTAG(ESHUTDOWN); break;
    case ETOOMANYREFS: errStr = GETTAG(ETOOMANYREFS); break;
    case ETIMEDOUT: errStr = GETTAG(ETIMEDOUT); break;
    case ECONNREFUSED: errStr = GETTAG(ECONNREFUSED); break;
    case EHOSTDOWN: errStr = GETTAG(EHOSTDOWN); break;
    case EHOSTUNREACH: errStr = GETTAG(EHOSTUNREACH); break;
    case ELOOP: errStr = GETTAG(ELOOP); break;
    case EOVERFLOW: errStr = GETTAG(EOVERFLOW); break;
    default:
      block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(mpz_ptr)));
      retBlock->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("Lbl'Hash'unknownIOError{}"));
      mpz_t err;
      mpz_init_set_si(err, errno);
      mpz_ptr p = move_int(err);
      memcpy(retBlock->children, &p, sizeof(mpz_ptr));
      return retBlock;
    }

    return leaf_block(getTagForSymbolName(errStr));
  }

  static blockheader header_int() {
    static blockheader header = {(uint64_t)-1};

    if (header.hdr == (uint64_t)-1) {
      header = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortInt{}, SortIOInt{}}"));
    }

    return header;
  }

  blockheader header_err() {
    static blockheader header = {(uint64_t)-1};

    if (header.hdr == (uint64_t)-1) {
      header = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortIOError{}, SortKItem{}}"));
    }

    return header;
  }

  static blockheader header_string() {
    static blockheader header = {(uint64_t)-1};

    if (header.hdr == (uint64_t)-1) {
      header = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortString{}, SortIOString{}}"));
    }

    return header;
  }

  static inline block * getKSeqErrorBlock() {
    block * err = block_errno();
    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + 2 * sizeof(block *)));
    block * inj = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
    retBlock->h = kseqHeader;
    inj->h = header_err();
    memcpy(inj->children, &err, sizeof(block *));
    memcpy(retBlock->children, &inj, sizeof(block *));
    memcpy(retBlock->children + 1, &dotK, sizeof(block *));
    return retBlock;
  }

  static inline block* getInjErrorBlock() {
    block * p = block_errno();
    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
    retBlock->h = header_err();
    memcpy(retBlock->children, &p, sizeof(block *));
    return retBlock;
  }

#define MODE_R 1
#define MODE_W 2
#define MODE_A 4
#define MODE_E 8
#define MODE_X 16
#define MODE_B 32
#define MODE_P 64

  static int getFileModes(string * modes) {
    int flags = 0;
    int length = len(modes);

    if (length <= 0) {
      return -1;
    }

    switch (modes->data[0]) {
      case 'r': flags |= MODE_R; break;
      case 'w': flags |= MODE_W; break;
      case 'a': flags |= MODE_A; break;
      default: return -1;
    }

    for (int i = 1; i < length; i++) {
      switch (modes->data[i]) {
        case 'e':
          flags |= (flags & MODE_E) ? -1 : MODE_E;
          break;
        case 'x':
          flags |= (flags & MODE_X) ? -1 : MODE_X;
          break;
        case 'b':
          flags |= (flags & MODE_B) ? -1 : MODE_B;
          break;
        case '+':
          flags |= (flags & MODE_P) ? -1 : MODE_P;
          break;
        default: return -1;
      }
    }

    return (flags < 0) ? -1 : flags;
  }

  SortIOInt hook_IO_open(SortString filename, SortString control) {
    int flags = 0;
    int access = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int modes = getFileModes(control);
    int fd;
    mpz_t result;

    if (-1 != modes) {
      switch (modes & 7) {
        case MODE_R:
          flags = (modes & MODE_P) ? O_RDWR : O_RDONLY;
          break;
        case MODE_W:
          flags = (modes & MODE_P) ? O_RDWR : O_WRONLY;
          flags |= O_TRUNC | O_CREAT;
          break;
        case MODE_A:
          flags = (modes & MODE_P) ? O_RDWR : O_WRONLY;
          flags |= O_APPEND | O_CREAT;
          break;
      }

      if (modes & MODE_E) {
        flags |= O_CLOEXEC;
      }

      if (modes & MODE_X) {
        flags |= O_EXCL;
      }

      if (modes & MODE_B) {
      }

      char * f = getTerminatedString(filename);
      fd = open(f, flags, access);
    } else {
      errno = EINVAL;
      fd = -1;
    }

    if (-1 == fd) {
      return getInjErrorBlock();
    }

    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(mpz_ptr)));
    retBlock->h = header_int();
    mpz_init_set_si(result, fd);
    mpz_ptr p = move_int(result);
    memcpy(retBlock->children, &p, sizeof(mpz_ptr));
    return retBlock;
  }

  SortIOInt hook_IO_tell(SortInt i) {
    if (!mpz_fits_sint_p(i)) {
      throw std::invalid_argument("Arg too large for int");
    }

    int fd = mpz_get_si(i);
    off_t loc = lseek(fd, 0, SEEK_CUR);

    if (-1 == loc) {
      return getInjErrorBlock();
    }

    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(mpz_ptr)));
    retBlock->h = header_int();
    mpz_t result;
    mpz_init_set_si(result, (long) loc);
    mpz_ptr p =  move_int(result);
    memcpy(retBlock->children, &p, sizeof(mpz_ptr));
    return retBlock;
  }

  SortIOInt hook_IO_getc(SortInt i) {
    if (!mpz_fits_sint_p(i)) {
      throw std::invalid_argument("Arg too large for int");
    }

    int fd = mpz_get_si(i);
    char c;
    ssize_t ret = read(fd, &c, sizeof(char));


    if (0 == ret) {
      block * p = leaf_block(getTagForSymbolName(GETTAG(EOF)));
      block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
      retBlock->h = header_err();
      memcpy(retBlock->children, &p, sizeof(block *));
      return retBlock;
    } else if (-1 == ret) {
      return getInjErrorBlock();
    }

    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(mpz_ptr)));
    retBlock->h = header_int();
    mpz_t result;
    mpz_init_set_si(result, (int) c);
    mpz_ptr p = move_int(result);
    memcpy(retBlock->children, &p, sizeof(mpz_ptr));
    return retBlock;
  }

  SortIOString hook_IO_read(SortInt i, SortInt len) {
    if (!mpz_fits_sint_p(i) || !mpz_fits_ulong_p(len)) {
      throw std::invalid_argument("Arg too large");
    }

    int fd = mpz_get_si(i);
    size_t length = mpz_get_ui(len);

    auto result = static_cast<string *>(koreAllocToken(sizeof(string) + length));
    int bytes = read(fd, &(result->data), length);

    if (-1 == bytes) {
      return getInjErrorBlock();
    }

    result = static_cast<string *>(koreResizeLastAlloc(result, sizeof(string) + bytes, sizeof(string) + length));
    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
    retBlock->h = header_string();
    set_len(result, (size_t)bytes);
    memcpy(retBlock->children, &result, sizeof(string *));
    return retBlock;
  }

  SortK hook_IO_close(SortInt i) {
    if (!mpz_fits_sint_p(i)) {
      throw std::invalid_argument("Arg too large for int");
    }

    int fd = mpz_get_si(i);
    int ret = close(fd);

    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  SortK hook_IO_seek(SortInt i, SortInt loc) {
    if (!mpz_fits_sint_p(i) || !mpz_fits_slong_p(loc)) {
      throw std::invalid_argument("Arg too large");
    }

    int fd = mpz_get_si(i);
    off_t l = mpz_get_si(loc);
    int ret = lseek(fd, l, SEEK_SET);

    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  SortK hook_IO_seekEnd(SortInt i, SortInt loc) {
    if (!mpz_fits_sint_p(i) || !mpz_fits_slong_p(loc)) {
      throw std::invalid_argument("Arg too large");
    }

    int fd = mpz_get_si(i);
    off_t l = mpz_get_si(loc);
    int ret = lseek(fd, l, SEEK_END);

    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  SortK hook_IO_putc(SortInt i, SortInt c) {
    if (!mpz_fits_sint_p(i) || !mpz_fits_sint_p(c)) {
      throw std::invalid_argument("Arg too large");
    }

    int fd = mpz_get_si(i);
    int ch = mpz_get_si(c);
    int ret = write(fd, &ch, 1);

    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  SortK hook_IO_write(SortInt i, SortString str) {
    if (!mpz_fits_sint_p(i)) {
      throw std::invalid_argument("Arg too large for int");
    }

    int fd = mpz_get_si(i);
    int ret = write(fd, str->data, len(str));

    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  SortK hook_IO_lock(SortInt i, SortInt len) {
    if (!mpz_fits_sint_p(i) || !mpz_fits_slong_p(len)) {
      throw std::invalid_argument("Arg too large");
    }

    int fd = mpz_get_si(i);
    off_t l = mpz_get_si(len);

    struct flock lockp = {0};
    lockp.l_type = F_WRLCK; lockp.l_whence = SEEK_CUR; lockp.l_start = 0; lockp.l_len = l;
    int ret = fcntl(fd, F_SETLKW, &lockp);

    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  SortK hook_IO_unlock(SortInt i, SortInt len) {
    if (!mpz_fits_sint_p(i) || !mpz_fits_slong_p(len)) {
      throw std::invalid_argument("Arg too large");
    }

    int fd = mpz_get_si(i);
    off_t l = mpz_get_si(len);

    struct flock lockp = {0};
    lockp.l_type = F_UNLCK; lockp.l_whence = SEEK_CUR; lockp.l_start = 0; lockp.l_len = l;
    int ret = fcntl(fd, F_SETLKW, &lockp);

    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  SortK hook_IO_remove(SortString path) {
    char * p = getTerminatedString(path);

    int ret = unlink(p);
    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  SortIOInt hook_IO_accept(SortInt sock) {
    if (!mpz_fits_sint_p(sock)) {
      throw std::invalid_argument("Arg too large");
    }

    int fd = mpz_get_si(sock);
    int clientsock = accept(fd, NULL, NULL);

    if (clientsock == -1) {
      return getInjErrorBlock();
    }

    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(mpz_ptr)));
    retBlock->h = header_int();

    mpz_t result;
    mpz_init_set_si(result, clientsock);
    mpz_ptr p = move_int(result);
    memcpy(retBlock->children, &p, sizeof(mpz_ptr));
    return retBlock;
  }

  SortK hook_IO_shutdownWrite(SortInt sock) {
    if (!mpz_fits_sint_p(sock)) {
      throw std::invalid_argument("Arg too large");
    }

    int fd = mpz_get_si(sock);
    int ret = shutdown(fd, SHUT_WR);

    if (ret == -1) {
      return getKSeqErrorBlock();
    }

    return dotK;
  }

  void flush_IO_logs() {
    std::string pid = std::to_string(getpid());
    for (auto const& log : logFiles) {
      std::string pathStr = log.first;
      std::string msg = log.second;
      size_t length = pathStr.length();
      char * path1 = (char *) malloc(sizeof(char) * (length + 1));
      strcpy(path1, pathStr.c_str());
      char * path2 = (char *) malloc(sizeof(char) * (length + 1));
      strcpy(path2, pathStr.c_str());
      char * dir = dirname(path1);
      if ( getenv("K_LOG_DIR") ) {
        dir = getenv("K_LOG_DIR");
      }
      char fulldir[PATH_MAX];
      if (!realpath(dir, fulldir)) {
        abort();
      }
      char * base = basename(path2);
      std::string prefix = "";
      if ( getenv("K_LOG_PREFIX") ) {
        prefix = getenv("K_LOG_PREFIX");
      }
      std::string fullPath = std::string(fulldir) + "/" + prefix + pid + "_" + std::string(base);
      FILE* f = fopen(fullPath.c_str(), "a+");
      fwrite(msg.c_str(), sizeof(char), msg.length(), f);
      fclose(f);
      free(path1);
      free(path2);
    }
  }

  SortK hook_IO_log(SortString path, SortString msg) {
    char * p = getTerminatedString(path);
    char * m = getTerminatedString(msg);

    static bool flushRegistered = false;
    if (!flushRegistered) {
      atexit(&flush_IO_logs);
      flushRegistered = true;
    }

    logFiles[p].append(m);

    return dotK;
  }

  block * hook_KREFLECTION_parseKAST(string *kast) {
    throw std::invalid_argument("not implemented: KREFLECTION.parseKast");
  }

  block * hook_KREFLECTION_configuration() {
    throw std::invalid_argument("not implemented: KREFLECTION.configuration");
  }

  string * hook_KREFLECTION_getenv(string * str) {
    throw std::invalid_argument("not implemented: KREFLECTION.getenv");
  }

  string * hook_KREFLECTION_sort(block * K) {
    throw std::invalid_argument("not implemented: KREFLECTION.sort");
  }

  block * hook_KREFLECTION_getKLabel(block * K) {
    throw std::invalid_argument("not implemented: KREFLECTION.getKLabel");
  }

  block * hook_KREFLECTION_fresh(string * str) {
    throw std::invalid_argument("not implemented: KREFLECTION.fresh");
  }

  int llvm_backend_argc = 0;
  char const ** llvm_backend_argv = nullptr;

  list hook_KREFLECTION_argv() {
    if (!llvm_backend_argv)
      throw std::invalid_argument("KREFLECTION.argv: no arguments given");

    list l{};

    for (int i = 0; i < llvm_backend_argc; i++) {
      stringbuffer * buf = hook_BUFFER_empty();
      buf = hook_BUFFER_concat_raw(buf, llvm_backend_argv[i], strlen(llvm_backend_argv[i]));
      SortString str = hook_BUFFER_toString(buf);
      block * b = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(str)));
      b->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortString{}, SortKItem{}}"));
      memcpy(b->children, &str, sizeof(str));
      l = l.push_back(KElem(b));
    }

    return l;
  }

  SortIOFile hook_IO_mkstemp(SortString filename) {
    char * temp = getTerminatedString(filename);
    int ret = mkstemp(temp);

    if (ret == -1) {
      return getInjErrorBlock();
    }

    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *) + sizeof(mpz_ptr)));

    mpz_t result;
    mpz_init_set_si(result, ret);
    mpz_ptr p = move_int(result);
    size_t length = len(filename);
    string * retString = static_cast<string *>(koreAllocToken(sizeof(string) + sizeof(char) * length));
    memcpy(retString->data, temp, sizeof(char) * length);
    set_len(retString, length);
    memcpy(retBlock->children, &retString, sizeof(string *));
    memcpy(retBlock->children + 1, &p, sizeof(mpz_ptr));
    retBlock->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(GETTAG(tempFile)));

    return retBlock;
  }

  SortKItem hook_IO_system(SortString cmd) {
    pid_t pid;
    int ret = 0, out[2], err[2];
    stringbuffer *outBuffer = hook_BUFFER_empty();
    stringbuffer *errBuffer = hook_BUFFER_empty();
    char buf[IOBUFSIZE];

    if (pipe(out) == -1 || pipe(err) == -1 || (pid = fork()) == -1) {
      return getKSeqErrorBlock();
    }

    if (pid == 0) {
      dup2(out[1], STDOUT_FILENO);
      close(out[0]);
      close(out[1]);
      dup2(err[1], STDERR_FILENO);
      close(err[0]);
      close(err[1]);

      if (len(cmd) > 0) {
        char * command = getTerminatedString(cmd);
        ret = execl("/bin/sh", "/bin/sh", "-c", command, NULL);
        ret == -1 ? exit(127) : exit(0);
      } else {
        ret = system(NULL);
        exit(ret);
      }
    }

    close(out[1]);
    close(err[1]);

    fd_set read_fds, ready_fds;
    FD_ZERO(&read_fds);
    FD_SET(out[0], &read_fds);
    FD_SET(err[0], &read_fds);

    int done = 0;

    while(done < 2) {
      ready_fds = read_fds;
      if (select(FD_SETSIZE, &ready_fds, NULL, NULL, NULL) == -1) {
        return getKSeqErrorBlock();
      }
      if (FD_ISSET(out[0], &ready_fds)) {
        int nread = read(out[0], buf, IOBUFSIZE);
        if (nread == -1) {
          return getKSeqErrorBlock();
        } else if (nread == 0) {
          FD_CLR(out[0], &read_fds);
	  done++;
        } else {
          hook_BUFFER_concat_raw(outBuffer, buf, nread);
        }
      }
      if (FD_ISSET(err[0], &ready_fds)) {
        int nread = read(err[0], buf, IOBUFSIZE);
        if (nread == -1) {
          return getKSeqErrorBlock();
        } else if (nread == 0) {
          FD_CLR(err[0], &read_fds);
          done++;
        } else {
          hook_BUFFER_concat_raw(errBuffer, buf, nread);
        }
      }
    }

    waitpid(pid, &ret, 0);
    ret = WEXITSTATUS(ret);

    block * retBlock = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(mpz_ptr) + sizeof(string *) + sizeof(string *)));

    mpz_t result;
    mpz_init_set_si(result, ret);
    mpz_ptr p = move_int(result);
    memcpy(retBlock->children, &p, sizeof(mpz_ptr));
    
    retBlock->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(GETTAG(systemResult)));
    string *outStr, *errStr;
    outStr = hook_BUFFER_toString(outBuffer);
    errStr = hook_BUFFER_toString(errBuffer);
    memcpy(retBlock->children + 1, &outStr, sizeof(string *));
    memcpy(retBlock->children + 2, &errStr, sizeof(string *));

    return retBlock;
  }

  block * hook_IO_stat(string * path) {
    throw std::invalid_argument("not implemented: IO.stat");
  }

  block * hook_IO_lstat(string * path) {
    throw std::invalid_argument("not implemented: IO.lstat");
  }

  block * hook_IO_opendir(string * path) {
    throw std::invalid_argument("not implemented: IO.opendir");
  }

  SortInt hook_IO_time() {
    mpz_t result;
    mpz_init_set_si(result, time(NULL));
    return move_int(result);
  }
}
