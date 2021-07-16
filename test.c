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
#define TEST_TRUE(actuall) TEST_BASE((actuall) != 0, "true", "false", "%s")
#define TEST_FALSE(actuall) TEST_BASE((actuall) == 0, "false", "true", "%s")


#define TEST_JSON_LITERAL(json_type, json) \
do{\
  cjson_value v = {0};\
  cjson_value_init(&v);\
  v.type = CJSON_OBJECT;\
  TEST_INT(CJSON_OK, cjson_parse(&v, json));\
  TEST_INT(json_type, cjson_get_type(v));\
  cjson_value_free(&v);\
}while(0)

static void test_prase_literal()
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
    cjson_value_init(&v);\
    TEST_INT(CJSON_OK, cjson_parse(&v, (json)));\
    TEST_INT(CJSON_NUMBER, cjson_get_type(v));\
    TEST_DOUBLE(except, cjson_get_number(v));\
    cjson_value_free(&v);\
  }while(0)

static void test_prase_number()
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
    cjson_value_init(&v);\
    TEST_INT(CJSON_OK, cjson_parse(&v, (json)));\
    TEST_INT(CJSON_STRING, cjson_get_type(v));\
    TEST_STRING(except, cjson_get_string(v), cjson_get_string_length(v));\
    cjson_value_free(&v);\
  }while(0)

static void test_prase_string()
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

static void test_prase_array()
{
  cjson_value v = {0};
  cjson_value_init(&v);

  TEST_INT(CJSON_OK, cjson_parse(&v, "[ ]"));
  TEST_INT(CJSON_ARRAY, cjson_get_type(v));
  TEST_SIZE_T(0, cjson_get_array_size(v));
  cjson_value_free(&v);

  cjson_value_init(&v);
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
  cjson_value_free(&v);

  cjson_value_init(&v);
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
  cjson_value_free(&v);
}

static void test_prase_object() {
  cjson_value v;
  size_t i;

  cjson_value_init(&v);
  TEST_INT(CJSON_OK, cjson_parse(&v, " { } "));
  TEST_INT(CJSON_OBJECT, cjson_get_type(v));
  TEST_SIZE_T(0, cjson_get_object_size(v));
  cjson_value_free(&v);
  

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
      TEST_BASE(('1' + i == cjson_get_object_key(*o, i)[0]) != 0, "true", "false", "%s");
      TEST_SIZE_T(1, cjson_get_object_key_length(*o, i));
      TEST_INT(CJSON_NUMBER, cjson_get_type(*ov));
      TEST_DOUBLE(i + 1.0, cjson_get_number(*ov));
    }
  }
  cjson_value_free(&v);
}


#define TEST_PARSE_ERROR(error, json)\
  do {\
    cjson_value v;\
    cjson_value_init(&v);\
    v.type = CJSON_FALSE;\
    TEST_INT(error, cjson_parse(&v, json));\
    TEST_INT(CJSON_NULL, cjson_get_type(v));\
    cjson_value_free(&v);\
  } while(0)

static void test_parse_expect_value() {
  TEST_PARSE_ERROR(CJSON_ERR_MISS_VALUE, "");
  TEST_PARSE_ERROR(CJSON_ERR_MISS_VALUE, " ");
}

static void test_parse_invalid_value() {
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "nul");
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "?");

  /* invalid number */
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "+0");
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "+1");
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, ".123"); /* at least one digit before '.' */
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "1.");   /* at least one digit after '.' */
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "INF");
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "inf");
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "NAN");
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "nan");

  /* invalid value in array */
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "[1,]");
  TEST_PARSE_ERROR(CJSON_ERR_LITERAL, "[\"a\", nul]");
}

static void test_parse_root_not_singular() {
  TEST_PARSE_ERROR(CJSON_ERR_ROOT_NOT_SINGULAR, "null x");

  /* invalid number */
  TEST_PARSE_ERROR(CJSON_ERR_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
  TEST_PARSE_ERROR(CJSON_ERR_ROOT_NOT_SINGULAR, "0x0");
  TEST_PARSE_ERROR(CJSON_ERR_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
  TEST_PARSE_ERROR(CJSON_ERR_NUMBER_TOO_BIG, "1e309");
  TEST_PARSE_ERROR(CJSON_ERR_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_miss_quotation_mark() {
  TEST_PARSE_ERROR(CJSON_ERR_STRING_MISS_QUOTATION_MARK, "\"");
  TEST_PARSE_ERROR(CJSON_ERR_STRING_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
  TEST_PARSE_ERROR(CJSON_ERR_STRING_INVALID_ESCAPE, "\"\\v\"");
  TEST_PARSE_ERROR(CJSON_ERR_STRING_INVALID_ESCAPE, "\"\\'\"");
  TEST_PARSE_ERROR(CJSON_ERR_STRING_INVALID_ESCAPE, "\"\\0\"");
  TEST_PARSE_ERROR(CJSON_ERR_STRING_INVALID_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
  TEST_PARSE_ERROR(CJSON_ERR_STRING_INVALID_CAHR, "\"\x01\"");
  TEST_PARSE_ERROR(CJSON_ERR_STRING_INVALID_CAHR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u0\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u01\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u012\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u/000\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\uG000\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u0/00\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u0G00\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u0/00\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u00G0\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u000/\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u000G\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_SURROGATE, "\"\\uD800\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_SURROGATE, "\"\\uDBFF\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");  //0xDBFF < 0xDC00
  TEST_PARSE_ERROR(CJSON_ERR_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");  //0xE000 > 0xDFFF
}

static void test_parse_miss_comma_or_square_bracket() {
  TEST_PARSE_ERROR(CJSON_ERR_ARRAY_NEED_COMMA_OR_SQUARE_BRACKET, "[1");
  TEST_PARSE_ERROR(CJSON_ERR_ARRAY_NEED_COMMA_OR_SQUARE_BRACKET, "[1}");
  TEST_PARSE_ERROR(CJSON_ERR_ARRAY_NEED_COMMA_OR_SQUARE_BRACKET, "[1 2");
  TEST_PARSE_ERROR(CJSON_ERR_ARRAY_NEED_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_KEY, "{:1,");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_KEY, "{1:1,");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_KEY, "{true:1,");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_KEY, "{false:1,");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_KEY, "{null:1,");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_KEY, "{[]:1,");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_KEY, "{{}:1,");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_COLON, "{\"a\"}");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_COMMA_OR_SQUARE_BRACKET, "{\"a\":1");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_COMMA_OR_SQUARE_BRACKET, "{\"a\":1]");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_COMMA_OR_SQUARE_BRACKET, "{\"a\":1 \"b\"");
  TEST_PARSE_ERROR(CJSON_ERR_OBJECT_NEED_COMMA_OR_SQUARE_BRACKET, "{\"a\":{}");
}

static void test_prase()
{
  test_prase_literal();
  test_prase_number();
  test_prase_string();
  test_prase_array();
  test_prase_object();

  test_parse_expect_value();
  test_parse_invalid_value();
  test_parse_root_not_singular();
  test_parse_number_too_big();
  test_parse_miss_quotation_mark();
  test_parse_invalid_string_escape();
  test_parse_invalid_string_char();
  test_parse_invalid_unicode_hex();
  test_parse_invalid_unicode_surrogate();
  test_parse_miss_comma_or_square_bracket();
  test_parse_miss_key();
  test_parse_miss_colon();
  test_parse_miss_comma_or_curly_bracket();
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
    cjson_value_free(&v);\
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



#define TEST_EQUAL(json1, json2, equality) \
    do {\
        cjson_value v1 = {0}, v2 = {0};\
        cjson_value_init(&v1);\
        cjson_value_init(&v2);\
        TEST_INT(CJSON_OK, cjson_parse(&v1, json1));\
        TEST_INT(CJSON_OK, cjson_parse(&v2, json2));\
        TEST_INT(equality, cjson_is_equal(&v1, &v2));\
        cjson_value_free(&v1);\
        cjson_value_free(&v2);\
    } while(0)

static void test_equal() {
    TEST_EQUAL("true", "true", 1);
    TEST_EQUAL("true", "false", 0);
    TEST_EQUAL("false", "false", 1);
    TEST_EQUAL("null", "null", 1);
    TEST_EQUAL("null", "0", 0);
    TEST_EQUAL("123", "123", 1);
    TEST_EQUAL("123", "456", 0);
    TEST_EQUAL("\"abc\"", "\"abc\"", 1);
    TEST_EQUAL("\"abc\"", "\"abcd\"", 0);
    TEST_EQUAL("[]", "[]", 1);
    TEST_EQUAL("[]", "null", 0);
    TEST_EQUAL("[1,2,3]", "[1,2,3]", 1);
    TEST_EQUAL("[1,2,3]", "[1,2,3,4]", 0);
    TEST_EQUAL("[[]]", "[[]]", 1);
    TEST_EQUAL("{}", "{}", 1);
    TEST_EQUAL("{}", "null", 0);
    TEST_EQUAL("{}", "[]", 0);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}", 1);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"b\":2,\"a\":1}", 1);  //键值对顺序不同
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":3}", 0);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2,\"c\":3}", 0);
    TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":{}}}}", 1);
    TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":[]}}}", 0);
}

static void test_copy() {
    cjson_value v1 = {0}, v2 = {0};
    cjson_value_init(&v1);
    cjson_parse(&v1, "{\"t\":true,\"f\":false,\"n\":null,\"d\":1.5,\"a\":[1,2,3]}");
    cjson_value_init(&v2);
    cjson_copy(&v2, &v1);
    TEST_BASE((cjson_is_equal(&v2, &v1)) != 0, "true", "false", "%s");
    cjson_value_free(&v1);
    cjson_value_free(&v2);
}

static void test_move() {
    cjson_value v1, v2, v3;
    cjson_value_init(&v1);
    cjson_parse(&v1, "{\"t\":true,\"f\":false,\"n\":null,\"d\":1.5,\"a\":[1,2,3]}");
    cjson_value_init(&v2);
    cjson_copy(&v2, &v1);
    cjson_value_init(&v3);
    cjson_move(&v3, &v2);
    TEST_INT(CJSON_OK, cjson_get_type(v2));
    TEST_BASE((cjson_is_equal(&v3, &v1)) != 0, "true", "false", "%s");
    cjson_value_free(&v1);
    cjson_value_free(&v2);
    cjson_value_free(&v3);
}

static void test_swap() {
    cjson_value v1, v2;
    cjson_value_init(&v1);
    cjson_value_init(&v2);
    cjson_set_string(&v1, "Hello",  5);
    cjson_set_string(&v2, "World!", 6);
    cjson_swap(&v1, &v2);
    TEST_STRING("World!", cjson_get_string(v1), cjson_get_string_length(v1));
    TEST_STRING("Hello",  cjson_get_string(v2), cjson_get_string_length(v2));
    cjson_value_free(&v1);
    cjson_value_free(&v2);
}

static void test_access_null() {
    cjson_value v;
    cjson_value_init(&v);
    cjson_set_string(&v, "a", 1);
    cjson_set_null(&v);
    TEST_INT(CJSON_NULL, cjson_get_type(v));
    cjson_value_free(&v);
}

static void test_access_boolean() {
    cjson_value v;
    cjson_value_init(&v);
    cjson_set_string(&v, "a", 1);
    cjson_set_boolean(&v, 1);
    TEST_TRUE(cjson_get_boolean(v));
    cjson_set_boolean(&v, 0);
    TEST_FALSE(cjson_get_boolean(v));
    cjson_value_free(&v);
}

static void test_access_number() {
    cjson_value v;
    cjson_value_init(&v);
    cjson_set_string(&v, "a", 1);
    cjson_set_number(&v, 1234.5);
    TEST_DOUBLE(1234.5, cjson_get_number(v));
    cjson_value_free(&v);
}

static void test_access_string() {
    cjson_value v;
    cjson_value_init(&v);
    cjson_set_string(&v, "", 0);
    TEST_STRING("", cjson_get_string(v), cjson_get_string_length(v));
    cjson_set_string(&v, "Hello", 5);
    TEST_STRING("Hello", cjson_get_string(v), cjson_get_string_length(v));
    cjson_value_free(&v);
}

static void test_access_array() {
    cjson_value a, e;
    size_t i, j;

    cjson_value_init(&a);

    for (j = 0; j <= 5; j += 5) {
        cjson_init_array(&a, j);
        TEST_SIZE_T(0, cjson_get_array_size(a));
        TEST_SIZE_T(j, cjson_get_array_capacity(a));
        for (i = 0; i < 10; i++) {
            cjson_value_init(&e);
            cjson_set_number(&e, i);
            cjson_move(cjson_pushback_array_element(&a), &e);
            cjson_value_free(&e);
        }

        TEST_SIZE_T(10, cjson_get_array_size(a));
        for (i = 0; i < 10; i++)
            TEST_DOUBLE((double)i, cjson_get_number(*cjson_get_array_element(a, i)));
    }

    cjson_popback_array_element(&a);
    TEST_SIZE_T(9, cjson_get_array_size(a));
    for (i = 0; i < 9; i++)
        TEST_DOUBLE((double)i, cjson_get_number(*cjson_get_array_element(a, i)));

    cjson_erase_array_element(&a, 4, 0);
    TEST_SIZE_T(9, cjson_get_array_size(a));
    for (i = 0; i < 9; i++)
        TEST_DOUBLE((double)i, cjson_get_number(*cjson_get_array_element(a, i)));

    cjson_erase_array_element(&a, 8, 1);
    TEST_SIZE_T(8, cjson_get_array_size(a));
    for (i = 0; i < 8; i++)
        TEST_DOUBLE((double)i, cjson_get_number(*cjson_get_array_element(a, i)));

    cjson_erase_array_element(&a, 0, 2);
    TEST_SIZE_T(6, cjson_get_array_size(a));
    for (i = 0; i < 6; i++)
        TEST_DOUBLE((double)i + 2, cjson_get_number(*cjson_get_array_element(a, i)));

#if 0
    for (i = 0; i < 2; i++) {
        cjson_value_init(&e);
        cjson_set_number(&e, i);
        cjson_move(cjson_insert_array_element(&a, i), &e);
        cjson_value_free(&e);
    }
#endif
    
    TEST_SIZE_T(8, cjson_get_array_size(a));
    for (i = 0; i < 8; i++)
        TEST_DOUBLE((double)i, cjson_get_number(*cjson_get_array_element(a, i)));

    TEST_TRUE(cjson_get_array_capacity(a) > 8);
    cjson_shrink_array(&a);
    TEST_SIZE_T(8, cjson_get_array_capacity(a));
    TEST_SIZE_T(8, cjson_get_array_size(a));
    for (i = 0; i < 8; i++)
        TEST_DOUBLE((double)i, cjson_get_number(*cjson_get_array_element(a, i)));

    cjson_set_string(&e, "Hello", 5);
    cjson_move(cjson_pushback_array_element(&a), &e);     /* Test if element is freed */
    cjson_value_free(&e);

    i = cjson_get_array_capacity(a);
    cjson_clear_array_element(&a);
    TEST_SIZE_T(0, cjson_get_array_size(a));
    TEST_SIZE_T(i, cjson_get_array_capacity(a));   /* capacity remains unchanged */
    cjson_shrink_array(&a);
    TEST_SIZE_T(0, cjson_get_array_capacity(a));

    cjson_value_free(&a);
}

// static void test_access_object() {
// #if 0
//     cjson_value o, v, *pv;
//     size_t i, j, index;

//     cjson_value_init(&o);

//     for (j = 0; j <= 5; j += 5) {
//         cjson_set_object(&o, j);
//         TEST_SIZE_T(0, cjson_get_object_size(&o));
//         TEST_SIZE_T(j, cjson_get_object_capacity(&o));
//         for (i = 0; i < 10; i++) {
//             char key[2] = "a";
//             key[0] += i;
//             cjson_value_init(&v);
//             cjson_set_number(&v, i);
//             cjson_move(cjson_set_object_value(&o, key, 1), &v);
//             cjson_value_free(&v);
//         }
//         TEST_SIZE_T(10, cjson_get_object_size(&o));
//         for (i = 0; i < 10; i++) {
//             char key[] = "a";
//             key[0] += i;
//             index = cjson_find_object_index(&o, key, 1);
//             EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
//             pv = cjson_get_object_value(&o, index);
//             TEST_DOUBLE((double)i, cjson_get_number(pv));
//         }
//     }

//     index = cjson_find_object_index(&o, "j", 1);    
//     EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
//     cjson_remove_object_value(&o, index);
//     index = cjson_find_object_index(&o, "j", 1);
//     EXPECT_TRUE(index == LEPT_KEY_NOT_EXIST);
//     TEST_SIZE_T(9, cjson_get_object_size(&o));

//     index = cjson_find_object_index(&o, "a", 1);
//     EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
//     cjson_remove_object_value(&o, index);
//     index = cjson_find_object_index(&o, "a", 1);
//     EXPECT_TRUE(index == LEPT_KEY_NOT_EXIST);
//     TEST_SIZE_T(8, cjson_get_object_size(&o));

//     EXPECT_TRUE(cjson_get_object_capacity(&o) > 8);
//     cjson_shrink_object(&o);
//     TEST_SIZE_T(8, cjson_get_object_capacity(&o));
//     TEST_SIZE_T(8, cjson_get_object_size(&o));
//     for (i = 0; i < 8; i++) {
//         char key[] = "a";
//         key[0] += i + 1;
//         TEST_DOUBLE((double)i + 1, cjson_get_number(cjson_get_object_value(&o, cjson_find_object_index(&o, key, 1))));
//     }

//     cjson_set_string(&v, "Hello", 5);
//     cjson_move(cjson_set_object_value(&o, "World", 5), &v); /* Test if element is freed */
//     cjson_value_free(&v);

//     pv = cjson_find_object_value(&o, "World", 5);
//     EXPECT_TRUE(pv != NULL);
//     EXPECT_EQ_STRING("Hello", cjson_get_string(pv), cjson_get_string_length(pv));

//     i = cjson_get_object_capacity(&o);
//     cjson_clear_object(&o);
//     TEST_SIZE_T(0, cjson_get_object_size(&o));
//     TEST_SIZE_T(i, cjson_get_object_capacity(&o)); /* capacity remains unchanged */
//     cjson_shrink_object(&o);
//     TEST_SIZE_T(0, cjson_get_object_capacity(&o));

//     cjson_value_free(&o);
// #endif
// }

static void test_access() {
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
    test_access_array();
    // test_access_object();
}


void main()
{
  test_prase();
  test_stringify();

  test_equal();
  test_copy();
  test_move();
  test_swap();

  test_access();

  printf("\n===================== result =====================\n");
  printf("  test all %d, pass: %d\n", test_count, test_count_pass);
  printf("==================================================\n\n");
}
