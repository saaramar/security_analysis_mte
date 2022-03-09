#define CHUNK_SIZE 4
#define MAX_FREEPOOL 256

typedef char* FREEPOOL;

typedef enum {
  __LIST_HEAD = 0,
  LIST_LONG,
  LIST_DOUBLE,
  LIST_STRING,
  __LIST_BOTTOM
} LIST_TYPE;

typedef union {
  char *p_char;
  long d_long;
  double d_double;
} Data;

typedef struct {
  Data *data;
  size_t size;
  size_t max;
} List;

List* list_new(void);
void list_add(List*, Data, LIST_TYPE);
Data list_get(List*, int);
void list_edit(List*, int, Data, LIST_TYPE);
void list_del(List*, int);
void __list_abort(const char*);
