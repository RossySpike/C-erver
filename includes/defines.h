#ifndef DEFINES_H

#define DEFINES_H
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define PORT 1600
#define BACKLOG 1
#define BUFFER 1024

#define fperror                                                                \
  fprintf(stderr, "[-] %s. errno: %d, %s\n", __func__, errno, strerror(errno))
#ifndef bzero
#define bzero(x, len_x)                                                        \
  { memset((x), 0, (len_x)); }
#endif // bzero
struct mime {
  const char *type;
  const char *content_type;
} mime_types[] = {
    {"txt", "text/plain"},
    {"html", "text/html"},
    {"css", "text/css"},
    {"js", "text/javascript"},
    {"xml", "text/xml"},
    {"json", "application/json"},
    {"pdf", "application/pdf"},
    {"bin", "application/octet-stream"},
    {"form", "application/x-www-form-urlencoded"},
    {"jpg", "image/jpg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"gif", "image/gif"},
    {"svg", "image/svg+xml"},
    {"ico", "image/x-icon"},
    {"mpeg", "audio/mpeg"},
    {"ogg", "audio/ogg"},
    {"mp4", "video/mp4"},
    {"webm", "video/webm"},
    {"form-data", "multipart/form-data"},
    {"mixed", "multipart/mixed"},
    {"alternative", "multipart/alternative"},
    {"octet-stream", "application/octet-stream"},
    {"ttf", "font/ttf"},
    {NULL, NULL} // End.
};

#endif // !#ifndef DEFINES_H
