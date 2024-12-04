#include <unistd.h>
#include <argon2.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "protocol.h"
#define PORT 9991

int main(int argc, char**argv) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(PORT);
	connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));


	if (generate_global_format() != 0) {
		 printf("Error generating global format\n");
		return 1;
	}
	send_format_to_server(sockfd);
	
	struct format_mask_t* mask = malloc_mask();
	struct redir_table_t* redir = malloc_redir_table();
	generate_format_from_server(sockfd, redir);

	const struct struct1 a = {
		.x = 2,
		.nmemb_y = 17,
		.y = "Pavel the Yapper"
	};

	send_struct_client(sockfd, STRUCT_STRUCT1, &a, redir);
	struct struct1 b;
	recv_struct_client(sockfd,STRUCT_STRUCT1, &b, redir);
	printf("x: %d, nmemb_y: %u, y: %s\n",b.x,b.nmemb_y, b.y);
	close(sockfd);
	return 0;
}
