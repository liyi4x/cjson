#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define TEST_STRING(expect, actuall, length)\
    TEST_BASE((sizeof(expect)  == length) && !memcmp(expect, actuall, length), expect, actuall, "%s")


#define TEST_JSON_LITERAL(json_type, json) \
do{\
    cjson_value v;\
    v.type = CJSON_OBJECT;\
    TEST_INT(CJSON_OK, cjson_parse(&v, json));\
    TEST_INT(json_type, cjson_get_type(v));\
}while(0)

void test_literal()
{
    TEST_JSON_LITERAL(CJSON_NULL, " null");
    TEST_JSON_LITERAL(CJSON_NULL, " \r\n\t null");

    TEST_JSON_LITERAL(CJSON_TRUE, " true");
    TEST_JSON_LITERAL(CJSON_TRUE, " \r\n\t true");
    
    TEST_JSON_LITERAL(CJSON_FALSE, " false");
    TEST_JSON_LITERAL(CJSON_FALSE, " \r\n\t false");
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

#define TEST_JSON_STRING(except, json) \
    do{\
        cjson_value v = {0};\
        TEST_INT(CJSON_OK, cjson_parse(&v, (json)));\
        TEST_INT(CJSON_STRING, cjson_get_type(v));\
        TEST_STRING(except, cjson_get_string(v), sizeof(except));\
    }while(0)

void test_string()
{
    
    TEST_JSON_STRING("", "\"\"");
    TEST_JSON_STRING("Hello", "\"Hello\"");
    TEST_JSON_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_JSON_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
}

void main()
{
    test_literal();
    test_number();
    test_string();

    printf("\n===================== result =====================\n");
    printf("  test all %d, pass: %d\n", test_count, test_count_pass);
    printf("==================================================\n\n");

}