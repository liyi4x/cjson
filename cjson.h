#ifndef _CJSON_H_
#define _CJSON_H_

#include <stdint.h>
#include <stddef.h>

typedef enum{
  CJSON_OK,
  CJSON_ERR_LITERAL,      //文字解析错误
  CJSON_ERR_NUMBER,
  CJSON_ERR_STRING_MISS_QUOTATION_MARK,       //缺少双引号
  CJSON_ERR_STRING_INVALID_ESCAPE_CAHR,       //转义字符错误
  CJSON_ERR_STACK,
  CJSON_ERR_UNICODE
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

typedef struct
{
  cjson_type type;
  union
  {
    double num;
    struct{
      char *buf;
      size_t l;
    }str;
  }u;
}cjson_value;



CJSON_STATUS cjson_parse(cjson_value *v, const char *json);

cjson_type cjson_get_type(cjson_value value);

int cjson_get_boolean(cjson_value value);
void cjson_set_boolean(cjson_value *value, int bool);

double cjson_get_number(cjson_value value);
void cjson_set_number(cjson_value *value, double num);

size_t cjson_get_string_length(cjson_value value);
char* cjson_get_string(cjson_value value);
void cjson_set_string(cjson_value *value, const char *buf, size_t len);


#endif