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

static void cjson_parse_skip_space(cjson_context *c)    //指针的指针才能修改指针的指向
{
    const char *p = c->json;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static CJSON_STATUS cjson_parse_literal(cjson_context *c, cjson_value *v, const char* expect, cjson_type type)   //解析文字
{
    char i;
    const char *p = c->json;
    for(i = 0; expect[i+1]; i++)
    {
        if(p[i] != expect[i])
            return CJSON_ERR_LITERAL;
    }
    v->type = type;
    v->u.num = 0;
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
        while(IS0TO9(*p++));
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
    return CJSON_OK;
}


static CJSON_STATUS cjson_parse_string(cjson_context *c, cjson_value *v)
{
    size_t len, head = c->top;
    const char *p = c->json;
        int i = 0;

    if(*p == '\"')
        p++;
    else
        return CJSON_ERR_STRING;

    while(*p)
    {
        switch (*p) //规则检查
        {
        case '\"':
            len = c->top - head;

            cjson_set_string(v, (const char *)cjson_pop(c, len), len);
            c->json = ++p;
            return CJSON_OK;
            break;
        case '\\':  //转义字符
            p++;
            switch (*p)
            {
            case 'b': PUSH_CHAR_TO_STACK(c, '\b'); break;
            case 'f': PUSH_CHAR_TO_STACK(c, '\f'); break;
            case 'r': PUSH_CHAR_TO_STACK(c, '\r'); break;
            case 'n': PUSH_CHAR_TO_STACK(c, '\n'); break;
            case 't': PUSH_CHAR_TO_STACK(c, '\t'); break;
            case '\\': PUSH_CHAR_TO_STACK(c, '\\'); break;
            case '\"': PUSH_CHAR_TO_STACK(c, '\"'); break;
            case '/': PUSH_CHAR_TO_STACK(c, '/'); break;
            default:
                return CJSON_ERR_STRING;//转义字符错误
            }
            break;
        case '\0':  //字符串中不能有结束符
            return CJSON_ERR_STRING;

        default:

            PUSH_CHAR_TO_STACK(c, *p);
            break;
        }

        p++;

    }
    
}


CJSON_STATUS cjson_parse(cjson_value* v, const char *json)
{
    CJSON_STATUS ret;
    cjson_context c = {0};
    c.json = json;

    cjson_value_init(v);
    cjson_parse_skip_space(&c);

    switch (*c.json)
    {
        case 'n': ret = cjson_parse_literal(&c, v, "null", CJSON_NULL); break;
        case 't': ret = cjson_parse_literal(&c, v, "true", CJSON_TRUE); break;
        case 'f': ret = cjson_parse_literal(&c, v, "false", CJSON_FALSE); break;
        case '\"': ret = cjson_parse_string(&c, v); break;
        default: ret = cjson_parse_number(&c, v); break;
    }
    
    return ret;
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
