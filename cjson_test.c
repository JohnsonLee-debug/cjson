#include "cjson.h"
#include <stdio.h>
#include <string.h>

static int main_ret_val = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)                           \
do {                                                                               \
        test_count++;                                                              \
        if (equality)                                                              \
        test_pass++;                                                               \
        else {                                                                     \
        fprintf(stderr, "%s:%d: expect: " format " actual:" format "\n",           \
                __FILE__, __LINE__, expect, actual);                               \
        main_ret_val = 1;                                                          \
        }                                                                          \
  } while (0)

#define EXPECT_EQ_INT(expect, actual)\
        EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_STRING(expect, actual, alength) \
        EXPECT_EQ_BASE(sizeof(expect) - 1 == (alength) && \
                       memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_NULL(actual) EXPECT_EQ_INT(JSON_NULL, (actual))
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual), "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE(!(actual), "false", "true", "%s")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)(expect), (size_t)(actual), "%zu")

#define TEST_LITERAL(literal)\
        do{                                                             \
                json_value v;                                           \
                v.type = JSON_NULL;                                     \
                EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, #literal)); \
        }while(0)

#define TEST_ERROR(error, json)                                 \
        do{                                                     \
                json_value v;                                   \
                v.type = JSON_FALSE;                            \
                EXPECT_EQ_INT(error, json_parse(&v, json));     \
                EXPECT_EQ_INT(JSON_NULL, json_get_type(&v));    \
        }while(0)

#define TEST_NUMBER(expect, json)                                       \
        do{                                                             \
                json_value v;                                           \
                EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, json));     \
                EXPECT_EQ_INT(JSON_NUMBER, json_get_type(&v));          \
                EXPECT_EQ_DOUBLE(expect, json_get_number(&v));          \
        }while(0)


static void test_parse_literal() {
        TEST_LITERAL(null);
        TEST_LITERAL(false);
        TEST_LITERAL(true);
}

static void test_parse_expect_value() {
        TEST_ERROR(JSON_PARSE_EXPECT_VALUE, "");
        TEST_ERROR(JSON_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
        /* invalid literal */
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "nul");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "?");

        /* invalid number */
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "+0");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "+1");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, ".123");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "1.");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "INF");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "inf");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "NAN");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "nan");

        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "[1,]");
        TEST_ERROR(JSON_PARSE_INVALID_VALUE, "[\"a\", nul]");
}

static void test_parse_number() {
        TEST_NUMBER(0.0, "0");
        TEST_NUMBER(0.0, "-0");
        TEST_NUMBER(0.0, "-0.0");
        TEST_NUMBER(1.0, "1");
        TEST_NUMBER(-1.0, "-1");
        TEST_NUMBER(1.5, "1.5");
        TEST_NUMBER(-1.5, "-1.5");
        TEST_NUMBER(3.1416, "3.1416");
        TEST_NUMBER(1E10, "1E10");
        TEST_NUMBER(1e10, "1e10");
        TEST_NUMBER(1E+10, "1E+10");
        TEST_NUMBER(1E-10, "1E-10");
        TEST_NUMBER(-1E10, "-1E10");
        TEST_NUMBER(-1e10, "-1e10");
        TEST_NUMBER(-1E+10, "-1E+10");
        TEST_NUMBER(-1E-10, "-1E-10");
        TEST_NUMBER(1.234E+10, "1.234E+10");
        TEST_NUMBER(1.234E-10, "1.234E-10");
        TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
        TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); // the smallest number > 1
        TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324");// Min. subnormal positive double
        TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");// Max. subnormal double
        TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");// Min. normal positive double
        TEST_NUMBER(1.7976931348623157e308, "1.7976931348623157e308"); // Max. Double
}

static void test_parse_number_too_big() {
        TEST_ERROR(JSON_PARSE_NUMBER_TOO_BIG, "1e309");
        TEST_ERROR(JSON_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_root_not_singular() {
        TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "null x");
        /* invalid number */
        TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
        TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "0x0");
        TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "0x123");
}
static void test_parse_miss_comma_or_square_bracket() {
        TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
        TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
        TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
        TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}
#define TEST_STRING(expect, json)\
        do {\
                json_value v;\
                json_val_init(&v);\
                EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, json));\
                EXPECT_EQ_INT(JSON_STRING, json_get_type(&v));\
                EXPECT_EQ_STRING(expect, json_get_string(&v), json_get_string_length(&v));\
                json_val_free(&v);\
        } while(0)
static void test_parse_string() {
        TEST_STRING("", "\"\"");
        TEST_STRING("Hello", "\"Hello\"");
        TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
        TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
        TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
        TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
        TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
        TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
        TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
        TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_access_null() {
        json_value v;
        json_val_init(&v);
        json_set_string(&v, "a", 1);
        json_set_null(&v);
        EXPECT_EQ_INT(JSON_NULL, json_get_type(&v));
        json_val_free(&v);
}

static void test_access_boolean() {
        json_value v;
        json_val_init(&v);
        json_set_boolean(&v, 1);
        EXPECT_TRUE(json_get_boolean(&v));
        json_set_boolean(&v, 0);
        EXPECT_FALSE(json_get_boolean(&v));
        json_val_free(&v);
}

static void test_access_string() {
        json_value v;
        json_val_init(&v);
        json_set_boolean(&v, 1);
        json_set_string(&v,"abc", 3);
        EXPECT_EQ_STRING("abc", json_get_string(&v), 3);
        json_val_free(&v);
}

static void test_access_number() {
        json_value v;
        json_val_init(&v);
        json_set_boolean(&v, 1);
        json_set_string(&v,"abc", 3);
        json_set_number(&v, 123.123);
        EXPECT_EQ_DOUBLE(123.123, json_get_number(&v));
        json_val_free(&v);
}

static void test_parse_missing_quotation_mark() {
        TEST_ERROR(JSON_PARSE_MISS_QUOTATION_MARK, "\"");
        TEST_ERROR(JSON_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
        TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
        TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
        TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
        TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
        TEST_ERROR(JSON_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
        TEST_ERROR(JSON_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}
static void test_parse_invalid_unicode_hex() {
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_parse_invalid_unicode_surrogate() {
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
        TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_array() {
        json_value v;
        json_val_init(&v);
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "[ ]"));
        EXPECT_EQ_INT(JSON_ARRAY, json_get_type(&v));
        EXPECT_EQ_SIZE_T(0, json_get_array_size(&v));
        json_val_free(&v);

        json_val_init(&v);
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "[ null, false, true,  123, \"abc\"  ]"));
        EXPECT_EQ_INT(JSON_ARRAY, json_get_type(&v));
        EXPECT_EQ_SIZE_T(5, json_get_array_size(&v));
        EXPECT_NULL(json_get_type(json_get_array_element(&v, 0)));
        EXPECT_FALSE(json_get_boolean(json_get_array_element(&v, 1)));
        EXPECT_TRUE(json_get_boolean(json_get_array_element(&v, 2)));
        EXPECT_EQ_DOUBLE(123, json_get_number(json_get_array_element(&v, 3)));
        EXPECT_EQ_STRING("abc", json_get_string(json_get_array_element(&v, 4)), 3);
        json_val_free(&v);

        json_val_init(&v);
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "[[                 ] \n, [    0 ] , [ 0       ,              1 ] ,                    [ 0 , 1 , 2 ]]"));
        EXPECT_EQ_INT(JSON_ARRAY, json_get_type(&v));
        EXPECT_EQ_SIZE_T(0, json_get_array_size(json_get_array_element(&v, 0)));
        EXPECT_EQ_SIZE_T(1, json_get_array_size(json_get_array_element(&v, 1)));
        EXPECT_EQ_SIZE_T(2, json_get_array_size(json_get_array_element(&v, 2)));
        EXPECT_EQ_SIZE_T(3, json_get_array_size(json_get_array_element(&v, 3)));

        EXPECT_EQ_DOUBLE(0.0, json_get_number(json_get_array_element(json_get_array_element(&v, 1), 0)));

        EXPECT_EQ_DOUBLE(0.0, json_get_number(json_get_array_element(json_get_array_element(&v, 2), 0)));
        EXPECT_EQ_DOUBLE(1.0, json_get_number(json_get_array_element(json_get_array_element(&v, 2), 1)));

        EXPECT_EQ_DOUBLE(0.0, json_get_number(json_get_array_element(json_get_array_element(&v, 3), 0)));
        EXPECT_EQ_DOUBLE(1.0, json_get_number(json_get_array_element(json_get_array_element(&v, 3), 1)));
        EXPECT_EQ_DOUBLE(2.0, json_get_number(json_get_array_element(json_get_array_element(&v, 3), 2)));
        json_val_free(&v);
}

static void test_access(){
        test_access_null();
        test_access_boolean();
        test_access_string();
        test_access_number();
}

static void test_parse() {
        test_parse_literal();
        test_parse_string();
        test_parse_number();
        test_parse_array();
        test_parse_expect_value();
        test_parse_invalid_value();
        test_parse_root_not_singular();
        test_parse_number_too_big();
        test_parse_missing_quotation_mark();
        test_parse_invalid_string_escape();
        test_parse_invalid_string_char();
        test_parse_invalid_unicode_hex();
        test_parse_invalid_unicode_surrogate();
        test_parse_miss_comma_or_square_bracket();
}

int main() {
        test_parse();
        test_access();
        printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
               test_pass * 100.0 / test_count);
        return main_ret_val;
}
