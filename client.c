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
	connect(sockfd, &serveraddr, sizeof(serveraddr));


	generate_global_format();
	if (send_format_to_server(sockfd) < 0) {
		printf("FUCKKKKK\n");
	}
	
	struct format_mask_t* mask = malloc_mask();
	struct redir_table_t* redir = malloc_redir_table();
	printf("generate_format_from_server: %d\n",generate_format_from_server(sockfd, redir));
	DEBUG_PRINT;
	for(int i=0;i<global_format.num_of_structs;i++) {
		printf("%u\n", redir->struct_remap[i]);
		if (redir->struct_remap[i] == UINT32_MAX) {
			// skip because redir->field_remap[i] == NULL
			continue;
		}
		for (int j=0;j<global_format.struct_info[i].num_of_fields;j++) {
			printf("\t%u\n", redir->field_remap[i][j]);
		}
	}
	struct struct1 a = {
		.x = 2,
		.nmemb_y = 7,
		.y = "e Crow"
	};
	DEBUG_PRINT;

	FILE* fptr = fopen("client.log", "wb");
	send_struct_client(fileno(fptr),STRUCT_STRUCT1, &a, redir);
	fclose(fptr);
	send_struct_client(sockfd, STRUCT_STRUCT1, &a, redir);
	struct struct1 b;
	DEBUG_PRINT;
	recv_struct_client(sockfd,STRUCT_STRUCT1, &b, redir);
	DEBUG_PRINT;
	printf("x: %d, nmemb_y: %u, y: %s\n",b.x,b.nmemb_y, b.y);
	close(sockfd);
	return 0;
}
