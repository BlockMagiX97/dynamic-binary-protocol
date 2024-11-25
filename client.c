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
	struct redir_table* redir = NULL;
	generate_format_from_server(sockfd, mask, &redir);
	for(int i=0;i<global_format.num_of_structs;i++) {
		printf("%d\n", redir->struct_remap[i]);
		for (int j=0;j<global_format.struct_info[i].num_of_fields;j++) {
			printf("\t%d\n", redir->field_remap[i][j]);
		}
	}
	


	close(sockfd);
	
	return 0;

	
}
