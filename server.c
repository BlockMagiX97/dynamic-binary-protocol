#include "protocol.h"
#include <stdio.h>
#include <argon2.h>
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h> // mkdirat
// Server config
#define PORT 9991



void* handle_client(void* arg) {
	int client_fd = *((int*)arg);
	FILE* fptr = fopen("server.log", "wb");

	struct format_mask_t* mask = malloc_mask();
	if (mask == NULL) {
		perror("mask");
		free(arg);
		return NULL;
	}
	
	int32_t total_struct = generate_mask_from_client(client_fd, mask);

	printf("total_struct: %d\n", total_struct);
	if (total_struct < 0) {
		printf("generating mask failed: %d\n", total_struct);
		free(arg);
		free_mask(mask);
		return NULL;
	}
	for (int i=0;i<mask->num_of_structs;i++) {
		printf("%s\n", global_format.struct_info[i].identifier);
		for (int j=0;j<mask->struct_mask[i].num_of_fields;j++) {
			printf("    %s: %d\n", global_format.struct_info[i].field_info[j].identifier, mask->struct_mask[i].field_mask[j]);
		}
	}
	printf("error: %d\n",send_format_to_client(client_fd,mask,total_struct));

	struct struct1 a = {
		.x = 12,
		.nmemb_y = 9,
		.y = "Pavel th",
	};
	
	fclose(fptr);
	send_struct_server(client_fd,STRUCT_STRUCT1, &a, mask);

	struct struct1 b;
	DEBUG_PRINT;
	recv_struct_server(client_fd,STRUCT_STRUCT1,&b, mask);
	DEBUG_PRINT;
	printf("x: %d, nmemb_y: %u, y: %s\n",b.x,b.nmemb_y,b.y);
	
	
	free_mask(mask);
	free(arg);

	return NULL;
}


int main(int argc, char** argv) {
	generate_global_format();
	for (int i=0;i<global_format.num_of_structs;i++) {
		printf("struct %s essential: %u\n", global_format.struct_info[i].identifier, global_format.struct_info[i].essential);
		for (int j=0;j<global_format.struct_info[i].num_of_fields;j++) {
			printf("\t%s:essential=%u:size=%u\n",global_format.struct_info[i].field_info[j].identifier, global_format.struct_info[i].field_info[j].essential, global_format.struct_info[i].field_info[j].size);


		}
	}
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket");
		return -1;
	}

	struct struct1 a;
	a.x = 1;
	a.y = 'H';
	struct struct2 b;
	b.z = 'H';
	printf("x: %d; y: %c; z: %c\n", *((int*)get_field(STRUCT_STRUCT1, &a, 0)), *((char*)get_field(STRUCT_STRUCT1, &a, 1)), *((char*)get_field(STRUCT_STRUCT2, &b, 0)));
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	int address_len = sizeof(address);
	if (bind(sockfd, (struct sockaddr*) &address, address_len) == -1) {
		perror("bind");
		return -1;
	}

	if (listen(sockfd, 3) == -1) {
		perror("listen");
		return -1;
	}

	while(1) {
		struct sockaddr_in client_addr;

		int cilent_addr_len = sizeof(client_addr);

		int* cilent_fd = malloc(sizeof(int));

		if ((*cilent_fd = accept(sockfd, &client_addr, &cilent_addr_len)) == -1 ) {
			perror("accept");
			return -1;
		}
		
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, handle_client, (int*)cilent_fd);
		pthread_detach(thread_id);
	}
}
