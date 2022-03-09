#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "diylist.h"

long read_long(void)
{
  char buf[32];
  read(0, buf, 30);
  return atol(buf);
}

double read_double(void)
{
  char buf[16];
  read(0, buf, 14);
  return atof(buf);
}

void read_str(char *buf)
{
  int size = read(0, buf, 127);
  char *nl = strchr(buf, '\n');
  if (nl != NULL) {
    *nl = '\x00';
  } else {
    buf[size] = 0;
  }
}

long menu(void)
{
  puts("1. list_add");
  puts("2. list_get");
  puts("3. list_edit");
  puts("4. list_del");
  printf("> ");
  return read_long();
}

void add(List *list)
{
  char buf[128];
  printf("Type(long=%d/double=%d/str=%d): ", LIST_LONG, LIST_DOUBLE, LIST_STRING);
  
  switch(read_long()) {
  case LIST_LONG:
    printf("Data: ");
    list_add(list, (Data)read_long(), LIST_LONG);
    break;
    
  case LIST_DOUBLE:
    printf("Data: ");
    list_add(list, (Data)read_double(), LIST_DOUBLE);
    break;
    
  case LIST_STRING:
    printf("Data: ");
    read_str(buf);
    list_add(list, (Data)buf, LIST_STRING);
    break;
    
  default:
    puts("Invalid option");
    return;
  }
}

void get(List *list)
{
  printf("Index: ");
  long index = read_long();
  
  printf("Type(long=%d/double=%d/str=%d): ", LIST_LONG, LIST_DOUBLE, LIST_STRING);
  
  switch(read_long()) {
  case LIST_LONG:
    printf("Data: %ld\n", list_get(list, index).d_long);
    break;
    
  case LIST_DOUBLE:
    printf("Data: %lf\n", list_get(list, index).d_double);
    break;
    
  case LIST_STRING:
    printf("Data: %s\n", list_get(list, index).p_char);
    break;
    
  default:
    puts("Invalid option");
    return;
  }
}

void edit(List *list)
{
  char buf[128];
  
  printf("Index: ");
  long index = read_long();
  printf("Type(long=%d/double=%d/str=%d): ", LIST_LONG, LIST_DOUBLE, LIST_STRING);
  
  switch(read_long()) {
  case LIST_LONG: /* long */
    printf("Data: ");
    list_edit(list, index, (Data)read_long(), LIST_LONG);
    break;
    
  case LIST_DOUBLE: /* double */
    printf("Data: ");
    list_edit(list, index, (Data)read_double(), LIST_DOUBLE);
    break;
    
  case LIST_STRING: /* str */
    printf("Data: ");
    read_str(buf);
    list_edit(list, index, (Data)buf, LIST_STRING);
    break;
    
  default:
    puts("Invalid option");
    return;
  }
}

void del(List *list)
{
  printf("Index: ");
  long index = read_long();

  list_del(list, index);
  puts("Successfully removed");
}

void initialize(void)
{
  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

}

int main(void)
{
  initialize();
  
  List *list = list_new();

  while(1) {
    switch(menu()) {
    case 1: add(list); break;
    case 2: get(list); break;
    case 3: edit(list); break;
    case 4: del(list); break;
    }
  }
}
