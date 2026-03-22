#ifndef HTTP_POST_H
#define HTTP_POST_H

#include <stdbool.h>

bool http_post_text(const char *server_ip, int port,
                    const char *path, const char *body);

#endif