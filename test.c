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
    fprintf(stderr, "[test fail] @ %s:%d: expect: " format " actuall: " format "\n" \
                  , __FILE__, __LINE__, expect, actuall);\
}while(0)


#define TEST_INT(expect, actuall) TEST_BASE((expect) == (actuall), expect, actuall, "%d")
#define TEST_DOUBLE(expect, actuall) TEST_BASE((expect) == (actuall), expect, actuall, "%.17g")
#define TEST_STRING(expect, actuall, length)\
  TEST_BASE((sizeof(expect) == (length + 1)) && !memcmp(expect, actuall, (length + 1)), expect, actuall, "%s")
#define TEST_SIZE_T(expect, actuall) TEST_BASE((expect) == (actuall), (size_t)expect, (size_t)actuall, "%zu")


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
    TEST_STRING(except, cjson_get_string(v), cjson_get_string_length(v));\
  }while(0)

void test_string()
{
  
  TEST_JSON_STRING("", "\"\"");
  TEST_JSON_STRING("Hello", "\"Hello\"");
  TEST_JSON_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
  TEST_JSON_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");

  // utf-8
  TEST_JSON_STRING("Hello\0World", "\"Hello\\u0000World\"");
  TEST_JSON_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
  TEST_JSON_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
  TEST_JSON_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
  TEST_JSON_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
  TEST_JSON_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

void test_array()
{
    cjson_value v = {0};

    TEST_INT(CJSON_OK, cjson_parse(&v, "[ ]"));
    TEST_INT(CJSON_ARRAY, cjson_get_type(v));
    TEST_SIZE_T(0, cjson_get_array_size(v));

    v.type = CJSON_TRUE;
    TEST_INT(CJSON_OK, cjson_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    TEST_INT(CJSON_ARRAY, cjson_get_type(v));
    TEST_SIZE_T(5, cjson_get_array_size(v));
    TEST_INT(CJSON_NULL,   cjson_get_type(*cjson_get_array_element(v, 0)));
    TEST_INT(CJSON_FALSE,  cjson_get_type(*cjson_get_array_element(v, 1)));
    TEST_INT(CJSON_TRUE,   cjson_get_type(*cjson_get_array_element(v, 2)));
    TEST_INT(CJSON_NUMBER, cjson_get_type(*cjson_get_array_element(v, 3)));
    TEST_INT(CJSON_STRING, cjson_get_type(*cjson_get_array_element(v, 4)));
    TEST_DOUBLE(123.0, cjson_get_number(*cjson_get_array_element(v, 3)));
    TEST_STRING("abc", cjson_get_string(*cjson_get_array_element(v, 4)), cjson_get_string_length(*cjson_get_array_element(v, 4)));


    v.type = CJSON_NULL;
    TEST_INT(CJSON_OK, cjson_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    TEST_INT(CJSON_ARRAY, cjson_get_type(v));
    TEST_SIZE_T(4, cjson_get_array_size(v));
    for (char i = 0; i < 4; i++) {
        cjson_value* a = cjson_get_array_element(v, i);
        TEST_INT(CJSON_ARRAY, cjson_get_type(*a));
        TEST_SIZE_T(i, cjson_get_array_size(*a));
        for (char j = 0; j < i; j++) {
            cjson_value* e = cjson_get_array_element(*a, j);
            TEST_INT(CJSON_NUMBER, cjson_get_type(*e));
            TEST_DOUBLE((double)j, cjson_get_number(*e));
        }
    }
}

static void test_object() {
    cjson_value v;
    size_t i;

    cjson_value_init(&v);
    TEST_INT(CJSON_OK, cjson_parse(&v, " { } "));
    TEST_INT(CJSON_OBJECT, cjson_get_type(v));
    TEST_SIZE_T(0, cjson_get_object_size(v));
    

    cjson_value_init(&v);

    TEST_INT(CJSON_OK, cjson_parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    TEST_INT(CJSON_OBJECT, cjson_get_type(v));
    TEST_SIZE_T(7, cjson_get_object_size(v));
    TEST_STRING("n", cjson_get_object_key(v, 0), cjson_get_object_key_length(v, 0));
    TEST_INT(CJSON_NULL,   cjson_get_type(*cjson_get_object_value(v, 0)));
    TEST_STRING("f", cjson_get_object_key(v, 1), cjson_get_object_key_length(v, 1));
    TEST_INT(CJSON_FALSE,  cjson_get_type(*cjson_get_object_value(v, 1)));
    TEST_STRING("t", cjson_get_object_key(v, 2), cjson_get_object_key_length(v, 2));
    TEST_INT(CJSON_TRUE,   cjson_get_type(*cjson_get_object_value(v, 2)));
    TEST_STRING("i", cjson_get_object_key(v, 3), cjson_get_object_key_length(v, 3));
    TEST_INT(CJSON_NUMBER, cjson_get_type(*cjson_get_object_value(v, 3)));
    TEST_DOUBLE(123.0, cjson_get_number(*cjson_get_object_value(v, 3)));
    TEST_STRING("s", cjson_get_object_key(v, 4), cjson_get_object_key_length(v, 4));
    TEST_INT(CJSON_STRING, cjson_get_type(*cjson_get_object_value(v, 4)));
    TEST_STRING("abc", cjson_get_string(*cjson_get_object_value(v, 4)), cjson_get_string_length(*cjson_get_object_value(v, 4)));
    TEST_STRING("a", cjson_get_object_key(v, 5), cjson_get_object_key_length(v, 5));
    TEST_INT(CJSON_ARRAY, cjson_get_type(*cjson_get_object_value(v, 5)));
    TEST_SIZE_T(3, cjson_get_array_size(*cjson_get_object_value(v, 5)));
    for (i = 0; i < 3; i++) {
        cjson_value* e = cjson_get_array_element(*cjson_get_object_value(v, 5), i);
        TEST_INT(CJSON_NUMBER, cjson_get_type(*e));
        TEST_DOUBLE(i + 1.0, cjson_get_number(*e));
    }
    TEST_STRING("o", cjson_get_object_key(v, 6), cjson_get_object_key_length(v, 6));
    {
        cjson_value* o = cjson_get_object_value(v, 6);
        TEST_INT(CJSON_OBJECT, cjson_get_type(*o));
        for (i = 0; i < 3; i++) {
            cjson_value* ov = cjson_get_object_value(*o, i);
            // EXPECT_TRUE('1' + i == cjson_get_object_key(*o, i)[0]);
            TEST_SIZE_T(1, cjson_get_object_key_length(*o, i));
            TEST_INT(CJSON_NUMBER, cjson_get_type(*ov));
            TEST_DOUBLE(i + 1.0, cjson_get_number(*ov));
        }
    }
    cjson_value_init(&v);
}


#define TEST_ROUNDTRIP(json)\
    do {\
        cjson_value v;\
        char* json2;\
        size_t length;\
        cjson_value_init(&v);\
        TEST_INT(CJSON_OK, cjson_parse(&v, json));\
        json2 = cjson_stringify(v, &length);\
        TEST_STRING(json, json2, length);\
        cjson_value_init(&v);\
        free(json2);\
    } while(0)

static void test_stringify_number() {
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("3.25");
    TEST_ROUNDTRIP("1e+20");
    TEST_ROUNDTRIP("1.234e+20");
    TEST_ROUNDTRIP("1.234e-20");

    TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void test_stringify_array() {
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

static void test_stringify() {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    test_stringify_number();
    test_stringify_string();
    test_stringify_array();
    test_stringify_object();
}


void main()
{
  test_literal();
  test_number();
  test_string();
  test_array();
  test_object();

  test_stringify();

// cjson_value v; 
// char* json2; 
// size_t length; 
// cjson_value_init(&v); 
// TEST_INT(CJSON_OK, cjson_parse(&v, "[null,false,true,123,\"abc\",[1,2,3]]"));

// json2 = cjson_stringify(v, &length); 
// puts(json2);
// TEST_STRING("[null,false,true,123,\"abc\",[1,2,3]]", json2, length);

  

  printf("\n===================== result =====================\n");
  printf("  test all %d, pass: %d\n", test_count, test_count_pass);
  printf("==================================================\n\n");

}