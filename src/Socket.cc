#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include "Socket.h"
#include "Threads.h"
#include "Printer.h"
#include "Main.h"

void Balau::Socket::SocketEvent::gotOwner(Task * task) {
    Printer::elog(E_SOCKET, "Arming SocketEvent at %p", this);
    if (!m_task) {
        Printer::elog(E_SOCKET, "...with a new task (%p)", task);
    } else if (task == m_task) {
        m_evt.start();
        return;
    } else {
        Printer::elog(E_SOCKET, "...with a new task (%p -> %p); stopping first", m_task, task);
        m_evt.stop();
    }
    m_task = task;
    m_evt.set(task->getLoop());
    m_evt.start();
}

struct DNSRequest {
    const char * name;
    const char * service;
    struct addrinfo * res;
    struct addrinfo * hints;
    Balau::Events::Async * evt;
    int error;
};

static Balau::String getErrorMessage() {
    Balau::String msg;
#ifdef _WIN32
    char * lpMsgBuf;
    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL)) {
        char * eol = strrchr(lpMsgBuf, '\n');
        if (eol)
            *eol = 0;
        msg = lpMsgBuf;
    } else {
        msg = "FormatMessage failed";
    }
    LocalFree(lpMsgBuf);
#else
    msg = strerror(errno);
#endif
    return msg;
}

#if defined(_WIN32) && !defined(AI_V4MAPPED)
// mingw32 is retarded.
#define AI_NUMERICSERV              0x00000008L
#define AI_ALL                      0x00000100L
#define AI_ADDRCONFIG               0x00000400L
#define AI_V4MAPPED                 0x00000800L
#define AI_NON_AUTHORITATIVE        0x00004000L
#define AI_SECURE                   0x00008000L
#define AI_RETURN_PREFERRED_NAMES   0x00010000L
#define AI_FQDN                     0x00020000L
#define AI_FILESERVER               0x00040000L

// and winXP is stupid

static const char * inet_ntop4(const unsigned char * src, char * dst, socklen_t size) {
    char out[INET_ADDRSTRLEN];

    int len = sprintf(out, "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
    if (len < 0) {
        return NULL;
    } else if (len > size) {
        errno = ENOSPC;
        return NULL;
    }
    return strcpy(dst, out);
}

static const char * inet_ntop6(const unsigned char * src, char * dst, socklen_t size) {
    char tmp[INET6_ADDRSTRLEN], *tp;
    struct { int base, len; } best, cur;
    unsigned int words[8];
    int i;

    memset(words, 0, sizeof(words));
    for (i = 0; i < 16; i += 2)
        words[i / 2] = (src[i] << 8) | src[i + 1];
    best.base = -1;
    cur.base = -1;
    for (i = 0; i < 8; i++) {
        if (words[i] == 0) {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;
            else
                cur.len++;
        } else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1) {
        if (best.base == -1 || cur.len > best.len)
            best = cur;
    }
    if (best.base != -1 && best.len < 2)
        best.base = -1;

    tp = tmp;
    for (i = 0; i < 8; i++) {
        if (best.base != -1 && i >= best.base && i < (best.base + best.len)) {
            if (i == best.base)
            *tp++ = ':';
            continue;
        }
        if (i != 0)
            *tp++ = ':';
        if (i == 6 && best.base == 0 && (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
            if (!inet_ntop4 (src + 12, tp, sizeof tmp - (tp - tmp)))
                return NULL;
            tp += strlen (tp);
            break;
        }

        int len = sprintf (tp, "%x", words[i]);
        if (len < 0)
            return NULL;
        tp += len;
    }
    if (best.base != -1 && (best.base + best.len) == 8)
        *tp++ = ':';
    *tp++ = '\0';

    if ((socklen_t) (tp - tmp) > size) {
        errno = ENOSPC;
        return NULL;
    }

    return strcpy(dst, tmp);
}

static const char * inet_ntop(int af, const void * src, char * dst, socklen_t size) {
    switch (af) {
    case AF_INET:
        return inet_ntop4((const unsigned char *) src, dst, size);
    case AF_INET6:
        return inet_ntop6((const unsigned char *) src, dst, size);
    default:
        errno = WSAEAFNOSUPPORT;
        WSASetLastError(errno);
        return NULL;
    }
}

#endif

#if 0
// TODO: use getaddrinfo_a, if available.
#else
class ResolverThread : public Balau::Thread, public Balau::AtStart {
  public:
      ResolverThread() : AtStart(8) { }
      virtual ~ResolverThread();
    void pushRequest(DNSRequest * req) { m_queue.push(req); }
  private:
    virtual void * proc();
    virtual void doStart();
    Balau::Queue<DNSRequest *> m_queue;
};

void ResolverThread::doStart() {
    threadStart();
}

ResolverThread::~ResolverThread() {
    DNSRequest req;
    memset(&req, 0, sizeof(req));
    pushRequest(&req);
}

void * ResolverThread::proc() {
    DNSRequest * req;
    DNSRequest stop;
    memset(&stop, 0, sizeof(stop));
    while (true) {
        req = m_queue.pop();
        if (memcmp(&stop, req, sizeof(stop)) == 0)
            break;
        Balau::Printer::elog(Balau::E_SOCKET, "Resolver thread got a request for `%s'", req->name);
        req->error = getaddrinfo(req->name, req->service, req->hints, &req->res);
        Balau::Printer::elog(Balau::E_SOCKET, "Resolver thread got an answer; sending signal");
        req->evt->trigger();
    }
    return NULL;
}
#endif

static ResolverThread resolverThread;

static DNSRequest resolveName(const char * name, const char * service = NULL, struct addrinfo * hints = NULL) {
    Balau::Events::Async evt;
    DNSRequest req;
    memset(&req, 0, sizeof(req));

    req.name = name;
    req.service = service;
    req.hints = hints;
    req.evt = &evt;
    Balau::Printer::elog(Balau::E_SOCKET, "Sending a request to the resolver thread");
    Balau::Task::prepare(&evt);
    resolverThread.pushRequest(&req);
    Balau::Task::yield(&evt);

    return req;
}

Balau::Socket::Socket() throw (GeneralException) : m_fd(socket(AF_INET6, SOCK_STREAM, 0)), m_connected(false), m_connecting(false), m_listening(false) {
    m_name = "Socket(unconnected)";
    Assert(m_fd >= 0);
    m_evtR = new SocketEvent(m_fd, EV_READ);
    m_evtW = new SocketEvent(m_fd, EV_WRITE);
#ifdef _WIN32
    u_long iMode = 1;
    ioctlsocket(m_fd, FIONBIO, &iMode);
#else
    fcntl(m_fd, F_SETFL, O_NONBLOCK);
#endif
    memset(&m_localAddr, 0, sizeof(m_localAddr));
    memset(&m_remoteAddr, 0, sizeof(m_remoteAddr));
    Printer::elog(E_SOCKET, "Creating a socket at %p", this);
}

Balau::Socket::Socket(int fd) : m_fd(fd), m_connected(true), m_connecting(false), m_listening(false)  {
    socklen_t len;

    len = sizeof(m_localAddr);
    getsockname(m_fd, (sockaddr *) &m_localAddr, &len);

    len = sizeof(m_remoteAddr);
    getpeername(m_fd, (sockaddr *) &m_remoteAddr, &len);

    char prtLocal[INET6_ADDRSTRLEN], prtRemote[INET6_ADDRSTRLEN];
    const char * rLocal, * rRemote;

    len = sizeof(m_localAddr);
    rLocal = inet_ntop(AF_INET6, &m_localAddr.sin6_addr, prtLocal, len);
    rRemote = inet_ntop(AF_INET6, &m_remoteAddr.sin6_addr, prtRemote, len);

    Assert(rLocal);
    Assert(rRemote);

    m_evtR = new SocketEvent(m_fd, EV_READ);
    m_evtW = new SocketEvent(m_fd, EV_WRITE);
#ifdef _WIN32
    u_long iMode = 1;
    ioctlsocket(m_fd, FIONBIO, &iMode);
#else
    fcntl(m_fd, F_SETFL, O_NONBLOCK);
#endif

    m_name.set("Socket(Connected - [%s]:%i <- [%s]:%i)", rLocal, ntohs(m_localAddr.sin6_port), rRemote, ntohs(m_remoteAddr.sin6_port));
    Printer::elog(E_SOCKET, "Created a new socket from listener at %p; %s", this, m_name.to_charp());
}

void Balau::Socket::close() throw (GeneralException) {
#ifdef _WIN32
    closesocket(m_fd);
    WSACleanup();
#else
    ::close(m_fd);
#endif
    Printer::elog(E_SOCKET, "Closing socket at %p", this);
    m_connected = false;
    m_connecting = false;
    m_listening = false;
    m_fd = -1;
    delete m_evtR;
    delete m_evtW;
    m_evtR = m_evtW = NULL;
}

bool Balau::Socket::isClosed() { return m_fd < 0; }
bool Balau::Socket::isEOF() { return isClosed(); }
bool Balau::Socket::canRead() { return true; }
bool Balau::Socket::canWrite() { return true; }
const char * Balau::Socket::getName() { return m_name.to_charp(); }

bool Balau::Socket::setLocal(const char * hostname, int port) {
    Assert(m_localAddr.sin6_family == 0);

    if (hostname && hostname[0]) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;

        DNSRequest req = resolveName(hostname, NULL, &hints);
        struct addrinfo * res = req.res;
        if (req.error != 0) {
            freeaddrinfo(res);
            return false;
        }
        if (!res) {
            freeaddrinfo(res);
            return false;
        }
        Assert(res->ai_family == AF_INET6);
        Assert(res->ai_protocol == IPPROTO_TCP);
        Assert(res->ai_addrlen == sizeof(sockaddr_in6));
        memcpy(&m_localAddr.sin6_addr, &((sockaddr_in6 *) res->ai_addr)->sin6_addr, sizeof(struct in6_addr));
        freeaddrinfo(res);
    } else {
        m_localAddr.sin6_addr = in6addr_any;
    }

    if (port)
        m_localAddr.sin6_port = htons(port);

    m_localAddr.sin6_family = AF_INET6;
#ifndef _WIN32
    int enable = 1;
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
#endif
    return bind(m_fd, (struct sockaddr *) &m_localAddr, sizeof(m_localAddr)) == 0;
}

#if defined(_WIN32) && !defined(EISCONN)
#define EISCONN WSAEISCONN
#endif

bool Balau::Socket::connect(const char * hostname, int port) {
    Assert(!m_listening);
    Assert(!m_connected);
    Assert(hostname);
    Assert(!isClosed());

    if (!m_connecting) {
        Printer::elog(E_SOCKET, "Resolving %s", hostname);
        Assert(m_remoteAddr.sin6_family == 0);

        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;

        DNSRequest req = resolveName(hostname, NULL, &hints);
        struct addrinfo * res = req.res;
        if (req.error != 0) {
            freeaddrinfo(res);
            return false;
        }
        if (!res) {
            freeaddrinfo(res);
            return false;
        }
        Printer::elog(E_SOCKET, "Got a resolution answer");
        Assert(res->ai_family == AF_INET6);
        Assert(res->ai_protocol == IPPROTO_TCP);
        Assert(res->ai_addrlen == sizeof(sockaddr_in6));
        memcpy(&m_remoteAddr.sin6_addr, &((sockaddr_in6 *) res->ai_addr)->sin6_addr, sizeof(struct in6_addr));

        m_remoteAddr.sin6_port = htons(port);
        m_remoteAddr.sin6_family = AF_INET6;

        m_connecting = true;

        freeaddrinfo(res);
    } else {
        // if we end up there, it means our yield earlier thrown a EAgain exception.
        Assert(m_evtR->gotSignal());
    }

    int spins = 0;

    do {
        Printer::elog(E_SOCKET, "Connecting now...");
        int r = ::connect(m_fd, (sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr));
        if ((r == 0) || ((r < 0) && (errno == EISCONN))) {
            m_connected = true;
            m_connecting = false;

            socklen_t len;

            len = sizeof(m_localAddr);
            getsockname(m_fd, (sockaddr *) &m_localAddr, &len);

            len = sizeof(m_remoteAddr);
            getpeername(m_fd, (sockaddr *) &m_remoteAddr, &len);

            char prtLocal[INET6_ADDRSTRLEN], prtRemote[INET6_ADDRSTRLEN];
            const char * rLocal, * rRemote;

            len = sizeof(m_localAddr);
            rLocal = inet_ntop(AF_INET6, &m_localAddr.sin6_addr, prtLocal, len);
            rRemote = inet_ntop(AF_INET6, &m_remoteAddr.sin6_addr, prtRemote, len);

            Assert(rLocal);
            Assert(rRemote);

            m_name.set("Socket(Connected - [%s]:%i -> [%s]:%i)", rLocal, ntohs(m_localAddr.sin6_port), rRemote, ntohs(m_remoteAddr.sin6_port));
            Printer::elog(E_SOCKET, "Connected; %s", m_name.to_charp());

            m_evtW->stop();
            return true;
        }

#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
        if (errno != EINPROGRESS) {
#endif
            Printer::elog(E_SOCKET, "Connect() failed with the following error code: %i (%s)", errno, strerror(errno));
            return false;
        } else {
            Assert(spins == 0);
        }

        Task::yield(m_evtW, true);
        // if we're still here, it means the parent task doesn't want to be thrown an exception
        Assert(m_evtW->gotSignal());

    } while (spins++ < 2);

    return false;
}

bool Balau::Socket::listen() {
    Assert(!m_listening);
    Assert(!m_connecting);
    Assert(!m_connected);
    Assert(!isClosed());

    if (::listen(m_fd, 16) == 0) {
        m_listening = true;

        socklen_t len;

        len = sizeof(m_localAddr);
        getsockname(m_fd, (sockaddr *) &m_localAddr, &len);

        char prtLocal[INET6_ADDRSTRLEN];
        const char * rLocal;

        len = sizeof(m_localAddr);
        rLocal = inet_ntop(AF_INET6, &m_localAddr.sin6_addr, prtLocal, len);

        Assert(rLocal);

        m_name.set("Socket(Listener - [%s]:%i)", rLocal, ntohs(m_localAddr.sin6_port));
        Printer::elog(E_SOCKET, "Socket %i started to listen: %s", m_fd, m_name.to_charp());
    } else {
        String msg = getErrorMessage();
        Printer::elog(E_SOCKET, "listen() failed with error #i (%s)", errno, msg.to_charp());
    }

    return m_listening;
}

#ifdef _WIN32
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif
#endif

Balau::IO<Balau::Socket> Balau::Socket::accept() throw (GeneralException) {
    Assert(m_listening);
    Assert(m_fd >= 0);

    while(true) {
        sockaddr_in6 remoteAddr;
        socklen_t len;
        Printer::elog(E_SOCKET, "Socket %i (%s) is going to accept()", m_fd, m_name.to_charp());
        int s = ::accept(m_fd, (sockaddr *) &remoteAddr, &len);

        if (s < 0) {
            if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
                Task::yield(m_evtR, true);
            } else {
                String msg = getErrorMessage();
                throw GeneralException(String("Unexpected error accepting a connection: #") + errno + "(" + msg + ")");
            }
        } else {
            Printer::elog(E_SOCKET, "Listener at %p got a new connection", this);
            m_evtR->stop();
            return IO<Socket>(new Socket(s));
        }
    }
}

ssize_t Balau::Socket::read(void * buf, size_t count) throw (GeneralException) {
    if (count == 0)
        return 0;

    Assert(m_connected);
    Assert(m_fd >= 0);

    int spins = 0;

    do {
        ssize_t r = ::recv(m_fd, (char *) buf, count, 0);

        if (r >= 0) {
            if (r == 0)
                close();
            return r;
        }

        if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
            Task::yield(m_evtR, true);
        } else {
            m_evtR->stop();
            return r;
        }
    } while (spins++ < 2);

    return -1;
}

ssize_t Balau::Socket::write(const void * buf, size_t count) throw (GeneralException) {
    if (count == 0)
        return 0;

    Assert(m_connected);
    Assert(m_fd >= 0);

    int spins = 0;

    do {
        ssize_t r = ::send(m_fd, (const char *) buf, count, 0);

        Assert(r != 0);

        if (r > 0)
            return r;

        if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
            Task::yield(m_evtW, true);
        } else {
            m_evtW->stop();
            return r;
        }
    } while (spins++ < 2);

    return -1;
}
