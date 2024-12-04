#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PROTOCOL_H
#define PROTOCOL_H

struct field_info_t {
  uint8_t size;
  uint8_t essential;
  uint8_t is_ptr;
  const char *type;
  const char *identifier;
};
struct struct_info_t {
  uint32_t num_of_fields;
  uint8_t essential;
  const char *identifier;
  struct field_info_t *field_info;
};

struct format_t {
  uint32_t num_of_structs;
  struct struct_info_t *struct_info;
};

extern struct format_t global_format;

struct struct_mask_t {
  uint32_t num_of_fields;
  // 0 for false, 1 for true
  uint8_t *field_mask;
};
struct format_mask_t {
  uint32_t num_of_structs;
  struct struct_mask_t *struct_mask;
};

// used only for clients. Server does not use this
struct redir_table_t {
  uint32_t *struct_remap;
  uint32_t **field_remap;
};

#define DEBUG_PRINT printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);

#define READ_SAFE_FORMAT(var, iter, size, data, error_handle, msg)             \
  if (size - iter < sizeof(var)) {                                             \
    printf(msg " " #var " not found\n");                                       \
    error_handle;                                                              \
    return -4;                                                                 \
  }                                                                            \
  memcpy(&var, data + iter, sizeof(var));                                      \
  iter += sizeof(var);

#define __COUNT(...) i++;

// count num of calls of macro list
#define COUNT(var, macro_list)                                                 \
  {                                                                            \
    uint64_t i = 0;                                                            \
    macro_list(__COUNT);                                                       \
    var = i;                                                                   \
  }

//	_F(MACRO_NAME, type_name, essential(1 means yes))
#define STRUCTS(_F, ...)                                                       \
  _F(STRUCT2, "STRUCT2", struct2, 0)                                           \
  _F(STRUCT1, "STRUCT1", struct1, 1)                                           \

//	_F(field_name, type, ptr, indentifier, essential, size of (one) member)
#define STRUCT1_FIELDS(_F, ...)                                                \
  _F(y, char*, PTR, "y", 1, sizeof(char)) \
  _F(x, const uint32_t ,NORMAL, "x", 0, 4)\

#define STRUCT2_FIELDS(_F, ...) \
  _F(z, char, NORMAL, "z", 1, 1)

#define MAKE_STRUCT_FIELD_DEF_NORMAL(field_name, type) \
  type field_name;

#define MAKE_STRUCT_FIELD_DEF_PTR(field_name, type)   \
  uint32_t nmemb_##field_name;                        \
  type field_name;

#define MAKE_STRUCT_FIELD_DEF(field_name, type,ptr, id, essential, size)       \
  MAKE_STRUCT_FIELD_DEF_##ptr(field_name, type)

#define MAKE_STRUCT_DEF(macro, id, type_name, essential)                       \
  struct type_name {                                                           \
    macro##_FIELDS(MAKE_STRUCT_FIELD_DEF)                                      \
  };



#define MAKE_FIELD_DEFS_NORMAL(field_name, type_char,id, essential_num, size_num)              \
  global_format.struct_info[iterator].field_info[field_iterator].essential = essential_num; \
  global_format.struct_info[iterator].field_info[field_iterator].size = size_num;           \
  global_format.struct_info[iterator].field_info[field_iterator].is_ptr = 0;                \
  global_format.struct_info[iterator].field_info[field_iterator].type = #type_char ;        \
  global_format.struct_info[iterator].field_info[field_iterator].identifier =id;            \
  field_iterator++;

#define MAKE_FIELD_DEFS_PTR(field_name, type_char,id, essential_num, size_num)              \
  global_format.struct_info[iterator].field_info[field_iterator].essential = essential_num; \
  global_format.struct_info[iterator].field_info[field_iterator].size = size_num;           \
  global_format.struct_info[iterator].field_info[field_iterator].is_ptr = 1;                \
  global_format.struct_info[iterator].field_info[field_iterator].type = #type_char ;        \
  global_format.struct_info[iterator].field_info[field_iterator].identifier =id;            \
  field_iterator++;

#define MAKE_FIELD_DEFS(field_name, type_char,ptr, id, essential_num, size_num)              \
  MAKE_FIELD_DEFS_##ptr(field_name, type_char,id, essential_num, size_num)

#define MAKE_STRUCT_DEFS(macro, id, type_name, essential_num)                  \
  COUNT(global_format.struct_info[iterator].num_of_fields, macro##_FIELDS);    \
  global_format.struct_info[iterator].identifier = id;                         \
  global_format.struct_info[iterator].essential = essential_num;               \
  global_format.struct_info[iterator].field_info = (struct field_info_t *)malloc(sizeof(struct field_info_t) * global_format.struct_info[iterator].num_of_fields); \
  if (global_format.struct_info[iterator].field_info == NULL) {                \
    perror("malloc");                                                          \
    return -1;                                                                 \
  }                                                                            \
  field_iterator = 0;                                                          \
  macro##_FIELDS(MAKE_FIELD_DEFS) iterator++;

// implements get_field_ptr (workaroud alignment issues) by index
#define __MAKE_SUB_GET_FIELD(field_name, ...) &(temp->field_name),

#define MAKE_SUB_GET_FIELD(macro, id, type_name, essential)                    \
  void *_get_field_##macro(void *object, uint32_t field_id) {                  \
    struct type_name *temp = (struct type_name *)object;                       \
    void *array[] = {macro##_FIELDS(__MAKE_SUB_GET_FIELD)};                    \
    return array[field_id];                                                    \
  }
#define MAKE_FUNCTION_POINTERS_SUB_GET(macro, id, type_name, essential)        \
  &_get_field_##macro,



#define __MAKE_SUB_GET_NMEMB_FIELD_PTR(field_name) &(temp->nmemb_##field_name),
#define __MAKE_SUB_GET_NMEMB_FIELD_NORMAL(field_name) NULL,

#define __MAKE_SUB_GET_NMEMB_FIELD(field_name, type, ptr, ...) \
  __MAKE_SUB_GET_NMEMB_FIELD_##ptr(field_name)

#define MAKE_SUB_GET_NMEMB_FIELD(macro, id, type_name, essential)                    \
  void *_get_field_nmemb_##macro(void *object, uint32_t field_id) {                  \
    struct type_name *temp = (struct type_name *)object;                       \
    void *array[] = {macro##_FIELDS(__MAKE_SUB_GET_NMEMB_FIELD)};                    \
    return array[field_id];                                                    \
  }

#define MAKE_FUNCTION_POINTERS_SUB_GET_NMEMB(macro, ...) \
  &_get_field_nmemb_##macro,


#define MAKE_ENUM_DEF(macro, id, typename, essential) STRUCT_##macro,

enum structs {STRUCT_ANY=-1,STRUCTS(MAKE_ENUM_DEF) };

STRUCTS(MAKE_STRUCT_DEF)

int generate_global_format();
int32_t generate_mask_from_client(int fd, struct format_mask_t *mask);
struct format_mask_t *malloc_mask();
void free_mask(struct format_mask_t *mask);
int send_format_to_server(int fd);
int send_data(int fd, void *data, size_t size);
int recv_data(int fd, void *data, size_t size);
int send_format_to_client(int fd, struct format_mask_t *mask,
                          int32_t num_of_structs);
int32_t generate_format_from_server(int fd, struct redir_table_t *redir);
void free_redir_table(struct redir_table_t *redir);
struct redir_table_t* malloc_redir_table();

void *get_field(enum structs struct_id, void *object, uint32_t field_id);
int byte_swap(void *data, size_t size);


int send_struct_client(int fd,enum structs struct_id, void* src, struct redir_table_t* redir);
int send_struct_server(int fd,enum structs struct_id, void* src, struct format_mask_t* mask);
int recv_struct_server(int fd,enum structs struct_id,void* dest, struct format_mask_t* mask);
int recv_struct_client(int fd,enum structs struct_id, void* dest, struct redir_table_t* redir);

#endif
