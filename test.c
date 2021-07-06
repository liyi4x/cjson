#include <stdio.h>
#include <stdlib.h>

#include "cjson.h"

int test_count = 0, test_count_pass = 0;

#define TEST_BASE(equation, expect, actuall, format) \
do{\
    test_count++;\
    if(equation)\
        test_count_pass++;\
    else\
        fprintf(stderr, "[test fail] @ %s:%d: expect: " format " actuall: " format "\n", __FILE__, __LINE__, expect, actuall);\
}while(0)


#define TEST_INT(expect, actuall) TEST_BASE((expect) == (actuall), expect, actuall, "%d")
#define TEST_DOUBLE(expect, actuall) TEST_BASE((expect) == (actuall), expect, actuall, "%.17g")

// #define TEST_LITERAL(equation, expect, actuall) TEST_BASE(equation, expect, actuall, "%s")




void test_null()
{
    cjson_value v;

    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, " null"));
    TEST_INT(CJSON_NULL, cjson_get_type(v));
    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \rnull"));
    TEST_INT(CJSON_NULL, cjson_get_type(v));
    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \r\nnull"));
    TEST_INT(CJSON_NULL, cjson_get_type(v));
    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \r\n\tnull"));
    TEST_INT(CJSON_NULL, cjson_get_type(v));
}

void test_true()
{
    cjson_value v;

    v.type = CJSON_NULL;
    TEST_INT(CJSON_OK, cjson_parse(&v, " true"));
    TEST_INT(CJSON_TRUE, cjson_get_type(v));
    v.type = CJSON_NULL;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \rtrue"));
    TEST_INT(CJSON_TRUE, cjson_get_type(v));
    v.type = CJSON_NULL;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \r\ntrue"));
    TEST_INT(CJSON_TRUE, cjson_get_type(v));
    v.type = CJSON_NULL;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \r\n\ttrue"));
    TEST_INT(CJSON_TRUE, cjson_get_type(v));
}

void test_false()
{
    cjson_value v;

    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, " false"));
    TEST_INT(CJSON_FALSE, cjson_get_type(v));
    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \r false"));
    TEST_INT(CJSON_FALSE, cjson_get_type(v));
    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \r\n false"));
    TEST_INT(CJSON_FALSE, cjson_get_type(v));
    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, " \r\n\t false"));
    TEST_INT(CJSON_FALSE, cjson_get_type(v));
}

#define TEST_JSON_NUMBER(except, json) \
    do{\
        cjson_value v = {0};\
        TEST_INT(CJSON_OK, cjson_parse(&v, (json)));\
        TEST_INT(CJSON_NUMBER, cjson_get_type(v));\
        TEST_DOUBLE(except, cjson_get_number(v));\
    }while(0)

void test_number()
{
    TEST_JSON_NUMBER(0.0, "0");
    TEST_JSON_NUMBER(0.0, "-0");
    TEST_JSON_NUMBER(0.0, "-0.0");
    TEST_JSON_NUMBER(1.0, "1");
    TEST_JSON_NUMBER(-1.0, "-1");
    TEST_JSON_NUMBER(1.5, "1.5");
    TEST_JSON_NUMBER(-1.5, "-1.5");
    TEST_JSON_NUMBER(3.1416, "3.1416");
    TEST_JSON_NUMBER(1E10, "1E10");
    TEST_JSON_NUMBER(1e10, "1e10");
    TEST_JSON_NUMBER(1E+10, "1E+10");
    TEST_JSON_NUMBER(1E-10, "1E-10");
    TEST_JSON_NUMBER(-1E10, "-1E10");
    TEST_JSON_NUMBER(-1e10, "-1e10");
    TEST_JSON_NUMBER(-1E+10, "-1E+10");
    TEST_JSON_NUMBER(-1E-10, "-1E-10");
    TEST_JSON_NUMBER(1.234E+10, "1.234E+10");
    TEST_JSON_NUMBER(1.234E-10, "1.234E-10");
    TEST_JSON_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_JSON_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_JSON_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_JSON_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_JSON_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_JSON_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_JSON_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_JSON_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_JSON_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_JSON_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");

}

void main()
{
    test_null();
    test_true();
    test_false();
    test_number();
    

    printf("\n===================== result =====================\n");
    printf("  test all %d, pass: %d\n", test_count, test_count_pass);
    printf("==================================================\n");

}