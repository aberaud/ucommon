#include <private.h>
#include <net/if.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <inc/config.h>
#include <inc/object.h>
#include <inc/timers.h>
#include <inc/linked.h>
#include <inc/string.h>
#include <inc/socket.h>

#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif

#if defined(HAVE_POLL) && defined(POLLRDNORM)
#define	USE_POLL
#endif

#ifndef	MSG_NOSIGNAL
#define	MSG_NOSIGNAL 0
#endif

typedef struct
{
	union {
		struct sockaddr addr;
		struct sockaddr_in ipv4;
		struct sockaddr_in6 ipv6;
	};
}	inetsockaddr_t;

typedef struct
{
	union {
		struct ip_mreq	ipv4;
		struct ipv6_mreq ipv6;
	};
}	inetmulticast_t;

using namespace UCOMMON_NAMESPACE;

typedef unsigned char   bit_t;

static void bitmask(bit_t *bits, bit_t *mask, unsigned len)
{
    while(len--)
        *(bits++) &= *(mask++);
}

static void bitimask(bit_t *bits, bit_t *mask, unsigned len)
{
    while(len--)
        *(bits++) |= ~(*(mask++));
}

static void bitset(bit_t *bits, unsigned blen)
{
    bit_t mask;

    while(blen) {
        mask = (bit_t)(1 << 7);
        while(mask && blen) {
            *bits |= mask;
            mask >>= 1;
            --blen;
        }
        ++bits;
    }
}

static unsigned bitcount(bit_t *bits, unsigned len)
{
    unsigned count = 0;
    bit_t mask, test;

    while(len--) {
        mask = (bit_t)(1<<7);
        test = *bits++;
        while(mask) {
            if(!(mask & test))
                return count;
            ++count;
            mask >>= 1;
		}
    }
    return count;
}


cidr::cidr() :
LinkedObject()
{
	family = AF_UNSPEC;
	memset(&network, 0, sizeof(network));
	memset(&netmask, 0, sizeof(netmask));
}

cidr::cidr(const char *cp) :
LinkedObject()
{
	set(cp);
}

cidr::cidr(LinkedObject **policy, const char *cp) :
LinkedObject(policy)
{
	set(cp);
}

cidr::cidr(const cidr &copy) :
LinkedObject()
{
	family = copy.family;
	memcpy(&network, &copy.network, sizeof(network));
	memcpy(&netmask, &copy.netmask, sizeof(netmask));
}

unsigned cidr::getMask(void) const
{
	switch(family)
	{
	case AF_INET:
		return bitcount((bit_t *)&netmask.ipv4, sizeof(struct in_addr));
	case AF_INET6:
		return bitcount((bit_t *)&netmask.ipv6, sizeof(struct in6_addr));
	default:
		return 0;
	}
}

bool cidr::allow(int so, const cidr *accept, const cidr *reject)
{
	struct sockaddr_storage addr;
	socklen_t slen = sizeof(addr);
	unsigned allowed = 0, denied = 0, masked;

	if(so == INVALID_SOCKET)
		return false;

	if(getpeername(so, (struct sockaddr *)&addr, &slen))
		return false;

	while(accept) {
		if(accept->isMember((struct sockaddr *)&addr)) {
			masked = accept->getMask();
			if(masked > allowed)
				allowed = masked;
		}
		accept = static_cast<cidr *>(accept->next);
	}

    while(reject) {
        if(reject->isMember((struct sockaddr *)&addr)) {
			masked = reject->getMask();
			if(masked > denied)
				denied = masked;
        }
        reject = static_cast<cidr *>(reject->next);
    }

	if(!allowed && !denied)
		return true;

	if(allowed > denied)
		return true;

	return false;
}	

bool cidr::find(const cidr *policy, const struct sockaddr *s)
{
	while(policy) {
		if(policy->isMember(s))
			return true;
		policy = static_cast<cidr *>(policy->next);
	}
	return false;
}

bool cidr::isMember(const struct sockaddr *s) const
{
	inethostaddr_t host;
	inetsockaddr_t *addr = (inetsockaddr_t *)s;

	if(addr->addr.sa_family != family)
		return false;

	switch(family) {
	case AF_INET:
		memcpy(&host.ipv4, &addr->ipv4.sin_addr, sizeof(host.ipv4));
		bitmask((bit_t *)&host.ipv4, (bit_t *)&netmask, sizeof(host.ipv4));
		if(!memcmp(&host.ipv4, &network.ipv4, sizeof(host.ipv4)))
			return true;
		return false;
	case AF_INET6:
		memcpy(&host.ipv6, &addr->ipv6.sin6_addr, sizeof(host.ipv6));
		bitmask((bit_t *)&host.ipv6, (bit_t *)&netmask, sizeof(host.ipv6));
		if(!memcmp(&host.ipv6, &network.ipv6, sizeof(host.ipv6)))
			return true;
		return false;
	default:
		return false;
	}
}

inethostaddr_t cidr::getBroadcast(void) const
{
	inethostaddr_t bcast;

	switch(family) {
	case AF_INET:
		memcpy(&bcast.ipv4, &network.ipv4, sizeof(network.ipv4));
		bitimask((bit_t *)&bcast.ipv4, (bit_t *)&netmask.ipv4, sizeof(bcast.ipv4));
		return bcast;
	case AF_INET6:
		memcpy(&bcast.ipv6, &network.ipv6, sizeof(network.ipv6));
		bitimask((bit_t *)&bcast.ipv6, (bit_t *)&netmask.ipv6, sizeof(bcast.ipv6));
		return bcast;
	default:
		memset(&bcast, 0, sizeof(bcast));
		return  bcast;
	}
}

unsigned cidr::getMask(const char *cp) const
{
	unsigned count = 0, rcount = 0, dcount = 0;
	const char *sp = strchr(cp, '/');
	bool flag = false;
	const char *gp = cp;
	unsigned char dots[4];
	uint32_t mask;

	switch(family) {
	case AF_INET6:
		if(sp)
			return atoi(++sp);
		if(!strncmp(cp, "ff00:", 5))
			return 8;
		if(!strncmp(cp, "ff80:", 5))
			return 10;
		if(!strncmp(cp, "2002:", 5))
			return 16;
		
		sp = strrchr(cp, ':');
		while(*(++sp) == '0')
			++sp;
		if(*sp)
			return 128;
		
		while(*cp && count < 128) {
			if(*(cp++) == ':') {
				count += 16;
				while(*cp == '0')
					++cp;
				if(*cp == ':') {
					if(!flag)
						rcount = count;
					flag = true;
				}			
				else
					flag = false;
			}
		}
		return rcount;
	case AF_INET:
		if(sp) {
			if(!strchr(++sp, '.'))
				return atoi(sp);
			mask = inet_addr(sp);
			return bitcount((bit_t *)&mask, sizeof(mask));
		}
		memset(dots, 0, sizeof(dots));
		dots[0] = atoi(cp);
		while(*gp && dcount < 3) {
			if(*(gp++) == '.')
				dots[++dcount] = atoi(gp);
		}
		if(dots[3])
			return 32;

		if(dots[2])
			return 24;

		if(dots[1])
			return 16;

		return 8;
	default:
		return 0;
	}
}

void cidr::set(const char *cp)
{
	char cbuf[128];
	char *ep;
	unsigned dots = 0;

	if(strchr(cp, ':'))
		family = AF_INET6;
	else
		family = AF_INET;

	switch(family) {
	case AF_INET:
		memset(&netmask.ipv4, 0, sizeof(netmask.ipv4));
		bitset((bit_t *)&netmask.ipv4, getMask(cp));
		cpr_strset(cbuf, sizeof(cbuf), cp);
		ep = strchr(cp, '/');
		if(ep)
			*ep = 0;

		cp = cbuf;
		while(NULL != (cp = strchr(cp, '.'))) {
			++dots;
			++cp;
		}

		while(dots++ < 3)
			cpr_stradd(cbuf, sizeof(cbuf), ".0");

#ifdef	_MSWINDOWS_
		addr = inet_addr(cbuf);
		memcpy(&network.ipv4, &addr, sizeof(network.ipv4));
#else
		inet_aton(cbuf, &network.ipv4);
#endif
		bitmask((bit_t *)&network.ipv4, (bit_t *)&netmask.ipv4, sizeof(network.ipv4));
		break;
	case AF_INET6:
		memset(&netmask.ipv6, 0, sizeof(netmask));
		bitset((bit_t *)&netmask.ipv6, getMask(cp));
		cpr_strset(cbuf, sizeof(cbuf), cp);
		ep = strchr(cp, '/');
		if(ep)
			*ep = 0;
		inet_pton(AF_INET6, cbuf, &network.ipv6);
		bitmask((bit_t *)&network.ipv6, (bit_t *)&netmask.ipv6, sizeof(network.ipv6));
	default:
		break;
	}
}

Socket::Socket(const Socket &s)
{
#ifdef	_MSWINDOWS_
	HANDLE pidH = GetCurrentProcess();
	HANDLE dupH;

	if(DuplicateHandle(pidH, reinterpret_cast<HANDLE>(s.so), pidH, dupH, 0, FALSE, DUPLICATE_SAME_ACCESS))
		so = reinterpret_cast<SOCKET>(dupH);
	else
		so = INVALID_SOCKET;
#else
	so = ::dup(s.so);
#endif
}

Socket::Socket()
{
	so = INVALID_SOCKET;
}

Socket::Socket(SOCKET s)
{
	so = s;
}

Socket::Socket(int family, int type, int protocol)
{
	so = ::socket(family, type, protocol);
}

Socket::Socket(const char *iface, const char *port, int family, int type, int protocol)
{
	so = ::socket(family, type, protocol);
	if(so != INVALID_SOCKET)
		if(::cpr_bindaddr(so, iface, port))
			release();
}

Socket::~Socket()
{
	release();
}

void Socket::release(void)
{
	if(so != INVALID_SOCKET) {
		cpr_closesocket(so);
		so = INVALID_SOCKET;
	}
}

bool Socket::operator!() const
{
	if(so == INVALID_SOCKET)
		return true;
	return false;
}

Socket &Socket::operator=(SOCKET s)
{
	release();
	so = s;
	return *this;
}	

size_t Socket::peek(void *data, size_t len) const
{
	ssize_t rtn = ::recv(so, data, 1, MSG_DONTWAIT | MSG_PEEK);
	if(rtn < 1)
		return 0;
	return (size_t)rtn;
}

ssize_t Socket::get(void *data, size_t len, sockaddr *from)
{
	socklen_t slen = 0;
	return ::recvfrom(so, data, len, 0, from, &slen);
}

ssize_t Socket::put(const void *data, size_t len, sockaddr *dest)
{
	socklen_t slen = 0;
	if(dest)
		slen = ::cpr_getaddrlen(dest);
	
	return ::sendto(so, data, len, MSG_NOSIGNAL, dest, slen);
}

ssize_t Socket::puts(const char *str)
{
	if(!str)
		return 0;

	return put(str, strlen(str));
}

ssize_t Socket::gets(char *data, size_t max, timeout_t timeout)
{
	bool crlf = false;
	bool nl = false;
	int nleft = max - 1;		// leave space for null byte
	int nstat, c;

	if(max < 1)
		return -1;

	data[0] = 0;
	while(nleft && !nl) {
		if(timeout) {
			if(!isPending(timeout))
				return -1;
		}
		nstat = ::recv(so, data, nleft, MSG_PEEK);
		if(nstat <= 0)
			return -1;
		
		for(c = 0; c < nstat; ++c) {
			if(data[c] == '\n') {
				if(c > 0 && data[c - 1] == '\r')
					crlf = true;
				++c;
				nl = true;
				break;
			}
		}
		nstat = ::recv(so, data, c, 0);
		if(nstat < 0)
			break;
			
		if(crlf) {
			--nstat;
			data[nstat - 1] = '\n';
		}	

		data += nstat;
		nleft -= nstat;
	}
	*data = 0;
	return ssize_t(max - nleft - 1);
}

int Socket::setBroadcast(bool enable)
{
	if(so == INVALID_SOCKET)
		return -1;
    int opt = (enable ? 1 : 0);
    return ::setsockopt(so, SOL_SOCKET, SO_BROADCAST,
              (char *)&opt, (socklen_t)sizeof(opt));
}

int Socket::setKeepAlive(bool enable)
{
	if(so == INVALID_SOCKET)
		return -1;
#if defined(SO_KEEPALIVE) || defined(_MSWINDOWS_)
	int opt = (enable ? ~0 : 0);
	return ::setsockopt(so, SOL_SOCKET, SO_KEEPALIVE,
             (char *)&opt, (socklen_t)sizeof(opt));
#else
	return -1;
#endif
}				

int Socket::setMulticast(bool enable)
{
	inetsockaddr_t addr;
	socklen_t len = sizeof(addr);

	if(so == INVALID_SOCKET)
		return -1;

	::getsockname(so, (struct sockaddr *)&addr, &len);
	if(!enable)
		switch(addr.addr.sa_family)
		{
		case AF_INET:
			memset(&addr.ipv4.sin_addr, 0, sizeof(addr.ipv4.sin_addr));
			break;
		case AF_INET6:
			memset(&addr.ipv6.sin6_addr, 0, sizeof(addr.ipv6.sin6_addr));
		default:
			break;
		}
	switch(addr.addr.sa_family) {
	case AF_INET6:
		return ::setsockopt(so, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&addr.ipv6.sin6_addr, sizeof(addr.ipv6.sin6_addr));
	case AF_INET:
#ifdef	IP_MULTICAST_IF
		return ::setsockopt(so, IPPROTO_IP, IP_MULTICAST_IF, (char *)&addr.ipv4.sin_addr, sizeof(addr.ipv4.sin_addr));
#else
		return -1;
#endif
	default:
		return -1;
	}	
}

int Socket::setNonBlocking(bool enable)
{
	if(so == INVALID_SOCKET)
		return -1;

#if defined(_MSWINDOWS_)
	unsigned long flag = (enable ? 1 : 0);
	return ioctlsocket(so, FIONBIO, &flag);
#else
	long flags = fcntl(so, F_GETFL);
	if(enable)
		flags |= O_NONBLOCK;
	else
		flags &=~ O_NONBLOCK;
	return fcntl(so, F_SETFL, flags);
#endif
}

int Socket::connect(const char *host, const char *svc)
{
	return ::cpr_connect(so, host, svc);
}

int Socket::join(const char *member)
{
	return ::cpr_joinaddr(so, member);
}

int Socket::drop(const char *member)
{
	return ::cpr_dropaddr(so, member);
}

int Socket::disconnect(void)
{
	return ::cpr_disconnect(so);
}

bool Socket::isConnected(void) const
{
	char buf;

	if(so == INVALID_SOCKET)
		return false;

	if(!isPending())
		return true;

	if(::recv(so, &buf, 1, MSG_DONTWAIT | MSG_PEEK) < 1)
		return false;

	return true;
}

#ifdef _MSWINDOWS_
unsigned Socket::getPending(void) const
{
	long opt;
	if(so == INVALID_SOCKET)
		return 0;

	ioctlsocket(so, FIONREAD, &opt);
	return (unsigned)opt;
}
#else
unsigned Socket::getPending(void) const
{
	int opt;
	if(so == INVALID_SOCKET)
		return 0;

	if(::ioctl(so, FIONREAD, &opt))
		return 0;
	return (unsigned)opt;
}
#endif

bool Socket::isPending(timeout_t timeout) const
{
	int status;
#ifdef	USE_POLL
	struct pollfd pfd;

	pfd.fd = so;
	pfd.revents = 0;
	pfd.events = POLLIN;

	if(so == INVALID_SOCKET)
		return false;

	status = 0;
	while(status < 1) {
		if(timeout == Timer::inf)
			status = ::poll(&pfd, 1, -1);
		else
			status = ::poll(&pfd, 1, timeout);
		if(status == -1 && errno == EINTR)
			continue;
		if(status < 0)
			return false;
	}
	if(pfd.revents & POLLIN)
		return true;
	return false;
#else
	struct timeval tv;
	struct timeval *tvp = &tv;
	fd_set grp;

	if(so == INVALID_SOCKET)
		return false;

	if(timeout == Timer::inf)
		tvp = NULL;
	else {
        tv.tv_usec = (timeout % 1000) * 1000;
        tv.tv_sec = timeout / 1000;
	}

	FD_ZERO(&grp);
	FD_SET(so, &grp);
	status = ::select((int)(so + 1), &grp, NULL, NULL, tvp);
	if(status < 1)
		return false;
	if(FD_ISSET(so, &grp))
		return true;
	return false;
#endif
}

bool Socket::isSending(timeout_t timeout) const
{
	int status;
#ifdef	USE_POLL
	struct pollfd pfd;

	pfd.fd = so;
	pfd.revents = 0;
	pfd.events = POLLOUT;

	if(so == INVALID_SOCKET)
		return false;

	status = 0;
	while(status < 1) {
		if(timeout == Timer::inf)
			status = ::poll(&pfd, 1, -1);
		else
			status = ::poll(&pfd, 1, timeout);
		if(status == -1 && errno == EINTR)
			continue;
		if(status < 0)
			return false;
	}
	if(pfd.revents & POLLOUT)
		return true;
	return false;
#else
	struct timeval tv;
	struct timeval *tvp = &tv;
	fd_set grp;

	if(so == INVALID_SOCKET)
		return false;

	if(timeout == Timer::inf)
		tvp = NULL;
	else {
        tv.tv_usec = (timeout % 1000) * 1000;
        tv.tv_sec = timeout / 1000;
	}

	FD_ZERO(&grp);
	FD_SET(so, &grp);
	status = ::select((int)(so + 1), NULL, &grp, NULL, tvp);
	if(status < 1)
		return false;
	if(FD_ISSET(so, &grp))
		return true;
	return false;
#endif
}

ListenSocket::ListenSocket(const char *iface, const char *svc, unsigned backlog) :
Socket()
{
	int family = AF_INET;
	if(strchr(iface, '/'))
		family = AF_UNIX;
	else if(strchr(iface, ':'))
		family = AF_INET6;

retry:
	so = ::socket(family, SOCK_STREAM, 0);
	if(so == INVALID_SOCKET)
		return;
		
	if(::cpr_bindaddr(so, iface, svc)) {
		release();
		if(family == AF_INET && !strchr(iface, '.')) {
			family = AF_INET6;
			goto retry;
		}
		return;
	}
	if(::listen(so, backlog))
		release();
}

SOCKET ListenSocket::accept(struct sockaddr *addr)
{
	socklen_t len = 0;
	if(addr) {
		len = ::cpr_getaddrlen(addr);		
		return ::accept(so, addr, &len);
	}
	else
		return ::accept(so, NULL, NULL);
}

#ifdef	AF_UNIX

static socklen_t unixaddr(struct sockaddr_un *addr, const char *path)
{
	socklen_t len;
	unsigned slen = strlen(path);

    if(slen > sizeof(struct sockaddr_storage) - 8)
        slen = sizeof(struct sockaddr_storage) - 8;

    memset(addr, 0, sizeof(struct sockaddr_storage));
    addr->sun_family = AF_UNIX;
    memcpy(addr->sun_path, path, slen);

#ifdef	__SUN_LEN
	len = sizeof(addr->sun_len) + strlen(addr->sun_path) + 
		sizeof(addr->sun_family) + 1;
	addr->sun_len = len;
#else
    len = strlen(addr->sun_path) + sizeof(addr->sun_family) + 1;
#endif
	return len;
}

#endif

extern "C" struct addrinfo *cpr_getsockhint(SOCKET so, struct addrinfo *hint)
{
	struct sockaddr_storage st;
	struct sockaddr *sa = (struct sockaddr *)&st;
	socklen_t slen;

	memset(hint, 0 , sizeof(struct addrinfo));
	memset(&sa, 0, sizeof(sa));
	if(getsockname(so, sa, &slen))
		return NULL;
	hint->ai_family = sa->sa_family;
	slen = sizeof(hint->ai_socktype);
	getsockopt(so, SOL_SOCKET, SO_TYPE, &hint->ai_socktype, &slen);
	return hint;
}

extern "C" char *cpr_hosttostr(struct sockaddr *sa, char *buf, size_t max)
{
	socklen_t sl;

#ifdef	AF_UNIX
    struct sockaddr_un *un = (struct sockaddr_un *)sa;
#endif

	switch(sa->sa_family) {
#ifdef	AF_UNIX
	case AF_UNIX:
		if(max > sizeof(un->sun_path))
			max = sizeof(un->sun_path);
		else
			--max;
		strncpy(buf, un->sun_path, max);
		buf[max] = 0;
		return buf;		
#endif
	case AF_INET:
		sl = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		sl = sizeof(struct sockaddr_in6);
		break;
	default:
		return NULL;
	}

	if(getnameinfo(sa, sl, buf, max, NULL, 0, NI_NOFQDN))
		return NULL;

	return buf;
}

extern "C" socklen_t cpr_getsockaddr(SOCKET so, struct sockaddr_storage *sa, const char *host, const char *svc)
{
	socklen_t len = 0;
	struct addrinfo hint, *res = NULL;

#ifdef	AF_UNIX
	if(strchr(host, '/'))
		return unixaddr((struct sockaddr_un *)sa, host);
#endif

	if(!cpr_getsockhint(so, &hint) || !svc)
		return 0;

	if(getaddrinfo(host, svc, &hint, &res) || !res)
		goto exit;

	memcpy(sa, res->ai_addr, res->ai_addrlen);
	len = res->ai_addrlen;

exit:
	if(res)
		freeaddrinfo(res);
	return len;
}

extern "C" int cpr_bindaddr(SOCKET so, const char *host, const char *svc)
{
	int rtn = -1;
	int reuse = 1;

	struct addrinfo hint, *res = NULL;

    setsockopt(so, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

#ifdef AF_UNIX
	if(strchr(host, '/')) {
		struct sockaddr_un uaddr;
		socklen_t len = unixaddr(&uaddr, host);
		rtn = bind(so, (struct sockaddr *)&uaddr, len);
		goto exit;	
	};
#endif

    if(!cpr_getsockhint(so, &hint) || !svc)
        return -1;

	if(host && !cpr_stricmp(host, "*"))
		host = NULL;

#ifdef	SO_BINDTODEVICE
	if(host && !strchr(host, '.') && !strchr(host, ':')) {
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_ifrn.ifrn_name, host, IFNAMSIZ);
		setsockopt(so, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
		host = NULL;		
	}
#endif	

	hint.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

	rtn = getaddrinfo(host, svc, &hint, &res);
	if(rtn)
		goto exit;

	rtn = bind(so, res->ai_addr, res->ai_addrlen);
exit:
	if(res)
		freeaddrinfo(res);
	return rtn;
}

extern "C" socklen_t cpr_getaddrlen(struct sockaddr *sa)
{
	switch(sa->sa_family)
	{
	case AF_INET:
		return sizeof(sockaddr_in);
	case AF_INET6:
		return sizeof(sockaddr_in6);
	default:
		return sizeof(sockaddr_storage);
	}
}

extern "C" int cpr_joinaddr(SOCKET so, const char *host)
{
	inetmulticast_t mcast;
	inetsockaddr_t addr;
	socklen_t len = sizeof(addr);
	inetsockaddr_t *target;
	struct addrinfo hint, *res, *node;
	int rtn;

	if(so == INVALID_SOCKET)
		return -1;
	
	getsockname(so, (struct sockaddr *)&addr, &len);
	if(cpr_getsockhint(so, &hint) == NULL)
		return -1;
	rtn = getaddrinfo(host, NULL, &hint, &res);
	if(rtn)
		goto exit;

	node = res;
	while(!rtn && node && node->ai_addr) {
		target = (inetsockaddr_t *)node->ai_addr;
		node = node->ai_next;
		switch(addr.addr.sa_family) {
		case AF_INET6:
			memcpy(&mcast.ipv6.ipv6mr_interface, &addr.ipv6.sin6_addr, sizeof(addr.ipv6.sin6_addr));
			memcpy(&mcast.ipv6.ipv6mr_multiaddr, &target->ipv6.sin6_addr, sizeof(target->ipv6.sin6_addr));
			rtn = ::setsockopt(so, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&mcast, sizeof(mcast.ipv6));
		case AF_INET:
			memcpy(&mcast.ipv4.imr_interface, &addr.ipv4.sin_addr, sizeof(addr.ipv4.sin_addr));
			memcpy(&mcast.ipv4.imr_multiaddr, &target->ipv4.sin_addr, sizeof(target->ipv4.sin_addr));
			rtn = ::setsockopt(so, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mcast, sizeof(mcast.ipv6));
		default:
			rtn = -1;
		}
	}
exit:
	if(res)
		freeaddrinfo(res);
	return rtn;
}

extern "C" int cpr_dropaddr(SOCKET so, const char *host)
{
	inetmulticast_t mcast;
	inetsockaddr_t addr;
	socklen_t len = sizeof(addr);
	inetsockaddr_t *target;
	struct addrinfo hint, *res, *node;
	int rtn;

	if(so == INVALID_SOCKET)
		return -1;
	
	getsockname(so, (struct sockaddr *)&addr, &len);
	if(cpr_getsockhint(so, &hint) == NULL)
		return -1;
	rtn = getaddrinfo(host, NULL, &hint, &res);
	if(rtn)
		goto exit;

	node = res;
	while(!rtn && node && node->ai_addr) {
		target = (inetsockaddr_t *)node->ai_addr;
		node = node->ai_next;
		switch(addr.addr.sa_family) {
		case AF_INET6:
			memcpy(&mcast.ipv6.ipv6mr_interface, &addr.ipv6.sin6_addr, sizeof(addr.ipv6.sin6_addr));
			memcpy(&mcast.ipv6.ipv6mr_multiaddr, &target->ipv6.sin6_addr, sizeof(target->ipv6.sin6_addr));
			rtn = ::setsockopt(so, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char *)&mcast, sizeof(mcast.ipv6));
		case AF_INET:
			memcpy(&mcast.ipv4.imr_interface, &addr.ipv4.sin_addr, sizeof(addr.ipv4.sin_addr));
			memcpy(&mcast.ipv4.imr_multiaddr, &target->ipv4.sin_addr, sizeof(target->ipv4.sin_addr));
			rtn = ::setsockopt(so, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mcast, sizeof(mcast.ipv6));
		default:
			rtn = -1;
		}
	}
exit:
	if(res)
		freeaddrinfo(res);
	return rtn;
}

extern "C" int cpr_disconnect(SOCKET so)
{
	struct sockaddr_storage saddr;
	struct sockaddr *addr = (struct sockaddr *)&saddr;
	int family;
	socklen_t len = sizeof(addr);

	getsockname(so, (struct sockaddr *)&addr, &len);
	family = addr->sa_family;
	memset(addr, 0, sizeof(saddr));
#if defined(_MSWINDOWS_)
	addr->sa_family = family;
#else
	addr->sa_family = AF_UNSPEC;
#endif
	if(len > sizeof(saddr))
		len = sizeof(saddr);
	return connect(so, addr, len);
}

extern "C" void cpr_closesocket(SOCKET so)
{
#ifdef	_MSWINDOWS_
	if(so != SOCKET_INVALID)
		closesocket(so);
#else
	if(so > -1)
		close(so);
#endif
}	

extern "C" int cpr_connect(SOCKET so, const char *host, const char *svc)
{
	int rtn = -1;
	struct addrinfo hint, *res = NULL, *node;

#ifdef	AF_UNIX
	if(strchr(host, '/')) {
		struct sockaddr_un uaddr;
		socklen_t len = unixaddr(&uaddr, host);
		return connect(so, (struct sockaddr *)&uaddr, len);
	}
#endif

	if(cpr_getsockhint(so, &hint))
		return -1;

	rtn = getaddrinfo(host, svc, &hint, &res);
	if(rtn)
		goto exit;

	rtn = -1;
	if(!res)
		goto exit;

	node = res;
	while(node) {
		if(!connect(so, node->ai_addr, node->ai_addrlen)) {
			rtn = 0;
			goto exit;
		}
		node = node->ai_next;
	}

exit:
	if(res)
		freeaddrinfo(res);
	return rtn;
}	
	
	
