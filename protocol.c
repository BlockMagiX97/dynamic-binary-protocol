#include "protocol.h"

struct format_t global_format;
// wrappers for implementing TLS
int send_data(int fd, void* data, size_t size) {
	return write(fd, data, size);
}
// test comment
int recv_data(int fd, void* data, size_t size) {
	size_t size_old = size;
	int out;
	while (size != 0) {
		out=read(fd, data, size);
		if(out < 0) {
			perror("recv_data");
			return size_old - size;
		}
		size-=out;
		data+=out;
	}
	return size_old;
}

int generate_global_format() {
	uint32_t num_of_structs = 0;
	COUNT(num_of_structs, STRUCTS);
	global_format.num_of_structs = num_of_structs;
	global_format.struct_info = (struct struct_info_t*) malloc(sizeof(struct struct_info_t) * global_format.num_of_structs);
	if (global_format.struct_info == NULL) {
		perror("malloc");
		return -1;
	}
	int iterator = 0;
	int field_iterator = 0;
	STRUCTS(MAKE_STRUCT_DEFS);
	return 0;
}

int send_format_to_server(int fd) {
	uint32_t num_structs = htonl(global_format.num_of_structs);
	if (send_data(fd, &num_structs, 4 )!= 4) {
		printf("Error while sending num of structs to server\n");
		return -1;
	}
	for(int i=0; i<global_format.num_of_structs; i++) {
		char terminator = '\0';
		uint32_t size = 0;
		size+=sizeof(global_format.struct_info[i].essential);
		size+=strlen(global_format.struct_info[i].identifier)+1;
		for(int j=0;j<global_format.struct_info[i].num_of_fields; j++) {
			size+=sizeof(global_format.struct_info[i].field_info[j].essential);
			size+=strlen(global_format.struct_info[i].field_info[j].type)+1;
			size+=strlen(global_format.struct_info[i].field_info[j].identifier)+1;
			size+=sizeof(global_format.struct_info[i].field_info[j].size);
		}
		uint32_t size_net = htonl(size);
		if (send_data(fd, &size_net, sizeof(size_net)) != 4) {
			printf("Error while sending packet size to server\n");
			return -2;
		}

		if (send_data(fd, &global_format.struct_info[i].essential, 1) != 1) {
			printf("Error while sending struct essential to server\n");
			return -3;
		}
		if (send_data(fd, global_format.struct_info[i].identifier, strlen(global_format.struct_info[i].identifier)) != strlen(global_format.struct_info[i].identifier)) {
			printf("Error while sending struct name to server\n");
			return -4;
		}
		if (send_data(fd, &terminator, sizeof(terminator)) != 1) {
			printf("Error while sending null terminator to server\n");
			return -5;
		}
		for(int j=0;j<global_format.struct_info[i].num_of_fields; j++) {
			if (send_data(fd, &global_format.struct_info[i].field_info[j].essential, 1) != 1) {
				printf("Error while sending field essential to server\n");
				return -6;
			}
			if (send_data(fd, &global_format.struct_info[i].field_info[j].size, 1) != 1) {
				printf("Error while sending field size to server\n");
				return -7;
			}
			
			if (send_data(fd, global_format.struct_info[i].field_info[j].type, strlen(global_format.struct_info[i].field_info[j].type)) != strlen(global_format.struct_info[i].field_info[j].type)) {
				printf("Error while sending field type to server\n");
				return -9;
			}
			if (send_data(fd, &terminator, sizeof(terminator)) != 1) {
				printf("Error while sending null terminator to server\n");
				return -5;
			}
			printf("essential=%d ; size=%d\n", global_format.struct_info[i].field_info[j].essential, global_format.struct_info[i].field_info[j].size );
			if (send_data(fd, global_format.struct_info[i].field_info[j].identifier, strlen(global_format.struct_info[i].field_info[j].identifier)) != strlen(global_format.struct_info[i].field_info[j].identifier)) {
				printf("Error while sending field name to server\n");
				return -9;
			}
			if (send_data(fd, &terminator, sizeof(terminator)) != 1) {
				printf("Error while sending null terminator to server\n");
				return -5;
			}
		}
	}
	return 0;
}

struct format_mask_t* malloc_mask() {
	struct format_mask_t* mask = (struct format_mask_t*) malloc(sizeof(struct format_mask_t));
	if (mask == NULL) {
		perror("malloc");
		return NULL;
	}
	mask->num_of_structs = global_format.num_of_structs;
	mask->struct_mask = (struct struct_mask_t*) malloc(sizeof(struct struct_mask_t)*mask->num_of_structs);
	if (mask->struct_mask == NULL) {
		perror("malloc");
		free(mask);
		return NULL;
	}
	for(int i=0;i<mask->num_of_structs;i++){
		mask->struct_mask[i].num_of_fields = global_format.struct_info[i].num_of_fields;
		mask->struct_mask[i].field_mask = (uint8_t*) malloc(mask->struct_mask[i].num_of_fields);
		memset(mask->struct_mask[i].field_mask, 0, mask->struct_mask[i].num_of_fields);
		if (mask->struct_mask[i].field_mask == NULL) {
			perror("malloc");
			for (int j=i;j>0;j--) {
				free(mask->struct_mask[j].field_mask);
			}
			free(mask);
			return NULL;
		}
	}
	return mask;
}
void free_mask(struct format_mask_t* mask) {
	for (int i=0;i<mask->num_of_structs;i++) {
		free(mask->struct_mask[i].field_mask);
	}
	free(mask->struct_mask);
	free(mask);
}
struct redir_table_t* malloc_redir_table() {
	struct redir_table_t* redir;
	redir = (struct redir_table_t*) malloc(sizeof(struct redir_table_t));
	redir->struct_remap=(uint32_t*) malloc(sizeof(uint32_t)*global_format.num_of_structs);
	memset(redir->struct_remap, UINT32_MAX, sizeof(uint32_t)*global_format.num_of_structs);
	redir->field_remap=(uint32_t**) malloc(sizeof(uint32_t*)*global_format.num_of_structs);
	for (int i=0;i<global_format.num_of_structs;i++) {
		redir->field_remap[i] = malloc(sizeof(uint32_t)*global_format.struct_info[i].num_of_fields);
		memset(redir->field_remap[i], UINT32_MAX, sizeof(uint32_t)*global_format.struct_info[i].num_of_fields);
	}
	return redir;
}
void free_redir_table(struct redir_table_t* redir) {
	if (redir == NULL) {
		return;
	}
	free(redir->struct_remap);

	if (redir->field_remap == NULL) {
		return;
	}
	for (int i=0;i<global_format.num_of_structs;i++) {
		free(redir->field_remap[i]);
		redir->field_remap[i] = NULL;
	}
	return;
}
int32_t generate_mask_from_client(int fd, struct format_mask_t* mask) {
	int32_t total_num_of_struct=0;

	uint32_t num_of_structs;
	int err=0;
	if ((err = recv_data(fd, &num_of_structs, 4)) != 4) {
		printf("Error reciving num_of_supported_structs: %d\n", err);
		return  -6;
	}
	num_of_structs = ntohl(num_of_structs);

	for (int i=0;i<num_of_structs;i++) {
		printf("i=%d\n", i);
		uint32_t size=0;
		if ((err = recv_data(fd, &size, 4)) != 4) {
			printf("Error reciving packet_size: %d\n", err);
			return -6;
		}
		size = ntohl(size);
		uint8_t* data = (uint8_t*) malloc(size);
		if (data == NULL) {
			perror("malloc");
			return -5;

		}
		if ((err =recv_data(fd, data, size)) != size) {
			printf("Error reciving data: %d\n", err);
			free(data);
			return -6;
		}
		FILE* fptr = fopen("./log_recv_data", "w");
		fwrite(data, 1, size, fptr);
		fclose(fptr);
		uint32_t iter=0;

		uint8_t is_essential=0;
		READ_SAFE_FORMAT(is_essential, iter, size, data,free(data);, "struct")
		
		size_t struct_name_len = strnlen((const char*) data+iter, size-iter);
		if (size-iter == struct_name_len) {
			free(data);
			printf("Struct not null terminated\n");
			return -4;
		}
		

		for (int j=0;j<global_format.num_of_structs;j++)  {
			if (strcmp(global_format.struct_info[j].identifier, (char*) data+iter) == 0) {
				total_num_of_struct++;
				iter+=struct_name_len+1;
				while (size-iter != 0) {

					uint8_t essential=0;
					READ_SAFE_FORMAT(essential, iter, size, data,free(data);, "field")

					uint8_t field_size=0;
					READ_SAFE_FORMAT(field_size, iter, size, data,free(data);,"field")
					printf("essential=%d ; size=%d\n", essential, field_size);

					size_t field_type_len = strnlen((const char*) data+iter, size-iter);
					if (size-iter == field_type_len) {
						free(data);
						printf("Field type not null terminated\n");
						return -4;
					}
					size_t field_name_len = strnlen((const char*) data+iter+field_type_len+1, size-(iter+field_type_len+1));
					if (size-iter == field_name_len) {
						free(data);
						printf("Field name not null terminated\n");
						return -4;
					}
					for (int k=0;k<global_format.struct_info[j].num_of_fields;k++) {
						if (strcmp(global_format.struct_info[j].field_info[k].identifier, (char*)data+iter+field_type_len+1) == 0) {
							if(field_size != global_format.struct_info[j].field_info[k].size) {
								printf("Non-equal size for field %s\n", global_format.struct_info[j].field_info[k].identifier);
								goto fail_field;
							}
							if (strcmp(global_format.struct_info[j].field_info[k].type, (char*)data+iter) != 0) {
								printf("Type confusion in field %s\n", global_format.struct_info[j].field_info[k].identifier);
								goto fail_field;
							}
							mask->struct_mask[j].field_mask[k] = 1;
							iter+=field_name_len+1+field_type_len+1;
							goto skip;
						}
					}
				fail_field:
					iter+= field_name_len+1+field_type_len+1;
					if (essential == 1) {
						free(data);
						printf("Essential field not supported by server\n");
						return -2;
					}
				skip:
					continue;
				}
				goto skip_outer;
			}
		} 
		if (is_essential == 1) {
			free(data);
			printf("Essential struct not supported by server\n");
			return -2;
		}
	skip_outer:
		free(data);
		continue;
	}
	// clean up the mask and check for server requirements
	for (int i=0;i<global_format.num_of_structs;i++) {
		
		uint8_t struct_exists=0;
		for(int j=0;j < global_format.struct_info[i].num_of_fields;j++) {
			if(mask->struct_mask[i].field_mask[j] != 0) {
				struct_exists = 1;
			}

			printf("struct %s:%s:essential: %u\n",global_format.struct_info[i].identifier, global_format.struct_info[i].field_info[j].identifier, global_format.struct_info[i].field_info[j].essential);

			if (global_format.struct_info[i].field_info[j].essential == 1 && mask->struct_mask[i].field_mask[j] == 0) {
				printf("Essential field not supported by client\n");
				return -1; 
			} 
		}
		if (struct_exists == 0) {
			free(mask->struct_mask[i].field_mask);
			mask->struct_mask[i].field_mask = NULL;
			if (global_format.struct_info[i].essential == 1) {
				printf("Essential struct not supported by client\n");
				return -1;
			}
		}
	}
	return total_num_of_struct;
}


int send_format_to_client(int fd, struct format_mask_t* mask, int32_t num_of_structs) {
	int32_t num_of_structs_net = htonl(num_of_structs);
	if (send_data(fd, &num_of_structs_net, 4) != 4) {
		perror("send_num_of_structs");
		return -6;
	}
	if (num_of_structs < 0) {
		return num_of_structs;
	}
	DEBUG_PRINT;
	for (uint32_t i=0;i<global_format.num_of_structs;i++) {
		uint32_t size=0;
		if (mask->struct_mask[i].field_mask != NULL) {
			DEBUG_PRINT;
			size+=strlen(global_format.struct_info[i].identifier); // struct_name
			size++; // null terminator
			size+=sizeof(uint32_t); // struct_identifier
			for(int j=0;j<global_format.struct_info[i].num_of_fields;j++) {
				if (mask->struct_mask[i].field_mask[j] != 0) {
					size++; // size
					size += strlen(global_format.struct_info[i].field_info[j].identifier);
					size++; // null terminator
				}
			}
			printf("send format size: %u\n", size);
			size = htonl(size);
			if (send_data(fd, &size, 4) != 4) {
				perror("send_size");
				return -6;
			}
			
			DEBUG_PRINT;
			if (send_data(fd, global_format.struct_info[i].identifier, strlen(global_format.struct_info[i].identifier)) !=strlen(global_format.struct_info[i].identifier)) {
				perror("send_struct_name");
				return -6;
			}
			DEBUG_PRINT;
			char null_terminator = '\0';
			if (send_data(fd, &null_terminator, 1) != 1) {
				perror("send_null_terminator");
				return -6;
			}
			DEBUG_PRINT;
			uint32_t i_net = htonl(i);
			if (send_data(fd, &i_net, 4) != 4) {
				perror("send_struct_identifier");
				return -6;
			}
			DEBUG_PRINT;
			for(int j=0;j<global_format.struct_info[i].num_of_fields;j++) {
				if (mask->struct_mask[i].field_mask[j] != 0) {
					if (send_data(fd, mask->struct_mask[i].field_mask+j, 1) != 1) {
						perror("send_field_size");
						return -6;
					}
					DEBUG_PRINT;
					if (send_data(fd, global_format.struct_info[i].field_info[j].identifier, strlen(global_format.struct_info[i].field_info[j].identifier)) != strlen(global_format.struct_info[i].field_info[j].identifier)) {
						perror("send_field_identifier");
						return -6;
					}
					DEBUG_PRINT;
					if (send_data(fd, &null_terminator, 1) != 1) {
						perror("send_null_terminator");
						return -6;
					}
				}
				
			}
		}
	}
	return 0;

}


// Params: recv_fd, pointer to malloced mask, pointer to var that schould store redir_table
int32_t generate_format_from_server(int fd, struct redir_table_t* redir) {
	int32_t num_of_structs=0;
	if (recv_data(fd,&num_of_structs, 4) != 4) {
		perror("recv_num_of_structs");
		return -6;
	}
	num_of_structs = ntohl(num_of_structs);
	if (num_of_structs < 0) {
		printf("error happened: %d\n", num_of_structs);
		return num_of_structs;
	}

	// loop until you receive all suported structs and their format
	for (int i=0;i<num_of_structs;i++) {
		uint32_t size=0;
		if (recv_data(fd, &size, 4) != 4) {
			perror("recv_error_size");
			return -6;
		}
		size = ntohl(size);
		uint8_t* data = malloc(size);
		if (data == NULL) {
			perror("malloc");
			return -5;
		}
		if (recv_data(fd, data, size) != size) {
			free(data);
			perror("recv_data");
			return -6;
		}

		uint32_t iter=0;

		size_t struct_name_len = strnlen((const char*) data+iter, size-iter);
		if (size-iter == struct_name_len) {
			free(data);
			printf("Struct not null terminated\n");
			return -4;
		}
		uint32_t identifier=UINT32_MAX;
		for (int j=0;j<global_format.num_of_structs;j++) {
			if (strcmp((char*) data+iter, global_format.struct_info[j].identifier) == 0) {
				iter+=struct_name_len+1;

				READ_SAFE_FORMAT(identifier, iter, size, data, free(data);,"struct");
				identifier = ntohl(identifier);

				uint32_t field_count=0;
				while (size-iter > 0) {
					uint8_t field_size;
					READ_SAFE_FORMAT(field_size, iter, size, data,free(data);,"field");

					size_t field_name_len = strnlen((const char*) data+iter, size-iter);
					if (size-iter == field_name_len) {
						free(data);
						printf("Field not null terminated\n");
						return -4;
					}
					for (int k=0;k<global_format.struct_info[j].num_of_fields;k++) {
						if (strcmp((char*) data+iter, global_format.struct_info[j].field_info[k].identifier) == 0) {
							iter += field_name_len+1;
							if (field_count > global_format.struct_info[j].num_of_fields) {
								printf("too much fields read\n");
								free(data);
								return -5;
							}
							// reverse so nth element in order in at kth place in global_format
							redir->field_remap[j][field_count] = k;
							field_count++;
							break;
						}
					}
				}
				if (field_count > 0) {
					redir->struct_remap[j] = identifier;
				}
				break;
			}
		}
		if (identifier == UINT32_MAX) {
			free(data);
			printf("invalid struct identifier\n");
			return -4;
		}
	}
	// cleanup unused structs
	for (int i=0;i<global_format.num_of_structs;i++) {
		if (redir->struct_remap[i] == UINT32_MAX) {
			free(redir->field_remap[i]);
			redir->field_remap[i] = NULL;
		}
	}
	return 0;
}

STRUCTS(MAKE_SUB_GET_FIELD)
void* get_field(enum structs struct_id, void* object, uint32_t field_id) {
	void* (*sub_funcs[])(void*, uint32_t) = {STRUCTS(MAKE_FUNCTION_POINTERS_SUB_GET)};
	if (struct_id >= global_format.num_of_structs) {
		printf("%s: struct_id too big\n", __func__);
		return NULL;
	}
	if (field_id >= global_format.struct_info[struct_id].num_of_fields) {
		printf("%s: field_id too big\n", __func__);
		return NULL;
	}
	if (object == NULL) {
		printf("%s: object is NULL\n", __func__);
		return NULL;
	}
	return sub_funcs[struct_id](object, field_id);
}
STRUCTS(MAKE_SUB_GET_NMEMB_FIELD)
void* get_field_nmemb(enum structs struct_id, void* object, uint32_t field_id) {
	void* (*sub_funcs[])(void*, uint32_t) = {STRUCTS(MAKE_FUNCTION_POINTERS_SUB_GET_NMEMB)};
	if (struct_id >= global_format.num_of_structs) {
		printf("%s: struct_id too big\n", __func__);
		return NULL;
	}
	if (field_id >= global_format.struct_info[struct_id].num_of_fields) {
		printf("%s: field_id too big\n", __func__);
		return NULL;
	}
	if (global_format.struct_info[struct_id].field_info[field_id].is_ptr == 0) {
		printf("Attempted to get nmemb of NORMAL field %s:%s\n", global_format.struct_info[struct_id].identifier, global_format.struct_info[struct_id].field_info[field_id].identifier);
		return NULL;
	}
	if (object == NULL) {
		printf("%s: object is NULL\n", __func__);
		return NULL;
	}
	return sub_funcs[struct_id](object, field_id);
}

int send_struct_server(int fd,enum structs struct_id, void* src, struct format_mask_t* mask) {
	if (struct_id == STRUCT_ANY) {
		printf("Cannot send unknown struct\n");
		return -5;
	}
	if (mask->struct_mask[struct_id].field_mask == NULL) {
		printf("Attempted to send masked struct %s\n", global_format.struct_info[struct_id].identifier);
		return -7;
	}
	uint32_t size = 0;
	for (int i=0;i<global_format.struct_info[struct_id].num_of_fields;i++) {
		if (mask->struct_mask[struct_id].field_mask[i] != 0) {
			if (global_format.struct_info[struct_id].field_info[i].is_ptr == 1) {
				size+=4;
				size += global_format.struct_info[struct_id].field_info[i].size*(*((uint32_t*)get_field_nmemb(struct_id, src, i)));
			} else {
				size += global_format.struct_info[struct_id].field_info[i].size;
			}
		}
	}
	size += 4; // identifier
	size = htonl(size);
	if (send_data(fd, &size, 4) != 4) {
		perror("Error sending size in send_struct_server");
		return -6;
	}
	uint32_t identifier = htonl(struct_id);
	if (send_data(fd, &identifier, 4) != 4) {
		perror("Error sending identifier in send_struct_server");
		return -6;
	}
	for (int i=0;i<global_format.struct_info[struct_id].num_of_fields;i++) {
		if (mask->struct_mask[struct_id].field_mask[i] != 0) {
			// TODO: Add serialization (byte order)
			if (global_format.struct_info[struct_id].field_info[i].is_ptr == 1) {
				if (send_data(fd, get_field_nmemb(struct_id, src, i), 4) != 4) {
					printf("Error sending nmemb_field\n");
					return -6;
				}
				if (send_data(fd, *((void**)get_field(struct_id, src, i)), global_format.struct_info[struct_id].field_info[i].size*(*((uint32_t*)get_field_nmemb(struct_id, src, i)))) != global_format.struct_info[struct_id].field_info[i].size*(*((uint32_t*)get_field_nmemb(struct_id, src, i)))) {
					perror("Error sending ptr_field");
					return -6;
				}
			} else {
				if (send_data(fd, get_field(struct_id, src, i), global_format.struct_info[struct_id].field_info[i].size) != global_format.struct_info[struct_id].field_info[i].size) {
					perror("Error sending field");
					return -6;
				}

			}
		}
	}
	return 0;
}

int send_struct_client(int fd,enum structs struct_id, void* src, struct redir_table_t* redir) {
	if (struct_id == STRUCT_ANY) {
		printf("Cannot send unknown struct\n");
		return -5;
	}
	if (redir->struct_remap[struct_id] == UINT32_MAX) {
		printf("Attempted to send \"masked\" struct %s\n", global_format.struct_info[struct_id].identifier);
		return -7;
	}
	uint32_t size = 0;
	for (int i=0;i<global_format.struct_info[struct_id].num_of_fields;i++) {
		if (redir->field_remap[struct_id][i] == UINT32_MAX) {
			// no need to continue the loop all the next ones are UINT32_MAX
			break;
		}
		if (global_format.struct_info[struct_id].field_info[redir->field_remap[struct_id][i]].is_ptr == 1) {
			size+=4;
			size += global_format.struct_info[struct_id].field_info[redir->field_remap[struct_id][i]].size*(*((uint32_t*)get_field_nmemb(struct_id, src, redir->field_remap[struct_id][i])));
		} else {
			size += global_format.struct_info[struct_id].field_info[redir->field_remap[struct_id][i]].size;
		}
	}
	size += 4; // identifier
	size = htonl(size);
	if (send_data(fd, &size, 4) != 4) {
		perror("Error sending size in send_struct_client");
		return -6;
	}
	uint32_t identifier = htonl(redir->struct_remap[struct_id]);
	if (send_data(fd, &identifier, 4) != 4) {
		perror("Error sending identifier in send_struct_client");
		return -6;
	}
	printf("num_fields: %u\n", global_format.struct_info[struct_id].num_of_fields);
	for (int i=0;i<global_format.struct_info[struct_id].num_of_fields;i++) {
		if (redir->field_remap[struct_id][i] == UINT32_MAX) {
			return 0;
		}
		printf("I WAS CALLED!!\n");
		// TODO: Add serialization (byte order)
		if (global_format.struct_info[struct_id].field_info[redir->field_remap[struct_id][i]].is_ptr == 1) {
			DEBUG_PRINT;
			if (send_data(fd, get_field_nmemb(struct_id, src, redir->field_remap[struct_id][i]), 4) != 4) {
				printf("Error sending nmemb_field\n");
				return -6;
			}
			if (send_data(fd, *((void**)get_field(struct_id, src, redir->field_remap[struct_id][i])), global_format.struct_info[struct_id].field_info[redir->field_remap[struct_id][i]].size*(*((uint32_t*)get_field_nmemb(struct_id, src, redir->field_remap[struct_id][i])))) != global_format.struct_info[struct_id].field_info[redir->field_remap[struct_id][i]].size*(*((uint32_t*)get_field_nmemb(struct_id, src, redir->field_remap[struct_id][i])))) {
				perror("Error sending ptr_field");
				return -6;
			}
		} else {
			if (send_data(fd, get_field(struct_id, src, redir->field_remap[struct_id][i]), global_format.struct_info[struct_id].field_info[redir->field_remap[struct_id][i]].size) != global_format.struct_info[struct_id].field_info[redir->field_remap[struct_id][i]].size) {
				perror("Error sending field");
				return -6;
			}
		}
		
	}
	return 0;
}
int recv_struct_server(int fd,enum structs struct_id,void* dest, struct format_mask_t* mask) {
	uint32_t size;
	if (recv_data(fd, &size, 4) != 4) {
		perror("Error recv size in recv_struct_client");
		return -6;
	}
	size = ntohl(size);
	printf("server_recv size: %u\n", size);
	uint8_t* data = (uint8_t*) malloc(size);
	if (data == NULL) {
		perror("malloc");
		return -5;
	}
	if (recv_data(fd, data, size) != size) {
		perror("Error recv data in recv_struct_client");
		free(data);
		return -6;
	}
	uint32_t iter=0;

	uint32_t identifier;
	READ_SAFE_FORMAT(identifier, iter, size, data, free(data);, "packet");
	identifier = ntohl(identifier);
	if (identifier >= global_format.num_of_structs) {
		printf("Wrong identifier in recv_struct_server\n");
		free(data);
		return -4;
	}
	if (identifier != struct_id && (int32_t)identifier != STRUCT_ANY) {
		printf("Expected different identifier in recv_struct_server\n");
		free(data);
		return -4;

	}
	if (mask->struct_mask[identifier].field_mask == NULL) {
		printf("Attempted to recv masked struct %s\n", global_format.struct_info[identifier].identifier);
		free(data);
		return -7;
	}
	for (int i=0;i<global_format.struct_info[identifier].num_of_fields;i++) {
		if (mask->struct_mask[identifier].field_mask[i] != 0) {
			if (global_format.struct_info[identifier].field_info[i].is_ptr == 1) {
				if (4 > size-iter) {
					printf("Not enough data received in recv_struct_server\n");
					free(data);
					return -4;
				}
				memcpy(get_field_nmemb(identifier, dest, i), data+iter, 4);
				iter+=4;
				uint32_t nmemb_field = *((uint32_t*)get_field_nmemb(identifier,dest,i));
				if (global_format.struct_info[identifier].field_info[i].size*nmemb_field > size-iter) {
					printf("Not enough data received in recv_struct_server\n");
					free(data);
					return -4;
				}
				uint8_t* buf = (uint8_t*)malloc(global_format.struct_info[identifier].field_info[i].size*nmemb_field);
				memcpy(buf, data+iter, global_format.struct_info[identifier].field_info[i].size*nmemb_field);
				// get size of generic data pointer
				memcpy(get_field(identifier, dest, i), &buf, sizeof(void*));
				iter+=global_format.struct_info[identifier].field_info[i].size*nmemb_field;
			} else {
				if (global_format.struct_info[identifier].field_info[i].size > size-iter) {
					printf("Not enough data received in recv_struct_server\n");
					free(data);
					return -4;
				}
				memcpy(get_field(identifier, dest, i), data+iter, global_format.struct_info[identifier].field_info[i].size);
				iter +=global_format.struct_info[identifier].field_info[i].size;
			}
		}
	}
	if (size-iter != 0) {
		printf("Too much data provided\n");
		free(data);
		return -4;
	}
	free(data);
	return 0;
}
int recv_struct_client(int fd,enum structs struct_id, void* dest, struct redir_table_t* redir) {
	uint32_t size = 0;
	if (recv_data(fd, &size, 4) != 4) {
		perror("Error recv size in recv_struct_client");
		return -6;
	}
	size = ntohl(size);
	printf("size: %u\n",size);
	uint8_t* data = (uint8_t*) malloc(size);
	if (data == NULL) {
		perror("malloc");
		return -5;
	}
	if (recv_data(fd, data, size) != size) {
		perror("Error recv data in recv_struct_client");
		free(data);
		return -6;
	}
	uint32_t iter=0;

	uint32_t identifier;
	READ_SAFE_FORMAT(identifier, iter, size, data, free(data);, "packet");
	identifier = ntohl(identifier);

	// find out local identifier
	for (int i=0;i<global_format.num_of_structs;i++) {
		if (redir->struct_remap[i] == identifier) {
			identifier = i;
			goto loop_success;
		}
	}
	printf("Wrong (prob. masked) identifier in recv_struct_client\n");
	free(data);
	return -7;
loop_success:
	if (identifier != struct_id && (int32_t)identifier != STRUCT_ANY) {
		printf("Expected different identifier in recv_struct_client\n");
		free(data);
		return -4;
	}
	for (int i=0;i<global_format.struct_info[identifier].num_of_fields;i++) {
		if (redir->field_remap[identifier][i] == UINT32_MAX) {
			DEBUG_PRINT;
			break;
		}
		if (global_format.struct_info[identifier].field_info[redir->field_remap[identifier][i]].is_ptr == 1) {
			if (4 > size-iter) {
				printf("Not enough data received in recv_struct_client\n");
				free(data);
				return -4;
			}
			memcpy(get_field_nmemb(identifier, dest, redir->field_remap[identifier][i]), data+iter, 4);
			printf("nmemb: %u\n", *((uint32_t*)get_field_nmemb(identifier, dest, redir->field_remap[identifier][i])));

			iter+=4;
			uint32_t nmemb_field = *((uint32_t*)get_field_nmemb(identifier,dest,redir->field_remap[identifier][i]));
			if (global_format.struct_info[identifier].field_info[redir->field_remap[identifier][i]].size*nmemb_field > size-iter) {
				printf("Not enough data received in recv_struct_client\n");
				free(data);
				return -4;
			}
			uint8_t* buf = (uint8_t*)malloc(global_format.struct_info[identifier].field_info[redir->field_remap[identifier][i]].size*nmemb_field);
			memcpy(buf, data+iter, global_format.struct_info[identifier].field_info[redir->field_remap[identifier][i]].size*nmemb_field);
			// get size of generic data pointer
			memcpy(get_field(identifier, dest, redir->field_remap[identifier][i]), &buf, sizeof(void*));
			iter+=global_format.struct_info[identifier].field_info[redir->field_remap[identifier][i]].size*nmemb_field;
		} else {
			if (global_format.struct_info[identifier].field_info[redir->field_remap[identifier][i]].size > size-iter) {
				printf("Error size to small in recv_struct_client\n");
				return -4;
			}
			memcpy(get_field(identifier, dest, redir->field_remap[identifier][i]),data+iter, global_format.struct_info[identifier].field_info[redir->field_remap[identifier][i]].size);
			iter +=global_format.struct_info[identifier].field_info[redir->field_remap[identifier][i]].size;
		}
	}
	return 0;
}

