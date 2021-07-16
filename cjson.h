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
    }arr;    //数组

    struct
    {
      cjson_member *members;
      size_t size;  //成员个数
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

void cjson_value_init(cjson_value *value);

cjson_type cjson_get_type(cjson_value value);

int cjson_get_boolean(cjson_value value);
void cjson_set_boolean(cjson_value *value, int bool);

double cjson_get_number(cjson_value value);
void cjson_set_number(cjson_value *value, double num);

size_t cjson_get_string_length(cjson_value value);
char* cjson_get_string(cjson_value value);
void cjson_set_string(cjson_value *value, const char *buf, size_t len);

size_t cjson_get_array_size(cjson_value value);
cjson_value *cjson_get_array_element(cjson_value value, size_t index);

size_t cjson_get_object_size(cjson_value value);
const char *cjson_get_object_key(cjson_value value, size_t index);
size_t cjson_get_object_key_length(cjson_value value, size_t index);
cjson_value *cjson_get_object_value(cjson_value value, size_t index);

char *cjson_stringify(cjson_value v, size_t *length);


#endif