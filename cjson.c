#include "cjson.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#ifndef CJSON_STACK_SIZE
#define CJSON_STACK_SIZE (256)
#endif

typedef struct
{
  const char *json;

  char *stack;
  size_t top, size;
}cjson_context;

#define IS0TO9(ch) ((ch) >= '0' && (ch) <= '9')
#define IS1TO9(ch) ((ch) >= '1' && (ch) <= '9')

#define PUSH_CHAR_TO_STACK(c, ch) do{ *(char *)cjson_push(c, sizeof(char)) = ch; }while(0)

static void* cjson_push(cjson_context *c, size_t len)
{
  void *ret;
  assert(len > 0);

  if(c->top + len > c->size)  //调整栈空间
  {
    if(c->size == 0)
      c->size = CJSON_STACK_SIZE;

    while(c->top + len > c->size)   //循环保证扩容一次空间还不够的情况
      c->size += c->size >> 1;

    c->stack = realloc(c->stack, c->size);
  }
  
  ret = c->stack + c->top;
  c->top += len;

  return ret;
}

static void* cjson_pop(cjson_context *c, size_t len)
{
  void *ret;
  assert(len >= 0);

  ret = c->stack + (c->top -= len);
  
  return ret;
}

static void cjson_value_init(cjson_value *value)
{
  assert(value != NULL);

  value->type = CJSON_NULL;
  value->u.num = 0;
  value->u.str.l = 0;
  free(value->u.str.buf);
}

static void cjson_parse_skip_space(cjson_context *c)
{
  const char *p = c->json;

  while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    p++;
  c->json = p;
}

static CJSON_STATUS cjson_parse_literal(cjson_context *c, cjson_value *v, const char* expect, cjson_type type)   //解析文字
{
  char i = 0;
  const char *p = c->json;

  // for(i = 0; expect[i+1]; i++)   //这里for这么写的化，最后一个字符不比较，有bug
  // {
  //   if(*p++ != expect[i])
  //     return CJSON_ERR_LITERAL;
  // }

  do
  {
    if(*p++ != expect[i])
      return CJSON_ERR_LITERAL;
  }while(expect[++i]);
  

  v->type = type;
  v->u.num = 0;
  
  c->json = p;

  return CJSON_OK;
}

static CJSON_STATUS cjson_parse_number(cjson_context *c, cjson_value *v)
{
  const char *p = c->json;

  if(*p == '-')
    p++;

  if(*p == '0')
    p++;
  else
  {
    if(!IS0TO9(*p))
      return CJSON_ERR_NUMBER;
    // while(IS0TO9(*p++));         //bug

    char ch = *p;
    while(IS0TO9(ch))
      ch = *p++;
  }

  if(*p == '.')
  {
    p++;
    if(!IS0TO9(*p))
      return CJSON_ERR_NUMBER;
    while(IS0TO9(*p++));
  }

  if(*p == 'e' || *p == 'E')
  {
    p++;
    if (*p == '+' || *p == '-')
      p++;
    if(!IS0TO9(*p))
      return CJSON_ERR_NUMBER;
    while(IS0TO9(*p++));
  }

  v->u.num = strtod(c->json, NULL);
  v->type = CJSON_NUMBER;

  c->json = p;

  return CJSON_OK;
}


static CJSON_STATUS cjson_parse_4hex(const char *p, uint16_t *hex)
{
  assert(p != NULL && (p+1) != NULL && (p+2) != NULL && (p+3) != NULL);
  assert(hex != NULL);

  *hex = 0;
  for(int i = 0; i < 4; i++)
  {
    char ch = *p++;
    *hex <<= 4;
    if(IS0TO9(ch))
      *hex |= ch - '0';
    else if(ch >= 'a' && ch <= 'f')
      *hex |= ch - 'a' + 10;
    else if(ch >= 'A' && ch <= 'F')
      *hex |= ch - 'A' + 10;
    else
      return CJSON_ERR_UNICODE_HEX;
  }

  return CJSON_OK;
}

static void cjson_parse_utf8(cjson_context *c, uint32_t codepoint)
{
  if(codepoint <= 0x7f)
    PUSH_CHAR_TO_STACK(c, codepoint & 0xff);
  else if(codepoint <= 0x7ff)
  {
    PUSH_CHAR_TO_STACK(c, 0xc0 | ((codepoint >> 6) & 0x1f));    // 110x xxxx
    PUSH_CHAR_TO_STACK(c, 0x80 | ( codepoint       & 0x3f));    // 10xx xxxx
  }
  else if(codepoint <= 0xffff)
  {
    PUSH_CHAR_TO_STACK(c, 0xe0 | ((codepoint >> 12) & 0x1f));    // 1110 xxxx
    PUSH_CHAR_TO_STACK(c, 0x80 | ((codepoint >> 6)  & 0x3f));    // 10xx xxxx
    PUSH_CHAR_TO_STACK(c, 0x80 | ( codepoint        & 0x3f));    // 10xx xxxx
  }
  else
  {
    assert(codepoint <= 0x10ffff);
    PUSH_CHAR_TO_STACK(c, 0xf0 | ((codepoint >> 18) & 0x07));    // 1111 0xxx
    PUSH_CHAR_TO_STACK(c, 0x80 | ((codepoint >> 12) & 0x3f));    // 10xx xxxx
    PUSH_CHAR_TO_STACK(c, 0x80 | ((codepoint >> 6)  & 0x3f));    // 10xx xxxx
    PUSH_CHAR_TO_STACK(c, 0x80 | ( codepoint        & 0x3f));    // 10xx xxxx

  }
}

static CJSON_STATUS cjson_parse_string(cjson_context *c, cjson_value *v)
{
  size_t len, head = c->top;
  const char *p = c->json;
  char ch = 0;
  uint16_t hex = 0;   //utf8高代理项、BMP平面内码点
  uint32_t codepoint = 0; //utf8码点

  if(*p == '\"')
    p++;
  else
    return CJSON_ERR_STRING_MISS_QUOTATION_MARK;

  while(1)
  {
    ch = *p++;
    switch (ch) //规则检查
    {
      case '\\':  //转义字符
        // p++;
        switch (*p++)
        {
          case 'b':  PUSH_CHAR_TO_STACK(c, '\b'); break;
          case 'f':  PUSH_CHAR_TO_STACK(c, '\f'); break;
          case 'r':  PUSH_CHAR_TO_STACK(c, '\r'); break;
          case 'n':  PUSH_CHAR_TO_STACK(c, '\n'); break;
          case 't':  PUSH_CHAR_TO_STACK(c, '\t'); break;
          case '\\': PUSH_CHAR_TO_STACK(c, '\\'); break;
          case '\"': PUSH_CHAR_TO_STACK(c, '\"'); break;
          case '/':  PUSH_CHAR_TO_STACK(c, '/');  break;
          case 'u': 
            if(cjson_parse_4hex(p, &hex) != CJSON_OK)
              return CJSON_ERR_UNICODE_HEX;
            p += 4;
            codepoint = hex;

            if(hex >= 0xd800 && hex <= 0xdbff)  //hex为高代理项
            {
              uint16_t hex2 = 0;
              if(*p++ != '\\')
                return CJSON_ERR_UNICODE_SURROGATE;
              if(*p++ != 'u')
                return CJSON_ERR_UNICODE_SURROGATE;

              if(cjson_parse_4hex(p, &hex2) != CJSON_OK)  //hex2 为低代理项
                return CJSON_ERR_UNICODE_HEX;
              p += 4;

              codepoint = 0x10000 + (hex - 0xd800) * 0x400 + (hex2 - 0xdc00);
            }
            cjson_parse_utf8(c, codepoint);
            break;    //utf-8
          default:
            return CJSON_ERR_STRING_INVALID_ESCAPE_CAHR;
        }
        break;

      case '\0':  //字符串中不能有结束符
        return CJSON_ERR_STRING_MISS_QUOTATION_MARK;

      default:    //普通合法字符
        PUSH_CHAR_TO_STACK(c, ch);
        break;

      case '\"':  //字符串结束引号
        len = c->top - head;
        cjson_set_string(v, (const char *)cjson_pop(c, len), len);
        c->json = ++p;
        return CJSON_OK;
        break;
    }
  }
}

static CJSON_STATUS cjson_parse_value(cjson_context *c, cjson_value *v);

static CJSON_STATUS cjson_parse_array(cjson_context *c, cjson_value *v)
{
  const char * p = c->json;
  cjson_value value = {0};
  v->type = CJSON_ARRAY;

  c->json++;  //跳过 [

  while(*c->json != ']')
  {
    cjson_parse_skip_space(c);

    if(cjson_parse_value(c, &value) == CJSON_OK)    //bug
    {
      v->u.arr.size++;
      v->u.arr.elements = (cjson_value *)realloc(v->u.arr.elements, v->u.arr.size * sizeof(cjson_value));
      memcpy(v->u.arr.elements + v->u.arr.size - 1, &value, sizeof(cjson_value));
    }

    cjson_parse_skip_space(c);

    if(*c->json != ',' && *c->json != ']')
      return CJSON_ERR_ARRAY;
    
    if(*c->json == ',')
      c->json++;
  }
  return CJSON_OK;
}

static CJSON_STATUS cjson_parse_value(cjson_context *c, cjson_value *v)
{
  CJSON_STATUS ret;
  switch (*(c->json))
  {
    case 'n':  ret = cjson_parse_literal(c, v, "null", CJSON_NULL);   break;
    case 't':  ret = cjson_parse_literal(c, v, "true", CJSON_TRUE);   break;
    case 'f':  ret = cjson_parse_literal(c, v, "false", CJSON_FALSE); break;
    case '\"': ret = cjson_parse_string(c, v); break;
    case '[':  ret = cjson_parse_array(c, v); break;
    default:   ret = cjson_parse_number(c, v); break;
  }

  return ret;
}

//--------------------------API--------------------------//

CJSON_STATUS cjson_parse(cjson_value* v, const char *json)
{
  CJSON_STATUS ret;
  cjson_context c = {0};
  c.json = json;

  cjson_value_init(v);
  cjson_parse_skip_space(&c);

  return cjson_parse_value(&c, v);
}

cjson_type cjson_get_type(cjson_value value)
{
  return value.type;
}

int cjson_get_boolean(cjson_value value)
{
  return value.type == CJSON_TRUE;
}

void cjson_set_boolean(cjson_value *value, int bool)
{
  cjson_value_init(value);
  value->type = bool ? CJSON_TRUE : CJSON_FALSE;
}

double cjson_get_number(cjson_value value)
{
  return value.u.num;
}

void cjson_set_number(cjson_value *value, double num)
{
  cjson_value_init(value);
  value->type = CJSON_NUMBER;
  value->u.num = num;
}

size_t cjson_get_string_length(cjson_value value)
{
  return value.u.str.l;
}

char* cjson_get_string(cjson_value value)
{
  return value.u.str.buf;
}

void cjson_set_string(cjson_value *value, const char *buf, size_t len)
{
  assert(buf != NULL || len == 0);
  cjson_value_init(value);
  value->type = CJSON_STRING;
  value->u.str.buf = malloc(len + 1);

  memcpy(value->u.str.buf, buf, len);
  value->u.str.buf[len] = '\0';
  value->u.str.l = len;
}

size_t cjson_get_array_size(cjson_value value)
{
  return value.u.arr.size;
}

cjson_value *cjson_get_array_element(cjson_value value, size_t index)
{
  return value.u.arr.elements + index;
}
