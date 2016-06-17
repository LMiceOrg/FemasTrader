
#include "lmice_eal_inc.h"
#include "lmice_trace.h"
#include <errno.h>
#include <stdlib.h>

int eal_inc_create_client(eal_inc_param* pm)
{
    int ret = 0;
    int flag_on = 1;
    struct addrinfo hints;
    struct addrinfo *remote = pm->remote;
    struct addrinfo *local = NULL;

    /* create a socket for sending to the multicast address */
    if ((pm->sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_client call socket failed[%d]\n", ret);
        return ret;
    }

    /* set the TTL (time to live/hop count) for the send */
    if ((setsockopt(pm->sock_client, IPPROTO_IP, IP_MULTICAST_TTL,
                    (void*) &pm->ttl, sizeof(pm->ttl))) < 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_client call setsockopt(TTL) failed[%d]\n", ret);
        return ret;
    }

    /* set reuse port to on to allow multiple binds per host */
    if ((setsockopt(pm->sock_client, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on))) < 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_client call setsockopt(REUSEADDR) failed[%d]\n", ret);
        return ret;
    }

    /* getaddrinfo hints addrinfo */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family =       AF_UNSPEC;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype =     SOCK_DGRAM;     /* Stream socket */
    hints.ai_flags =        AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol =     IPPROTO_IP;     /* IP protocol */
    hints.ai_canonname =    NULL;
    hints.ai_addr =         NULL;
    hints.ai_next =         NULL;

    ret = getaddrinfo(pm->local_addr, pm->local_port, &hints, &local);
    if (ret != 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_client call getaddrinfo[%s] failed[%d (%s)]\n",
                          pm->local_addr, ret, gai_strerror(ret));
        return ret;
    }

    /* bind to multicast address to socket */
    if ((bind(pm->sock_client, (struct sockaddr *) local->ai_addr, local->ai_addrlen)) < 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_client call bind() failed[%d]\n", ret);
        return ret;
    }

    ret = getaddrinfo(pm->remote_addr, pm->remote_port, &hints, &remote);
    if (ret != 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_client call 2 getaddrinfo[%s] failed[%d (%s)]\n",
                          pm->remote_addr, ret, gai_strerror(ret));
        return ret;
    }

    return ret;
}

int eal_inc_create_server(eal_inc_param *pm) {
    int ret = 0;
    int flag_on = 1;
    struct sockaddr_in mc_addr;
    struct ip_mreq mc_req;

    /* create a socket for sending to the multicast address */
    if ((pm->sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_server call socket failed[%d]\n", ret);
        return ret;
    }

    /* set reuse port to on to allow multiple binds per host */
    if ((setsockopt(pm->sock_server, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag_on,
                    sizeof(flag_on))) < 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_server call setsockopt(REUSEADDR) failed[%d]\n", ret);
        return ret;
    }


    /* construct a multicast address structure */
    memset(&mc_addr, 0, sizeof(mc_addr));
    mc_addr.sin_family      = AF_INET;
    if(strncmp(pm->local_addr, "127.0.0.1", 10) == 0) {
        mc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        mc_addr.sin_addr.s_addr = inet_addr(pm->local_addr);
    }
    mc_addr.sin_port        = htons(atoi(pm->local_port));

    /* bind to multicast address to socket */
    if ((bind(pm->sock_server, (struct sockaddr *) &mc_addr,
              sizeof(mc_addr))) < 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_server call bind() failed[%d]\n", ret);
        return ret;
    }

    /* construct an IGMP join request structure */
    mc_req.imr_multiaddr.s_addr = inet_addr(pm->remote_addr);
    mc_req.imr_interface.s_addr = inet_addr(pm->local_addr);

    /* send an ADD MEMBERSHIP message via setsockopt */
    if ((setsockopt(pm->sock_server, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                    (void*) &mc_req, sizeof(mc_req))) < 0) {
        ret = errno;
        lmice_error_print("eal_inc_create_server call setsockopt(ADD_MEMBERSHIP) failed[%d]", ret);
    }

    return ret;
}

