#ifndef _CJSON_H_
#define _CJSON_H_

#include <stdint.h>
#include <stddef.h>

typedef enum{
  CJSON_OK,
  CJSON_ERR_MISS_VALUE,
  CJSON_ERR_ROOT_NOT_SINGULAR,                    //解析根错误
  CJSON_ERR_LITERAL,                              //文字解析错误
  CJSON_ERR_NUMBER_TOO_BIG,
  CJSON_ERR_STRING_MISS_QUOTATION_MARK,           //缺少双引号
  CJSON_ERR_STRING_INVALID_ESCAPE,                //转义字符错误
  CJSON_ERR_STRING_INVALID_CAHR,                  //字符为小于0x20的控制字符
  CJSON_ERR_STACK,
  CJSON_ERR_UNICODE_HEX,                          //utf8 16进制解析错误
  CJSON_ERR_UNICODE_SURROGATE,                    //utf8 代理项错误
  CJSON_ERR_ARRAY_NEED_COMMA_OR_SQUARE_BRACKET,   //数组缺少 ',' 或者 ']'
  CJSON_ERR_OBJECT_NEED_KEY,                      //对象缺少key
  CJSON_ERR_OBJECT_NEED_COLON,                    //对象缺少冒号
  CJSON_ERR_OBJECT_NEED_COMMA_OR_SQUARE_BRACKET   //对象缺少 ',' 或者 '}'
}CJSON_STATUS;

#define CJSON_KEY_NOT_EXIST  ((size_t)-1)

typedef enum{
  CJSON_NULL,
  CJSON_TRUE,
  CJSON_FALSE,
  CJSON_NUMBER,
  CJSON_STRING,
  CJSON_ARRAY,
  CJSON_OBJECT
}cjson_type;

typedef struct cjson_member__ cjson_member;
typedef struct cjson_value__
{
  cjson_type type;
  union
  {
    double num;

    struct
    {
      char *buf;
      size_t l;   //不包括结尾 '\0'
    }str;

    struct
    {
      struct cjson_value__* elements;
      size_t size;  //元素个数
      size_t capacity;  //动态数组容量
    }arr;    //数组

    struct
    {
      cjson_member *members;
      size_t size;  //成员个数
      size_t capacity;  //动态数组容量
    }obj;
  }u;
}cjson_value;

struct cjson_member__
{
  char *key;
  size_t key_len;

  cjson_value value;
};



CJSON_STATUS cjson_parse(cjson_value *v, const char *json);
char *cjson_stringify(cjson_value v, size_t *length);

void cjson_value_free(cjson_value *value);
#define cjson_value_init(v) do { (v)->type = CJSON_NULL; } while(0)
int cjson_is_equal(const cjson_value* lhs, const cjson_value* rhs);
void cjson_copy(cjson_value *dest, const cjson_value *src);
void cjson_move(cjson_value *dest, cjson_value *src);
void cjson_swap(cjson_value *dest, cjson_value *src);

cjson_type cjson_get_type(cjson_value value);

#define cjson_set_null(v) cjson_value_init(v)

int cjson_get_boolean(cjson_value value);
void cjson_set_boolean(cjson_value *value, int bool);

double cjson_get_number(cjson_value value);
void cjson_set_number(cjson_value *value, double num);

size_t cjson_get_string_length(cjson_value value);
char* cjson_get_string(cjson_value value);
void cjson_set_string(cjson_value *value, const char *buf, size_t len);

size_t cjson_get_array_size(cjson_value value);
size_t cjson_get_array_capacity(cjson_value value);
cjson_value *cjson_get_array_element(cjson_value value, size_t index);
cjson_value *cjson_pushback_array_element(cjson_value *value);
cjson_value *cjson_popback_array_element(cjson_value *value);
cjson_value *cjson_insert_array_element(cjson_value *value, size_t index);
void cjson_init_array(cjson_value *value, size_t cap);
void cjson_resize_array(cjson_value *value);
void cjson_shrink_array(cjson_value *value);
void cjson_erase_array_element(cjson_value *value, size_t index, size_t count);
void cjson_clear_array_element(cjson_value *value);

size_t cjson_get_object_size(cjson_value value);
size_t cjson_get_object_capacity(cjson_value value);
const char *cjson_get_object_key(cjson_value value, size_t index);
size_t cjson_get_object_key_length(cjson_value value, size_t index);
cjson_value *cjson_get_object_value(cjson_value value, size_t index);
void cjson_init_object(cjson_value *value, size_t cap);
cjson_value *cjson_set_object_value(cjson_value *value, const char* key, size_t klen);
size_t cjson_find_object_index(cjson_value value, const char* key, size_t klen);
cjson_value *cjson_find_object_value(cjson_value value, const char* key, size_t klen);
void cjson_remove_object_value(cjson_value *value, size_t index);
void cjson_clear_object(cjson_value *value);
void cjson_shrink_object(cjson_value *value);



#endif