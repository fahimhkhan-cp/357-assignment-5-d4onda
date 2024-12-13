#define _GNU_SOURCE
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define USAGE_STRING "usage: %s <int>\n"
#define CGI_DIR "cgi-like"

void send_response(int nfd, const char *status, const char *cont_type, const char *body) {
   char header[1024];
   int cont_len = body ? strlen(body) : 0;

   snprintf(header, sizeof(header),
   "HTTP/1.0 %s\r\n"
   "Content-Length: %d\r\n"
   "Content-Type: %s\r\n"
   "Connection: close\r\n\r\n", status, cont_len, cont_type); // printing out header

   write(nfd, header, strlen(header));
   if (body) {
      write(nfd, body, cont_len);
   }
}

void handle_request(int nfd)
{
   FILE *network = fdopen(nfd, "r");
   char method[8], path[1024], version[16];
   char *line = NULL;
   size_t size;

   if (network == NULL) {
      perror("fdopen");
      close(nfd);
      return;
   }

   if (getline(&line, &size, network) <= 0) {
      free(line);
      fclose(network);
      return;
   }

   if (sscanf(line, "%s %s %s", method, path, version) != 3) {
      send_response(nfd, "400 Bad Request", "test/plain", "Bad Request\n");
      free(line);
      fclose(network);
      return;
   }

   memmove(path, path + 1, strlen(path)); // remove / and read name of file

   struct stat file_stat;

   if (stat(path, &file_stat) == -1 || S_ISDIR(file_stat.st_mode)) {
      send_response(nfd, "404 Not Found", "test/plain", "File Not Found\n");
   } else {
      if (strcmp(method, "HEAD") == 0 || strcmp(method, "GET") == 0) {
         FILE *file = fopen(path, "r");
         if (!file) {
            send_response(nfd, "500 Internal Server Error", "text/plain", "Internal Server Error\n");
         } else {
            char *body = NULL;
            if (strcmp(method, "GET") == 0) {
               body = malloc(file_stat.st_size + 1);
               fread(body, 1, file_stat.st_size, file);
               body[file_stat.st_size] = "\0";
            }
            send_response(nfd, "200 OK", "text/html", body);
            free(body);
            fclose(file);
            }
         }
      }
   send_response(nfd, "501 Not Implemented", "text/plain", "Not Implemented");
   free(line);
   fclose(network);
}

void signal_handler(int signum) {  // kill children
   (void)signum;
   while (waitpid(-1, NULL, WNOHANG) > 0) {
      
   }
}

void run_service(int fd)
{
   while (1)
   {
      int nfd = accept_connection(fd);
      if (nfd != -1)
      {
         printf("Connection established\n");
         pid_t pid = fork();

         if (pid == 0) {
            close(fd);
            handle_request(nfd);
            close(nfd);
            printf("Connection closed\n");
            exit(0);
         } else if (pid > 0){
            close(nfd);
         } else {
            perror("fork");
            
         }
      }
   }
}

void validate_arguments(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, USAGE_STRING, "port");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[])
{
   struct sigaction sa;
   sa.sa_handler = signal_handler;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGCHLD, &sa, NULL);

   validate_arguments(argc, argv);
   int PORT = atoi(argv[1]);
   int fd = create_service(PORT);

   if (fd == -1)
   {
      perror(0);
      exit(1);
   }

   printf("listening on port: %d\n", PORT);
   run_service(fd);
   close(fd);

   return 0;
}
