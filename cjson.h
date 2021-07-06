#ifndef _CJSON_H_
#define _CJSON_H_

#include <stdint.h>

typedef enum{
    CJSON_OK,
    CJSON_ERR_LITERAL,
    CJSON_ERR_NUMBER,
    CJSON_ERR_STRING
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
        struct {
            char *p;
            uint16_t l;
        }str;
    }u;
}cjson_value;



CJSON_STATUS cjson_parse(cjson_value *v, const char *json);
cjson_type cjson_get_type(cjson_value value);
double cjson_get_number(cjson_value value);

#endif