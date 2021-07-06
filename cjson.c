#include "cjson.h"
#include <assert.h>
#include <stdlib.h>

typedef struct
{
    const char *json;
}cjson_context;

#define IS0TO9(ch) ((ch) >= '0' && (ch) <= '9')
#define IS1TO9(ch) ((ch) >= '1' && (ch) <= '9')


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
    const char *p = c->json;
    if(*p == '\"')
        p++;
    else
        return CJSON_ERR_STRING;
    while(*p)
    {
        switch (*p) //规则检查
        {
        case '\\':
            switch (*p)
            {
            case 'n':
                
                break;
            
            default:
                break;
            }
            break;
        
        default:
            break;
        }
    }
    
}


CJSON_STATUS cjson_parse(cjson_value* v, const char *json)
{
    CJSON_STATUS ret;
    cjson_context c;
    c.json = json;
    cjson_parse_skip_space(&c);

    switch (*c.json)
    {
    case 'n':
        ret = cjson_parse_literal(&c, v, "null", CJSON_NULL);
        break;
    case 't':
        ret = cjson_parse_literal(&c, v, "true", CJSON_TRUE);
        break;
    case 'f':
        ret = cjson_parse_literal(&c, v, "false", CJSON_FALSE);
        break;
    
    default:
        ret = cjson_parse_number(&c, v);
        break;
    }


    return ret;
}

cjson_type cjson_get_type(cjson_value value)
{
    return value.type;
}

double cjson_get_number(cjson_value value)
{
    return value.u.num;
}


