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

  // for(i = 0; expect[i+1]; i++)   //这里for这么写的话，最后一个字符不比较，有bug
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

  c->json = p;

  return CJSON_OK;
}

static CJSON_STATUS cjson_parse_number(cjson_context *c, cjson_value *v)
{
  const char *p = c->json;
  // char ch = *p;

  if(*p == '-')
    p++;
    // ch = *(++p);  //指针运算，*运算优先级低于 p++，因此先自增，再解指针，*p++先取p值，解指针，p再自增
                    //https://blog.csdn.net/weixin_41413441/article/details/80849827

  if(*p == '0')
    p++;
  else
  {
    if(!IS0TO9(*p))
      return CJSON_ERR_NUMBER;
    // while(IS0TO9(*p++));       //bug //这里IS0TO9()宏中如果有自增的话会自增两次，宏定义的原因
    for(p++; IS0TO9(*p); p++);  //第一个表达式的作用是跳过当前字符，因为在上面if语句中已经判断一次了
  }

  if(*p == '.')
  {
    p++;
    if(!IS0TO9(*p))
      return CJSON_ERR_NUMBER;
    for(p++; IS0TO9(*p); p++);
  }

  if(*p == 'e' || *p == 'E')
  {
    p++;
    if (*p == '+' || *p == '-')
      p++;
    if(!IS0TO9(*p))
      return CJSON_ERR_NUMBER;
    for(p++; IS0TO9(*p); p++);
  }

  v->u.num = strtod(c->json, NULL);
  v->type = CJSON_NUMBER;

  c->json = p;

  return CJSON_OK;
}


static CJSON_STATUS cjson_parse_4hex(const char *p, uint16_t *hex)
{
  assert(p != NULL);
  assert(hex != NULL);

  *hex = 0;
  for(char i = 0; i < 4; i++)
  {
    *hex <<= 4;   //这个必须放在前面，如果放在if后边，最后一次填满16位，又移位，会丢数据
    if(IS0TO9(*p))
      *hex |= *p - '0';
    else if(*p >= 'a' && *p <= 'f')
      *hex |= *p - 'a' + 10;
    else if(*p >= 'A' && *p <= 'F')
      *hex |= *p - 'A' + 10;
    else
      return CJSON_ERR_UNICODE_HEX;
    p++;
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


static CJSON_STATUS cjson_parse_string_raw(cjson_context *c, const char **s, size_t *l) //解析出来的字符串长度不包括c语言规定的结尾字节 '\0'
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
        // cjson_set_string(v, (const char *)cjson_pop(c, len), len);
        *s = (const char *)cjson_pop(c, len);
        *l = len;
        // c->json = ++p;
        c->json = p++;
        return CJSON_OK;
        break;
    }
  }
}

static CJSON_STATUS cjson_parse_string(cjson_context *c, cjson_value *v)
{
  const char *s;
  size_t len;
  CJSON_STATUS ret;

  if((ret = cjson_parse_string_raw(c, &s, &len)) == CJSON_OK)
    cjson_set_string(v, s, len);
  return ret;
}

static CJSON_STATUS cjson_parse_value(cjson_context *c, cjson_value *v);
static CJSON_STATUS cjson_parse_array(cjson_context *c, cjson_value *v)
{
  cjson_value value = {0};
  CJSON_STATUS ret;
  ssize_t size = 0;

  c->json++;  //跳过 '['

  cjson_parse_skip_space(c);
  if(*c->json == ']')
  {
    c->json++;
    v->type = CJSON_ARRAY;
    v->u.arr.elements = NULL;
    v->u.arr.size = 0;
    return CJSON_OK;
  }

  while(1)
  {
    if((ret = cjson_parse_value(c, &value)) != CJSON_OK)  // = 的优先级低于 !=
      break;

    memcpy(cjson_push(c, sizeof(cjson_value)), &value, sizeof(cjson_value));  //压栈
    size++;   //元素个数，并非字节数

    cjson_parse_skip_space(c);

    if(*c->json == ',')   //如果逗号之后没有字符了会在下一次循环cjson_parse_value()中报错
      c->json++;
    else if(*c->json == ']')
    {
      c->json++;
      v->type = CJSON_ARRAY;
      v->u.arr.size = size;

      size *= sizeof(cjson_value);
      memcpy(v->u.arr.elements = (cjson_value *)malloc(size), cjson_pop(c, size), size);  //压栈，出栈的长度单位都是字节
      return CJSON_OK;
    }
    else
    {
      ret = CJSON_ERR_ARRAY_NEED_COMMA_OR_SQUARE_BRACKET;
      break;
    }
  }

  for(size_t i = 0; i < size; i++)
  {
    cjson_value_init((cjson_value *)cjson_pop(c, sizeof(cjson_value)));
  }

  return ret;
}

static CJSON_STATUS cjson_parse_object(cjson_context *c, cjson_value *v)
{
  CJSON_STATUS ret;
  size_t size = 0;
  cjson_member member;

  c->json++;  //跳过 '{'

  cjson_parse_skip_space(c);
  if(*c->json == '}')
  {
    c->json++;
    v->type = CJSON_OBJECT;
    v->u.obj.members = NULL;
    v->u.obj.size = 0;
    return CJSON_OK;
  }

  while(1)
  {
    //key
    if(*c->json != '\"')
    {
      ret = CJSON_ERR_OBJECT_NEED_KEY;
      break;
    }
    const char *str;
    if((ret = cjson_parse_string_raw(c, &(str), &(member.key_len))) != CJSON_OK)    //这里先解析字符串，成功之后再申请内存放到member变量中
      break;
    memcpy(member.key = (char *)malloc(member.key_len + 1), str, member.key_len);
    member.key[member.key_len] = '\0';

    //:
    cjson_parse_skip_space(c);
    if(*c->json == ':')
      c->json++;
    else
    {
      ret = CJSON_ERR_OBJECT_NEED_COLON;
      break;
    }

    if((ret = cjson_parse_value(c, &member.value) != CJSON_OK))
      return ret;

    memcpy((cjson_member *)cjson_push(c, sizeof(cjson_member)), &member, sizeof(cjson_member));
    size++;

    cjson_parse_skip_space(c);

    if(*c->json == ',')
    {
      c->json++;
      cjson_parse_skip_space(c);
    }
    else if(*c->json == '}')
    {
      v->type = CJSON_OBJECT;
      v->u.obj.size = size;

      size *= sizeof(cjson_member);

      memcpy(v->u.obj.members = (cjson_member *)malloc(size), cjson_pop(c, size), size);
      return CJSON_OK;
    }
  }

  free(member.key);
  for(size_t i = 0; i < size; i++)
  {
    cjson_member *m;
    m = (cjson_member *)cjson_pop(c, sizeof(cjson_member));
    free(m->key);
    cjson_value_init(&m->value);
  }

  return ret;
}

static CJSON_STATUS cjson_parse_value(cjson_context *c, cjson_value *v)
{
  CJSON_STATUS ret;

  cjson_parse_skip_space(c);
  switch (*(c->json))
  {
    case 'n':  ret = cjson_parse_literal(c, v, "null", CJSON_NULL);   break;
    case 't':  ret = cjson_parse_literal(c, v, "true", CJSON_TRUE);   break;
    case 'f':  ret = cjson_parse_literal(c, v, "false", CJSON_FALSE); break;
    case '\"': ret = cjson_parse_string(c, v); break;
    case '[':  ret = cjson_parse_array(c, v); break;
    case '{':  ret = cjson_parse_object(c, v); break;
    default:   ret = cjson_parse_number(c, v); break;
  }

  return ret;
}

static void cjson_stringify_string(cjson_context *c, const char *str, size_t len)
{
  static const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
  char *p, *head;

  p = head = cjson_push(c, len * 6 + 2);    //按最坏的情况计算，全部转化乘 \uxxxx 形式，再加两个引号

  *p++ = '\"';

  for(size_t i = 0; i < len; i++)
  {
    switch(str[i])
    {
      case '\b': *p++ = '\\'; *p++ = 'b'; break;
      case '\f': *p++ = '\\'; *p++ = 'f'; break;
      case '\r': *p++ = '\\'; *p++ = 'r'; break;
      case '\n': *p++ = '\\'; *p++ = 'n'; break;
      case '\t': *p++ = '\\'; *p++ = 't'; break;
      case '\\': *p++ = '\\'; *p++ = '\\'; break;
      case '\"': *p++ = '\\'; *p++ = '\"'; break;
      default :
        if((uint8_t)str[i] < 0x20)
        {
          *p++ = '\\';  *p++ = 'u';  *p++ = '0';  *p++ = '0';
          *p++ = hex[(str[i] >> 4) & 0x0f];
          *p++ = hex[(str[i]) & 0x0f];
        }
        else
        {
          *p++ = str[i];
        }
        break;
    }
  }
  *p++ = '\"';
  c->top -=  len * 6 + 2 - (p - head);
}

#define CJSON_STRINGIFY_PUT_CHAR(c, ch) do{*(char *)cjson_push(c, sizeof(char)) = (ch);}while(0)
static void cjson_stringify_value(cjson_context *c, const cjson_value *v)
{
  char *s;
  size_t len;

  switch(v->type)
  {
    case CJSON_NULL: memcpy(cjson_push(c, 4), "null", 4); break;
    case CJSON_TRUE: memcpy(cjson_push(c, 4), "true", 4); break;
    case CJSON_FALSE: memcpy(cjson_push(c, 5), "false", 5); break;
    case CJSON_NUMBER: c->top -= 32 - sprintf(cjson_push(c, 32), "%.17g", v->u.num); break;
    case CJSON_STRING: cjson_stringify_string(c, v->u.str.buf, v->u.str.l); break;
    case CJSON_ARRAY:
      CJSON_STRINGIFY_PUT_CHAR(c, '[');
      for(size_t i = 0; i < v->u.arr.size; i++)
      {
        if(i != 0)
          CJSON_STRINGIFY_PUT_CHAR(c, ',');
        cjson_stringify_value(c, (const cjson_value *)v->u.arr.elements + i);   //数组中的第i个元素
      }
      CJSON_STRINGIFY_PUT_CHAR(c, ']');
      break;
    case CJSON_OBJECT:
      CJSON_STRINGIFY_PUT_CHAR(c, '{');
      for(size_t i = 0; i < v->u.obj.size; i++)
      {
        if(i != 0)
          CJSON_STRINGIFY_PUT_CHAR(c, ',');
        cjson_stringify_string(c, v->u.obj.members[i].key, v->u.obj.members[i].key_len);
        CJSON_STRINGIFY_PUT_CHAR(c, ':');
        cjson_stringify_value(c, &v->u.obj.members[i].value);   //对象中的第i个key-value的 value
      }
      CJSON_STRINGIFY_PUT_CHAR(c, '}');
      break;
  }
}

//--------------------------API--------------------------//

char *cjson_stringify(cjson_value v, size_t *length)
{
  cjson_context c = {0};

  cjson_stringify_value(&c, &v);

  if(length)
    *length = c.top;

  *(char*)cjson_push(&c, sizeof(char)) = ('\0');

  return c.stack;
}

CJSON_STATUS cjson_parse(cjson_value* v, const char *json)
{
  CJSON_STATUS ret;
  cjson_context c = {0};
  c.json = json;

  cjson_value_init(v);

  return cjson_parse_value(&c, v);
}

void cjson_value_init(cjson_value *value)
{
  assert(value != NULL);

  switch(value->type)
  {
    case CJSON_STRING:
      value->u.str.l = 0;
      free(value->u.str.buf);
      break;
    case CJSON_ARRAY:
      for(size_t i = 0; i < value->u.arr.size; i++)
      {
        cjson_value_init(&(value->u.arr.elements[i]));
      }
      value->u.arr.size = 0;
      free(value->u.arr.elements);
      break;
    case CJSON_NUMBER:
      value->u.num = 0;
      break;
    case CJSON_OBJECT:
      for(size_t i = 0; i < value->u.obj.size; i++)
      {
        free(value->u.obj.members[i].key);
        value->u.obj.members[i].key_len = 0;
        cjson_value_init(&value->u.obj.members[i].value);
      }

      value->u.obj.size = 0;
      break;
  }

  value->type = CJSON_NULL;
}

cjson_type cjson_get_type(cjson_value value)
{
  return value.type;
}

int cjson_get_boolean(cjson_value value)
{
  assert(value.type == CJSON_TRUE || value.type == CJSON_FALSE);
  return value.type == CJSON_TRUE;
}

void cjson_set_boolean(cjson_value *value, int bool)
{
  assert(value != NULL);
  cjson_value_init(value);
  value->type = bool ? CJSON_TRUE : CJSON_FALSE;
}

double cjson_get_number(cjson_value value)
{
  assert(value.type == CJSON_NUMBER);
  return value.u.num;
}

void cjson_set_number(cjson_value *value, double num)
{
  assert(value != NULL);
  cjson_value_init(value);
  value->type = CJSON_NUMBER;
  value->u.num = num;
}

size_t cjson_get_string_length(cjson_value value)
{
  assert(value.type == CJSON_STRING);
  return value.u.str.l;
}

char* cjson_get_string(cjson_value value)
{
  assert(value.type == CJSON_STRING);
  return value.u.str.buf;
}

void cjson_set_string(cjson_value *value, const char *buf, size_t len)
{
  assert(value != NULL);
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
  assert(value.type == CJSON_ARRAY);
  return value.u.arr.size;
}

cjson_value *cjson_get_array_element(cjson_value value, size_t index)
{
  assert(value.type == CJSON_ARRAY);
  assert(value.u.arr.size > index);  //index为索引号，从0开始，size为元素数，从1开始
  return value.u.arr.elements + index;
}

size_t cjson_get_object_size(cjson_value value)
{
  assert(value.type == CJSON_OBJECT);
  return value.u.obj.size;
}

const char *cjson_get_object_key(cjson_value value, size_t index)
{
  assert(value.type == CJSON_OBJECT);
  assert(value.u.obj.size > index);  //index为索引号，从0开始，size为元素数，从1开始
  return (value.u.obj.members + index)->key;
}

size_t cjson_get_object_key_length(cjson_value value, size_t index)
{
  assert(value.type == CJSON_OBJECT);
  assert(value.u.obj.size > index);  //index为索引号，从0开始，size为元素数，从1开始
  return (value.u.obj.members + index)->key_len;
}

cjson_value *cjson_get_object_value(cjson_value value, size_t index)
{
  assert(value.type == CJSON_OBJECT);
  assert(value.u.obj.size > index);  //index为索引号，从0开始，size为元素数，从1开始
  return &((value.u.obj.members + index)->value);
}
