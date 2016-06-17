/** Unix Domain Socket Server
 *
 */



#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>


#include "lmice_trace.h"

#include "udss.h"



static int make_socket_non_blocking (int sfd);



void create_uds_msg(void** ppmsg) {
    *ppmsg = (struct uds_msg*)malloc(sizeof(struct uds_msg));
    memset(*ppmsg, 0, sizeof(struct uds_msg));
}

void delete_uds_msg(void** ppmsg) {
    free(*ppmsg);
    *ppmsg = 0;
}

int init_uds_server(const char* srv_name, struct uds_msg* msg) {
    struct sockaddr_un  srv_un = {0};
    int sock;

    memset(msg, 0, sizeof(struct uds_msg));

    if((msg->sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        lmice_error_log("UDS socket create failed.");
        return -1;
    }

    msg->local_un.sun_family = AF_UNIX;
    strncpy(msg->local_un.sun_path, srv_name, strlen(srv_name));
    unlink(srv_name);

    if(bind(msg->sock, (struct sockaddr*)&(msg->local_un), sizeof(msg->local_un)) == -1) {
        lmice_error_log("UDS socket bind failed.");
        return -1;
    }

    chmod(srv_name, 0666);

    if( make_socket_non_blocking(msg->sock) == -1) {
        lmice_error_log("UDS socket nonblock failed.");
        return -1;
    }

    return sock;
}



/**
 * @brief init_uds_client
 * @param srv_name
 * @param msg : remote_sock, remote_socketaddr, local_sock, local_socketaddr, data
 * @param sz
 * @return
 */
int init_uds_client(const char* srv_name, struct uds_msg* msg) {
    int fd;
    char cli_name[64] = {0};
    sprintf(cli_name, "/run/user/1000/lmiced_cli_XXXXXX");
    fd = mkstemp(cli_name);

    memset(msg, 0, sizeof(struct uds_msg));

    msg->remote_un.sun_family = AF_UNIX;
    strncpy(msg->remote_un.sun_path, srv_name, strlen(srv_name));


    if((msg->sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        lmice_error_log("UDS client socket create failed.");
        return -1;
    }

    msg->local_un.sun_family = AF_UNIX;
    strncpy(msg->local_un.sun_path, cli_name, strlen(cli_name));
    close(fd);
    unlink(cli_name);

    if(bind(msg->sock, (struct sockaddr*)&(msg->local_un), sizeof(msg->local_un)) == -1) {
        lmice_error_log("UDS client socket bind failed.");
        return -1;
    }

    /*
    if( make_socket_non_blocking(msg->sock) == -1) {
        lmice_error_log("UDS client socket nonblock failed.");
        return -1;
    }
    */

}

int finit_uds_msg(struct uds_msg* msg) {
    close(msg->sock);
    unlink(msg->local_un.sun_path);
    return 0;
}


static int make_socket_non_blocking (int sfd) {
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1) {
        return -1;
    }

    return 0;
}


int send_uds_msg(uds_msg *msg) {
    int ret;
    ret = sendto(msg->sock, msg->data, msg->size, 0, (struct sockaddr*)&(msg->remote_un), sizeof msg->remote_un);
    if(ret == msg->size) {
        ret = 0;
    }
    return ret;
}
