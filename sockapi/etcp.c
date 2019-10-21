#include "etcp.h"

void set_address(char *hname, char *sname,
                 struct sockaddr_in *sap, char *protocol)
{
    struct servent *sp;
    struct hostent *hp;
    char *endptr;
    short port;

    bzero(sap, sizeof(*sap));
    sap->sin_family = AF_INET;
    if( hname != NULL )
    {
        if( !inet_aton(hname, &sap->sin_addr) )
        {
            hp = gethostbyname( hname );
            if(hp == NULL)
                error(1, 0, "unknown host: %s\n", hname);
            sap->sin_addr = *(struct in_addr *)hp->h_addr;
        }
    }
    else
        sap->sin_addr.s_addr = htonl( INADDR_ANY );

    port = strtol( sname, &endptr, 0 );
    if( *endptr == '\0' )
        sap->sin_port = htons( port );
    else
    {
        sp = getservbyname( sname, protocol );
        if( sp == NULL )
            error(1, 0, "unknown service: %s\n", sname);
        sap->sin_port = sp->s_port;
    }
}

int readn(SOCKET fd, char *bp, size_t len)
{
    int cnt;
    int rc;
    cnt = len;
    while( cnt > 0 )
    {
        rc = recv(fd, bp, cnt, 0);  /* Will MSG_WAITALL flag be better? */
        if( rc < 0 )                /* read error? */
        {
            if( errno == EINTR )    /* interrupted? */
                continue;           /* restart */
            return -1;              /* return error */
        }
        if( rc == 0 )               /* EOF? */
            return len - cnt;       /* return short cnt */
        bp += rc;
        cnt -= rc;
    }
    return len;
}

int readvrec(SOCKET fd, char *bp, size_t len)
{
    u_int32_t reclen;
    int rc;

    /* Retrieve the length of the record */
    rc = readn(fd, (char *)&reclen, sizeof(u_int32_t));
    if( rc != sizeof(u_int32_t) )
        return rc < 0 ? -1 : 0;
    reclen = ntohl(reclen);
    if( reclen > len )
    {
        /*
         * Not enough room for the record--
         * discard it and return an error.
         */
        while( reclen > 0 )
        {
            rc = readn(fd, bp, len);
            if( rc != len )
                return rc < 0 ? -1 : 0;
            reclen -= len;
            if( reclen < len )
                len = reclen;
        }
        set_errno( EMSGSIZE );
        return -1;
    }
    /* Retrieve the record itself */
    rc = readn(fd, bp, reclen);
    if( rc != reclen )
        return rc < 0 ? -1 : 0;
    return rc;
}

SOCKET tcp_client(char *hname, char *sname)
{
    struct sockaddr_in peer;
    SOCKET s;

    set_address(hname, sname, &peer, "tcp");
    s = socket(AF_INET, SOCK_STREAM, 0);
    if( !isvalidsock(s) )
        error(1, errno, "socket call failed");
    if( connect(s, (struct sockaddr *)&peer, sizeof(peer)) )
        error(1, errno, "connect failed");
    return s;
}

SOCKET tcp_server(char *hname, char *sname)
{
    struct sockaddr_in local;
    //struct sockaddr_in peer;
    //int peerlen;
    //SOCKET s1;
    SOCKET s;
    const int on = 1;

    set_address(hname, sname, &local, "tcp");
    s = socket(AF_INET, SOCK_STREAM, 0);
    if( !isvalidsock(s) )
        error(1, errno, "socket call failed");
    if( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) )
        error(1, errno, "setsockopt failed");
    if( bind(s, (struct sockaddr *)&local, sizeof(local)) )
        error(1, errno, "bind failed");
    if( listen(s, NLISTEN) )
        error(1, errno, "listen failed");
    return s;

    // do
    // {
    //     peerlen = sizeof(peer);
    //     s1 = accept(s, (struct sockaddr *)&peer, &peerlen);
    //     if( !isvalidsock(s1) )
    //         error(1, errno, "accept failed");
    //     server(s, &peer);
    //     CLOSE(s1);
    // }while(1);
    //
    // EXIT(0);
}

SOCKET udp_client(char *hname, char *sname,
                  struct sockaddr_in *sap)
{
    SOCKET s;

    set_address(hname, sname, sap, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if( !isvalidsock(s) )
        error(1, errno, "socket call failed");
    return s;
}

SOCKET udp_server(char *hname, char *sname)
{
    SOCKET s;
    struct sockaddr_in local;

    set_address(hname, sname, &local, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if( !isvalidsock(s) )
        error(1, errno, "socket call failed");
    if( bind(s, (struct sockaddr *)&local, sizeof(local)) )
        error(1, errno, "bind failed");
    return s;
}

int readline(SOCKET fd, char *bufptr, size_t len)
{
    char *bufx = bufptr;
    char *bp;
    int cnt = 0;
    char b[1500];
    char c;

    while( --len > 0 )  /* To ensure there always have one byte to store '\0' */
    {
        if( --cnt <= 0 )/* To ensure only call <recv> once until finish data copy */
        {
            cnt = recv(fd, b, sizeof(b), 0);
            if(cnt < 0)
            {
                if(errno == EINTR)
                {
                    len++;      /* the while will decrement */
                    continue;
                }
                return -1;
            }
            if(cnt == 0)
                return 0;
            bp = b;
        }
        c = *bp++;
        *bufptr++ = c;
        if( c == '\n' )
        {
            *bufptr = '\0';
            return bufptr - bufx;
        }
    }
    set_errno(EMSGSIZE);
    return -1;
}
