#ifndef HTTP_CLIENT_COMPENT
#define HTTP_CLIENT_COMPENT
#include "application.h"
extern void http_clent_start();
extern void http_client_post(const char *url, const char *data);
extern void http_client_get(const char *url);

#endif