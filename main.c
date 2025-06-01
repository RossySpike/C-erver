#include "includes/defines.h" // for fperror, BUFFER, PORT, BACKLOG, bzero macros.
#include "includes/files.h" // for file structure and open_file, close_file functions.
#include <fcntl.h>          // for open() function
#include <netinet/in.h> // for sockaddr_in structure.
#include <signal.h>     // for signal handling.
#include <stdio.h>
#include <stdlib.h> // for exit() function.
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h> // for socket() function.
#include <sys/stat.h>   // for fstat() function
#include <sys/types.h>  // for types.
#include <unistd.h>     // for close() function

enum methods { GET, POST, PUT, DELETE };

typedef struct SERVER_ADDR {
  struct sockaddr_in addr;
  socklen_t len;
} server_addr;

typedef struct SERVER {
  int server_fd;
  int client_fd;
  server_addr addr;
} server;

int init_server_addr(server_addr *saddr) {
  saddr->len = sizeof(saddr->addr);
  bzero(saddr, saddr->len);
  saddr->addr.sin_family = AF_INET;
  saddr->addr.sin_port = htons(PORT);
  saddr->addr.sin_addr.s_addr = htonl(INADDR_ANY);
  return 0;
}

int optval = 1;
int init_sock_server(server *s) {
  s->server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (s->server_fd < 0) {
    fperror;
    return -1;
  }
  if (setsockopt(s->server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval,
                 sizeof(optval))) {
    close(s->server_fd);
    fperror;
    return -1;
  }
  if (bind(s->server_fd, (struct sockaddr *)&s->addr.addr, s->addr.len) < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }
  if (listen(s->server_fd, BACKLOG) < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }

  return 0;
}
int init_client(server *s) {
  printf("Waiting for client connection on PORT:%d...\n", PORT);
  s->client_fd =
      accept(s->server_fd, (struct sockaddr *)&s->addr.addr, &s->addr.len);
  if (s->client_fd < 0) {
    fperror;
    close(s->server_fd);
    return -1;
  }
  printf("Client connected successfully.\n");
  return 0;
}
int init_server(server *s) {
  printf("Initialazing server state...\n");
  if (init_server_addr(&s->addr) < 0) {
    return -1;
  }
  if (init_sock_server(s) < 0) {
    return -1;
  }
  printf("Server initialized successfully.\n");
  return 0;
}
int destroy_server(server *s) {
  if (s->client_fd >= 0) {
    close(s->client_fd);
    s->client_fd = -1;
  }
  if (s->server_fd >= 0) {
    close(s->server_fd);
    s->client_fd = -1;
  }
  printf("Server destroyed successfully.\n");
  return 0;
}
server s;
void handler(int sig) {
  write(STDOUT_FILENO, "\nExiting...\n", 12);
  destroy_server(&s);
  exit(EXIT_SUCCESS);
}

int get_method(char *r, int *start_url) {
  char *first_space = strchr(r, ' ');
  if (first_space == NULL) {
    return -1; // No space found, invalid request
  }
  first_space++;
  int len = first_space - r;
  *start_url = len; // Set start_url to the position after the method
  char method[len];
  method[len - 1] = '\0';
  for (int i = 0; i < len - 1; i++) {
    method[i] = r[i];
  }
  if (strcmp(method, "GET") == 0) {
    return 0; // GET method
  } else if (strcmp(method, "POST") == 0) {
    return 1; // POST method
  } else if (strcmp(method, "PUT") == 0) {
    return 2; // PUT method
  } else if (strcmp(method, "DELETE") == 0) {
    return 3; // DELETE method
  } else {
    return -1; // Unknown method
  }
}

int get_content_type(char *s, char *url, int url_len) {
  char *cur = url;

  int i;
  for (i = url_len - 1; i >= 0; i--) {
    if (cur[i] == '.') {
      cur += i + 1;
      break;
    }
  }
  if (i < 0) {
    fprintf(stderr, "[-] Invalid URL format.\n");
    return -1;
  }
  for (int i = 0; mime_types[i].type != NULL; i++) {
    /* printf("Checking type: %s\n", mime_types[i].type); */
    /* printf("URL: %s\n", url); */
    /* printf("URL length: %d\n", url_len); */
    /* printf("Comparing: %s\n", cur); */
    if (strcmp(cur, mime_types[i].type) == 0) {
      snprintf(s, BUFFER, "%s", mime_types[i].content_type);
      return 0; // Found a matching content type
    }
  }

  return -1; // Not found

  /* char *cur = url; */
  /**/
  /* int i; */
  /* int type_len = 0; */
  /* for (i = url_len - 1; i >= 0; i--) { */
  /*   if (cur[i] == '.') { */
  /*     cur += i + 1; */
  /*     break; */
  /*   } */
  /*   type_len++; */
  /* } */
  /* if (i < 0 || type_len > 255) { */
  /*   fprintf(stderr, "[-] Invalid URL format.\n"); */
  /*   return -1; */
  /* } */
  /* char type[256]; */
  /* bzero(type, sizeof(type)); */
  /* snprintf(type, sizeof(type), "%s", cur); */
  /* if (strncmp(type, "plain", 255) == 0) */
  /*   snprintf(s, BUFFER, "text/plain"); */
  /* else if (strncmp(type, "html", 255) == 0) */
  /*   snprintf(s, BUFFER, "text/html"); */
  /* else if (strncmp(type, "css", 255) == 0) */
  /*   snprintf(s, BUFFER, "text/css"); */
  /* else if (strncmp(type, "js", 255) == 0) */
  /*   snprintf(s, BUFFER, "text/javascript"); */
  /* else if (strncmp(type, "xml", 255) == 0) */
  /*   snprintf(s, BUFFER, "text/xml"); */
  /* else if (strncmp(type, "json", 255) == 0) */
  /*   snprintf(s, BUFFER, "application/json"); */
  /* else if (strncmp(type, "pdf", 255) == 0) */
  /*   snprintf(s, BUFFER, "application/pdf"); */
  /* else if (strncmp(type, "bin", 255) == 0) */
  /*   snprintf(s, BUFFER, "application/octet-stream"); */
  /* else if (strncmp(type, "form", 255) == 0) */
  /*   snprintf(s, BUFFER, "application/x-www-form-urlencoded"); */
  /* else if (strncmp(type, "jpeg", 255) == 0 || strncmp(type, "jpg", 255) == 0)
   */
  /*   snprintf(s, BUFFER, "image/jpeg"); */
  /* else if (strncmp(type, "png", 255) == 0) */
  /*   snprintf(s, BUFFER, "image/png"); */
  /* else if (strncmp(type, "gif", 255) == 0) */
  /*   snprintf(s, BUFFER, "image/gif"); */
  /* else if (strncmp(type, "svg", 255) == 0) */
  /*   snprintf(s, BUFFER, "image/svg+xml"); */
  /* else if (strncmp(type, "ico", 255) == 0) */
  /*   snprintf(s, BUFFER, "image/x-icon"); */
  /* else if (strncmp(type, "mpeg", 255) == 0) */
  /*   snprintf(s, BUFFER, "audio/mpeg"); */
  /* else if (strncmp(type, "ogg", 255) == 0) */
  /*   snprintf(s, BUFFER, "audio/ogg"); */
  /* else if (strncmp(type, "mp4", 255) == 0) */
  /*   snprintf(s, BUFFER, "video/mp4"); */
  /* else if (strncmp(type, "webm", 255) == 0) */
  /*   snprintf(s, BUFFER, "video/webm"); */
  /* else if (strncmp(type, "form-data", 255) == 0) */
  /*   snprintf(s, BUFFER, "multipart/form-data"); */
  /* else if (strncmp(type, "mixed", 255) == 0) */
  /*   snprintf(s, BUFFER, "multipart/mixed"); */
  /* else if (strncmp(type, "alternative", 255) == 0) */
  /*   snprintf(s, BUFFER, "multipart/alternative"); */
  /* else if (strncmp(type, "octet-stream", 255) == 0) */
  /*   snprintf(s, BUFFER, "application/octet-stream"); // Default type */
  /* else if (strncmp(type, "ttf", 255) == 0) */
  /*   snprintf(s, BUFFER, "font/ttf"); */
  /* else */
  /*   return -1; // Unknown type */

  /* return 0; // Success */
}

int get(char *r, int url_start) {
  char *url_end = strchr(r + url_start, ' ');
  char content_type[BUFFER + 1];
  char send_buffer[BUFFER + 1];
  if (url_end == NULL) {
    snprintf(send_buffer, BUFFER, "HTTP/1.1 400 Bad Request\r\n\r\n");
    write(s.client_fd, send_buffer, strlen(send_buffer));
    fprintf(stderr, "[-] Invalid request format.\n");
    return -1;
  }
  int url_length = url_end - (r + url_start);
  file file;
  if (url_length == 1 && *(r + url_start) == '/') {

    if (open_file(&file, "index.html", O_RDONLY) < 0) {
      snprintf(send_buffer, BUFFER, "HTTP/1.1 404 Not Found\r\n\r\n");
      write(s.client_fd, send_buffer, strlen(send_buffer));
      fprintf(stderr, "[-] Failed to open file: %s\n", "index.html");
      return -1; // No need to close s->client_fd here, as it will be closed
                 // later in main loop
    }
    snprintf(send_buffer, BUFFER,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             file.size);
    write(s.client_fd, send_buffer, strlen(send_buffer));
    sendfile(s.client_fd, file.fd, NULL, file.size);

    close_file(&file);
    return 0;
  }
  url_start++; // Move past the space after the method

  char url[url_length];
  strncpy(url, r + url_start, url_length);
  url[url_length - 1] = '\0'; // Null-terminate the URL string
  printf("GET request for URL: %s.\n", url);
  if (open_file(&file, url, O_RDONLY) < 0) {
    snprintf(send_buffer, BUFFER, "HTTP/1.1 404 Not Found\r\n\r\n");
    write(s.client_fd, send_buffer, strlen(send_buffer));
    fprintf(stderr, "[-] Failed to open file: %s\n", url);
    return -1;
  }
  if (get_content_type(content_type, url, url_length) == -1) {
    fprintf(stderr, "[-] Unknown content type for URL: %s\n", url);
    snprintf(send_buffer, BUFFER,
             "HTTP/1.1 405 Unsupported Media Type\r\n\r\n");
    write(s.client_fd, send_buffer, strlen(send_buffer));

  } else {

    snprintf(send_buffer, BUFFER,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             content_type, file.size);
    write(s.client_fd, send_buffer, strlen(send_buffer));
    sendfile(s.client_fd, file.fd, NULL, file.size);
  }
  close_file(&file);
  return 0;
}

int handle_request(char *r) {
  int url_start;
  switch (get_method(r, &url_start)) {
  case GET:
    get(r, url_start);
    break;
  default:
    fprintf(stderr, "[-] Unknown method.\n");
    return -1;
  }
  return 0;
}

int main() {
  char recv_buffer[BUFFER + 1];
  char send_buffer[BUFFER + 1];
  int n;

  bzero(recv_buffer, BUFFER);
  bzero(send_buffer, BUFFER);
  if (init_server(&s) < 0)
    exit(EXIT_FAILURE);
  signal(SIGINT, handler);
  for (;;) {

    if (init_client(&s) < 0) {
      return -1;
    }
    int first = 1;
    while ((n = read(s.client_fd, recv_buffer, BUFFER)) > 0) {
      fprintf(stdout, "------------\n\n%s", recv_buffer);
      if (first) {
        handle_request(recv_buffer);
      }
      first = 0;
      if (recv_buffer[n - 1] == '\n') {
        break;
      }
      bzero(recv_buffer, BUFFER);
    }
    if (n < 0) {
      fperror;
      close(s.client_fd);
    }
    /* snprintf((char *)send_buffer, BUFFER, */
    /*          "HTTP/1.1 200 OK\r\n\r\nHello\r\n\r\n"); */
    /* write(s.client_fd, send_buffer, strlen(send_buffer)); */

    /* file file; */
    /* if (open_file(&file, "index.html", O_RDONLY) < 0) { */
    /*   destroy_server(&s); */
    /*   exit(EXIT_FAILURE); */
    /* } */
    /* snprintf((char *)send_buffer, BUFFER, */
    /*          "HTTP/1.1 200 OK\r\n" */
    /*          "Content-Type: text/html\r\n" */
    /*          "Content-Length: %ld\r\n" */
    /*          "\r\n", */
    /*          file.size); */
    /* write(s.client_fd, send_buffer, strlen(send_buffer)); */
    /**/
    /* sendfile(s.client_fd, file.fd, NULL, file.size); */
    /**/
    /* close_file(&file); */
    close(s.client_fd);
  }
}
