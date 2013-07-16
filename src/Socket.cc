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
#include "Atomic.h"
#include "Async.h"
#include "Task.h"
#include "TaskMan.h"

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

namespace Balau {

struct DNSRequest {
    struct addrinfo * res;
    int error;
    Balau::Events::Custom evt;
};

};

namespace {

class AsyncOpResolv : public Balau::AsyncOperation {
  public:
      AsyncOpResolv(const char * name, const char * service, struct addrinfo * hints, Balau::DNSRequest * request)
        : m_name(name ? ::strdup(name) : NULL)
        , m_service(service ? ::strdup(service) : NULL)
        , m_hints(*hints)
        , m_request(request)
        { }
      virtual ~AsyncOpResolv() { free(m_name); free(m_service); }
    virtual bool needsMainQueue() { return false; }
    virtual bool needsFinishWorker() { return true; }
    virtual void run() {
        m_request->error = getaddrinfo(m_name, m_service, &m_hints, &m_request->res);
    }
    virtual void done() {
        m_request->evt.doSignal();
        delete this;
    }
  private:
    char * m_name;
    char * m_service;
    struct addrinfo m_hints;
    Balau::DNSRequest * m_request;
};

};

static Balau::DNSRequest * resolveName(const char * name, const char * service = NULL, struct addrinfo * hints = NULL) {
    Balau::DNSRequest * req = new Balau::DNSRequest();
    createAsyncOp(new AsyncOpResolv(name, service, hints, req));
    return req;
}

Balau::Socket::Socket() throw (GeneralException) {
    int fd = socket(AF_INET6, SOCK_STREAM, 0);

    m_name = "Socket(nonconnected)";
    RAssert(fd >= 0, "socket() returned %i", fd);

    setFD(fd);

    int on = 0;
    int r = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &on, sizeof(on));
    EAssert(r == 0, "setsockopt returned %i", r);

    memset(&m_localAddr, 0, sizeof(m_localAddr));
    memset(&m_remoteAddr, 0, sizeof(m_remoteAddr));
    Printer::elog(E_SOCKET, "Creating a socket at %p", this);
}

Balau::Socket::Socket(int fd) {
    socklen_t len;
    m_connected = true;

    len = sizeof(m_localAddr);
    getsockname(fd, (sockaddr *) &m_localAddr, &len);

    len = sizeof(m_remoteAddr);
    getpeername(fd, (sockaddr *) &m_remoteAddr, &len);

    char prtLocal[INET6_ADDRSTRLEN], prtRemote[INET6_ADDRSTRLEN];
    const char * rLocal, * rRemote;

    len = sizeof(m_localAddr);
    rLocal = inet_ntop(AF_INET6, &m_localAddr.sin6_addr, prtLocal, len);
    rRemote = inet_ntop(AF_INET6, &m_remoteAddr.sin6_addr, prtRemote, len);

    EAssert(rLocal, "inet_ntop returned NULL");
    EAssert(rRemote, "inet_ntop returned NULL");

    setFD(fd);

    m_name.set("Socket(Connected - [%s]:%i <- [%s]:%i)", rLocal, ntohs(m_localAddr.sin6_port), rRemote, ntohs(m_remoteAddr.sin6_port));
    Printer::elog(E_SOCKET, "Created a new socket from listener at %p; %s", this, m_name.to_charp());
}

void Balau::Socket::close() throw (GeneralException) {
    if (isClosed())
        return;
#ifdef _WIN32
    closesocket(getFD());
    WSACleanup();
#else
    ::close(getFD());
#endif
    Printer::elog(E_SOCKET, "Closing socket at %p", this);
    m_connected = false;
    m_connecting = false;
    m_listening = false;
    internalClose();
}

bool Balau::Socket::canRead() { return true; }
bool Balau::Socket::canWrite() { return true; }
const char * Balau::Socket::getName() { return m_name.to_charp(); }

bool Balau::Socket::resolved() {
    return m_req && m_req->evt.gotSignal();
}

bool Balau::Socket::setLocal(const char * hostname, int port) {
    AAssert(m_localAddr.sin6_family == 0, "Can't call setLocal twice");

    if (hostname && hostname[0] && !m_req) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_V4MAPPED;

        m_req = resolveName(hostname, NULL, &hints);
        Task::operationYield(&m_req->evt, Task::INTERRUPTIBLE);
    }

    if (m_req) {
        AAssert(m_req->evt.gotSignal(), "Please don't call setLocal after a EAgain without checking its resolution status first.");
        struct addrinfo * res = m_req->res;
        if (m_req->error != 0) {
            Printer::elog(E_SOCKET, "Got a resolution error for host %s: %s (%i)", hostname, gai_strerror(m_req->error), m_req->error);
            if (res)
                freeaddrinfo(res);
            delete m_req;
            m_req = NULL;
            return false;
        }
        IAssert(res, "That really shouldn't happen...");
        EAssert(res->ai_family == AF_INET6, "getaddrinfo returned a familiy which isn't AF_INET6; %i", res->ai_family);
        EAssert(res->ai_protocol == IPPROTO_TCP, "getaddrinfo returned a protocol which isn't IPPROTO_TCP; %i", res->ai_protocol);
        EAssert(res->ai_addrlen == sizeof(sockaddr_in6), "getaddrinfo returned an addrlen which isn't that of sizeof(sockaddr_in6); %i", res->ai_addrlen);
        memcpy(&m_localAddr.sin6_addr, &((sockaddr_in6 *) res->ai_addr)->sin6_addr, sizeof(struct in6_addr));
        freeaddrinfo(res);
        delete m_req;
        m_req = NULL;
    } else {
        m_localAddr.sin6_addr = in6addr_any;
    }

    if (port)
        m_localAddr.sin6_port = htons(port);

    m_localAddr.sin6_family = AF_INET6;
#ifndef _WIN32
    int enable = 1;
    setsockopt(getFD(), SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
#endif
    return bind(getFD(), (struct sockaddr *) &m_localAddr, sizeof(m_localAddr)) == 0;
}

#if defined(_WIN32) && !defined(EISCONN)
#define EISCONN WSAEISCONN
#endif

bool Balau::Socket::connect(const char * hostname, int port) {
    AAssert(!m_listening, "You can't call Socket::connect() on a listening socket");
    AAssert(!m_connected, "You can't call Socket::connect() on an already connected socket");
    AAssert(hostname, "You can't call Socket::connect() without a hostname");
    AAssert(!isClosed(), "You can't call Socket::connect() on a closed socket");

    if (!m_connecting && !m_req) {
        Printer::elog(E_SOCKET, "Resolving %s", hostname);
        IAssert(m_remoteAddr.sin6_family == 0, "That shouldn't happen...; family = %i", m_remoteAddr.sin6_family);

        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_V4MAPPED;

        m_req = resolveName(hostname, NULL, &hints);
        Task::operationYield(&m_req->evt, Task::INTERRUPTIBLE);
    }

    if (!m_connecting && m_req) {
        AAssert(m_req->evt.gotSignal(), "Please don't call connect after a EAgain without checking its resolution status first.");
        struct addrinfo * res = m_req->res;
        if (m_req->error != 0) {
            Printer::elog(E_SOCKET, "Got a resolution error for host %s: %s (%i)", hostname, gai_strerror(m_req->error), m_req->error);
            if (res)
                freeaddrinfo(res);
            delete m_req;
            m_req = NULL;
            return false;
        }
        IAssert(res, "That really shouldn't happen...");
        Printer::elog(E_SOCKET, "Got a resolution answer");
        EAssert(res->ai_family == AF_INET6, "getaddrinfo returned a familiy which isn't AF_INET6; %i", res->ai_family);
        EAssert(res->ai_protocol == IPPROTO_TCP, "getaddrinfo returned a protocol which isn't IPPROTO_TCP; %i", res->ai_protocol);
        EAssert(res->ai_addrlen == sizeof(sockaddr_in6), "getaddrinfo returned an addrlen which isn't that of sizeof(sockaddr_in6); %i", res->ai_addrlen);
        memcpy(&m_remoteAddr.sin6_addr, &((sockaddr_in6 *) res->ai_addr)->sin6_addr, sizeof(struct in6_addr));

        m_remoteAddr.sin6_port = htons(port);
        m_remoteAddr.sin6_family = AF_INET6;

        m_connecting = true;

        freeaddrinfo(res);
        delete m_req;
        m_req = NULL;
    } else {
        // if we end up there, it means our yield earlier threw an EAgain exception.
        AAssert(gotR(), "Please don't call connect after a EAgain without checking its signal first.");
    }

    int spins = 0;

    do {
        Printer::elog(E_SOCKET, "Connecting now...");
        int r;
        int err;
        if (spins == 0) {
            r = ::connect(getFD(), (sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr));
#ifdef _WIN32
            err = WSAGetLastError();
#else
            err = errno;
#endif
        } else {
            socklen_t sLen = sizeof(err);
            int g = getsockopt(getFD(), SOL_SOCKET, SO_ERROR, (char *) &err, &sLen);
            EAssert(g == 0, "getsockopt failed; g = %i", g);
            r = err != 0 ? -1 : 0;
        }
        if ((r == 0) || ((r < 0) && (err == EISCONN))) {
            m_connected = true;
            m_connecting = false;

            socklen_t len;

            len = sizeof(m_localAddr);
            getsockname(getFD(), (sockaddr *) &m_localAddr, &len);

            len = sizeof(m_remoteAddr);
            getpeername(getFD(), (sockaddr *) &m_remoteAddr, &len);

            char prtLocal[INET6_ADDRSTRLEN], prtRemote[INET6_ADDRSTRLEN];
            const char * rLocal, * rRemote;

            len = sizeof(m_localAddr);
            rLocal = inet_ntop(AF_INET6, &m_localAddr.sin6_addr, prtLocal, len);
            rRemote = inet_ntop(AF_INET6, &m_remoteAddr.sin6_addr, prtRemote, len);

            EAssert(rLocal, "inet_ntop returned NULL");
            EAssert(rRemote, "inet_ntop returned NULL");

            m_name.set("Socket(Connected - [%s]:%i -> [%s]:%i)", rLocal, ntohs(m_localAddr.sin6_port), rRemote, ntohs(m_remoteAddr.sin6_port));
            Printer::elog(E_SOCKET, "Connected; %s", m_name.to_charp());

            m_evtW->stop();
            return true;
        }

#ifdef _WIN32
        if (err != WSAWOULDBLOCK) {
#else
        if (err != EINPROGRESS) {
#endif
            Printer::elog(E_SOCKET, "Connect() failed with the following error code: %i (%s)", err, strerror(err));
            return false;
        } else {
            IAssert(spins == 0, "We shouldn't have spinned...");
        }

        Task::operationYield(m_evtW, Task::INTERRUPTIBLE);
        // if we're still here, it means the parent task doesn't want to be thrown an exception
        IAssert(gotW(), "We shouldn't have been awoken without getting our event signalled");

    } while (spins++ < 2);

    return false;
}

bool Balau::Socket::listen() {
    AAssert(!m_listening, "You can't call Socket::listen() on an already listening socket");
    AAssert(!m_connecting, "You can't call Socket::listen() on a connecting socket");
    AAssert(!m_connected, "You can't call Socket::listen() on a connected socket");
    AAssert(!isClosed(), "You can't call Socket::listen() on a closed socket");

    if (::listen(getFD(), 16) == 0) {
        m_listening = true;

        socklen_t len;

        len = sizeof(m_localAddr);
        getsockname(getFD(), (sockaddr *) &m_localAddr, &len);

        char prtLocal[INET6_ADDRSTRLEN];
        const char * rLocal;

        len = sizeof(m_localAddr);
        rLocal = inet_ntop(AF_INET6, &m_localAddr.sin6_addr, prtLocal, len);

        EAssert(rLocal, "inet_ntop() returned NULL");

        m_name.set("Socket(Listener - [%s]:%i)", rLocal, ntohs(m_localAddr.sin6_port));
        Printer::elog(E_SOCKET, "Socket %i started to listen: %s", getFD(), m_name.to_charp());
    } else {
        String msg = getErrorMessage();
        Printer::elog(E_SOCKET, "listen() failed with error %i (%s)", errno, msg.to_charp());
    }

    return m_listening;
}

#ifdef _WIN32
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif
#endif

Balau::IO<Balau::Socket> Balau::Socket::accept() throw (GeneralException) {
    AAssert(m_listening, "You can't call accept() on a non-listening socket");
    AAssert(!isClosed(), "You can't call accept() on a closed socket");

    while(true) {
        sockaddr_in6 remoteAddr;
        socklen_t len = sizeof(sockaddr_in6);
        Printer::elog(E_SOCKET, "Socket %i (%s) is going to accept()", getFD(), m_name.to_charp());
        int s = ::accept(getFD(), (sockaddr *) &remoteAddr, &len);

        if (s < 0) {
            if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
                Task::operationYield(m_evtR, Task::INTERRUPTIBLE);
            } else {
                String msg = getErrorMessage();
                throw GeneralException(String("Unexpected error accepting a connection: #") + errno + "(" + msg + ")");
            }
        } else {
            Printer::elog(E_SOCKET, "Listener at %p got a new connection", this);
            m_evtR->reset();
            return IO<Socket>(new Socket(s));
        }
    }
}

ssize_t Balau::Socket::read(void * buf, size_t count) throw (GeneralException) {
    AAssert(m_connected, "You can't call read() on a non-connected socket");
    return Selectable::read(buf, count);
}

ssize_t Balau::Socket::write(const void * buf, size_t count) throw (GeneralException) {
    AAssert(m_connected, "You can't call write() on a non-connected socket");
    return Selectable::write(buf, count);
}

ssize_t Balau::Socket::recv(int sockfd, void *buf, size_t len, int flags) {
    return ::recv(sockfd, buf, len, flags);
}

ssize_t Balau::Socket::send(int sockfd, const void *buf, size_t len, int flags) {
    return ::send(sockfd, buf, len, flags);
}

Balau::ListenerBase::ListenerBase(int port, const char * local, void * opaque) : m_listener(new Socket()), m_stop(false), m_local(local), m_port(port), m_opaque(opaque) {
    m_name = String("Listener for something - Starting on ") + local + ":" + port;
    Printer::elog(E_SOCKET, "Created a listener task at %p (%s)", this, m_name.to_charp());
}

const char * Balau::ListenerBase::getName() const {
    return m_name.to_charp();
}

void Balau::ListenerBase::stop() {
    Printer::elog(E_SOCKET, "Listener task at %p (%s) is asked to stop.", this, m_name.to_charp());
    m_stop = true;
    m_evt.trigger();
}

void Balau::ListenerBase::Do() {
    bool r;
    IO<Socket> io;
    while (!m_stop) {
        StacklessBegin();
        StacklessOperation(r = m_listener->setLocal(m_local.to_charp(), m_port));
        EAssert(r, "Couldn't set the local IP/port to listen to");
        r = m_listener->listen();
        EAssert(r, "Couldn't listen on the given IP/port");
        setName();
        waitFor(&m_evt);
        StacklessOperationOrCond(io = m_listener->accept(), m_stop);
        if (m_stop)
            return;
        factory(io, m_opaque);
        StacklessEnd();
    }
}
