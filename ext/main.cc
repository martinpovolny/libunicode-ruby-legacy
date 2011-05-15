/**********************************************************************
 
    Copyright (C) 2005 Jan Becvar
    Copyright (C) 2005 soLNet.cz

**********************************************************************/

#include <ruby-unicode.h>

extern "C" {

#include <st.h>
#include <node.h>
#include <version.h>
#include <config.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

VALUE rb_mUTF8;
VALUE rb_cUString;
VALUE rb_cUCollator;
VALUE rb_cUTime;
VALUE rb_cUTimeFormat;

VALUE rb_cUError;
VALUE rb_cUInvalidCharError;

static VALUE date_default_format = Qnil;

static ID id_allocate;
static ID id_to_str;
static ID id_default;
static ID id_to_f;
static ID id_locale;
static ID id_inspect;
static ID id_full;
static ID id_initialize_copy;
static ID id_month;
static ID id_wday;
static ID id_utf8;
static ID id_utf16;
static ID id_char32;
static ID id_chars32;
static ID id_char;
static ID id_chars;
static ID id_word;
static ID id_words;
static ID id_sentence;
static ID id_sentences;
static ID id_line;
static ID id_lines;
static ID id_string;
static ID id_code_points;

/* Encoding of ruby String */
int ruby_u_encoding = 0;

#ifndef ST_DATA_T_DEFINED
    typedef unsigned long st_data_t;
#endif

/* Caching prearranged objects for used locales */
typedef struct
{
    UBreakIterator *brk[UBRK_TITLE + 1];
} LocaleCache;

static st_table *locale_cache;

 
/************************************************************************
 *            UTILS                                                     *
 ************************************************************************/

#define STACK_BUF_SIZE 8192

/* <c++> */
#ifndef ANYARGS /* These definitions should work for Ruby 1.6 */
#  define PROTECTFUNC(f) ((VALUE (*)()) f)
#  define VALUEFUNC(f) ((VALUE (*)()) f)
#  define VOIDFUNC(f)  ((RUBY_DATA_FUNC) f)
#  define VOID1FUNC(f) ((void (*)(ANYARGS)) f)
#else /* These definitions should work for Ruby 1.8 */
#  define PROTECTFUNC(f) ((VALUE (*)(VALUE)) f)
#  define VALUEFUNC(f) ((VALUE (*)(ANYARGS)) f)
#  define VOIDFUNC(f)  ((RUBY_DATA_FUNC) f)
#  define VOID1FUNC(f) ((void (*)(ANYARGS)) f)
#endif
/* </c++> */

#define ENUM_INIT(name) \
    tbl_id2 ## name = st_init_numtable_with_size (20); \
    tbl_ ## name ## 2id = st_init_numtable_with_size (20);

#define ENUM_ADD(name, symbol, type_item) { \
    ID id = rb_intern (#symbol); \
    st_add_direct (tbl_id2 ## name, id, type_item); \
    st_add_direct (tbl_ ## name ## 2id, type_item, id); \
}

/* creating enum_ hashes */
#define ENUM_CREATE_FUNCS(type, name) \
static st_table *tbl_id2 ## name; \
static st_table *tbl_ ## name ## 2id; \
\
static type \
enum_id2 ## name (ID id) \
{ \
    st_data_t result; \
    if (st_lookup (tbl_id2 ## name, id, &result)) \
        return (type) result; \
    else \
        rb_raise (rb_eArgError, "wrong " #name " name"); \
} \
\
static ID \
enum_ ## name ## 2id (type type_item) \
{ \
    ID result; \
    if (st_lookup (tbl_ ## name ## 2id, type_item, (st_data_t *) &result)) \
        return result; \
    else \
        rb_raise (rb_eArgError, "bug: unknown " #name " item"); \
}
/* /creating enum_ hashes */

#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
#  define ADD_INIT_METHODS(klass, alloc_func) rb_define_alloc_func (klass, alloc_func);
#else
#  define ADD_INIT_METHODS(klass, alloc_func) \
        rb_add_method (CLASS_OF (klass), id_allocate, NEW_CFUNC (alloc_func, 0), NOEX_PRIVATE); \
        rb_define_singleton_method (klass, "new", VALUEFUNC (util_default_new), -1); \
        rb_define_method (klass, "clone", VALUEFUNC (util_default_clone), 0);
#endif


#if RUBY_VERSION_CODE < 170 /* for ruby 1.6 */

#   define CHECK_FROZEN(obj) if (OBJ_FROZEN (obj)) rb_error_frozen (rb_class2name (CLASS_OF (obj)));

    static VALUE 
    util_default_new (int argc, VALUE *argv, VALUE klass)
    {
        VALUE obj = rb_funcall (klass, id_allocate, 0, 0);
        rb_obj_call_init (obj, argc, argv);
        return obj;
    }

    static VALUE
    util_default_clone (VALUE self)
    {           
        VALUE klass = rb_obj_class (self);
        VALUE clone = rb_funcall (klass, id_allocate, 0, 0);

        rb_funcall (clone, id_initialize_copy, 1, self);

        CLONESETUP (clone, self);
        return clone;
    }

    static char *null_str = "";

    static VALUE
    rb_string_value (volatile VALUE *ptr)
    {                  
        VALUE s = *ptr;

        if (TYPE(s) != T_STRING) 
        {
            s = rb_str_to_str (s);
            *ptr = s;
        }
        return s;
    }

    static char *
    rb_string_value_ptr (volatile VALUE *ptr)
    {
        return RSTRING (rb_string_value (ptr))->ptr;
    } 

    static char *
    rb_string_value_cstr (volatile VALUE *ptr)
    {
        VALUE str = rb_string_value (ptr);
        char *s = RSTRING (str)->ptr;
        
        if (!s || RSTRING (str)->len != strlen (s)) 
            rb_raise (rb_eArgError, "string contains null byte");

        return s;
    }

#   define StringValue(v) rb_string_value (&(v))
#   define StringValuePtr(v) rb_string_value_ptr (&(v))
#   define StringValueCStr(v) rb_string_value_cstr (&(v))

#else /* for ruby 1.8 */

#   define CHECK_FROZEN(obj) if (OBJ_FROZEN (obj)) rb_error_frozen (rb_obj_classname (obj));

#endif 


static void
util_raise_error (UErrorCode err)
{
    switch (err)
    {
        case U_INVALID_CHAR_FOUND:
            rb_raise (rb_cUInvalidCharError, "found invalid character");
        default:
            rb_raise (rb_cUError, u_errorName (err));
    }
}

inline static void
util_icu_error (UErrorCode err)
{
    if (U_FAILURE (err))
        util_raise_error (err);
}

static void
util_icu_error_with_free (UErrorCode err, void *data_to_free)
{
    if (U_FAILURE (err))
    {
        if (data_to_free != NULL)
            free (data_to_free);
        util_raise_error (err);
    }
}

static void
util_icu_error_with_delete (UErrorCode err, UObject *object)
{
    if (U_FAILURE (err))
    {
        if (object != NULL)
            delete object;
        util_raise_error (err);
    }
}

static const char *
util_locale2c (VALUE locale)
{
    switch (TYPE (locale))
    {
        case T_NIL:
            return NULL; /* the default locale from system */
        case T_SYMBOL:
            return rb_id2name (SYM2ID (locale)); /* :root == UCA */
        default:
            return StringValueCStr (locale); /* "root" or "" == UCA */
    }
                
}

static const char *
util_zone2c (VALUE zone)
{
    switch (TYPE (zone))
    {
        case T_NIL:
            return NULL; /* the default zone from system */
        case T_SYMBOL:
            return rb_id2name (SYM2ID (zone));
        default:
            return StringValueCStr (zone);
    }
}

static TimeZone *
util_create_timezone (const char *tzid)
{
    TimeZone *zone;

    if (tzid == NULL)
        return TimeZone::createDefault ();

    zone = TimeZone::createTimeZone (rb_ustring_to_UnicodeString (rb_ustring_from_ascii (tzid)));

    if (zone == NULL)
        util_icu_error (U_MEMORY_ALLOCATION_ERROR);
    return zone;
}

static Locale
util_create_locale (const char *locale)
{
    Locale loc;

    loc = Locale (locale);
    if (loc.isBogus ())
        rb_raise (rb_eTypeError, "wrong locale name");

    return loc;
}

static void
util_float_divmod (double x, double y, double *divp, double *modp)
{
    double div, mod;

#ifdef HAVE_FMOD
    mod = fmod (x, y);
#else
    {
        double z;

        modf (x / y, &z);
        mod = x - z * y;
    }
#endif
    div = (x - mod) / y;
    if (y * mod < 0) 
    {
        mod += y;
        div -= 1.0;
    }
    if (modp) 
        *modp = mod;
    if (divp) 
        *divp = div;
}

inline static int32_t
util_check_string_len (VALUE str)
{
    if (RSTRING (str)->len > U_MAX_STRING_LEN)
        rb_raise (rb_eArgError, "string is too long (max. is %d)", U_MAX_STRING_LEN);
    return RSTRING (str)->len;
}

#define util_free_stack_buf(ptr, do_free) if (do_free) free (ptr);
#define util_realloc_stack_buf(ptr, do_free, type, size) \
    if (do_free) \
        REALLOC_N (ptr, type, size); \
    else \
        ptr = ALLOC_N (type, size);

/* If result length is lesser then STACK_BUF_SIZE then returns +stack_buf+ else returns
 * new allocated memory. Na str jiz musi byt zavolano StringValue. Vyvola
 * vyjimku v pripade chyby. */
static UChar *
util_str_to_uchars_try_stack (VALUE str, char *stack_buf, int32_t *dest_len, int *do_free)
{
    int32_t    str_len, len = STACK_BUF_SIZE / sizeof (UChar);
    UChar     *result;
    UErrorCode err = U_ZERO_ERROR;

    *do_free = 0;
    str_len = util_check_string_len (str);

    if (ruby_u_encoding == RUBY_U_UTF16)
    {
        // UTF16
        *dest_len = str_len / sizeof (UChar);
        return (UChar *) RSTRING (str)->ptr;
    }

    // UTF8
    result = (UChar *) stack_buf;

    if (str_len > len)
    {
        len = str_len;
        result = ALLOC_N (UChar, len);
        *do_free = 1;
    }
    u_strFromUTF8 (result,
                   len,
                   dest_len,
                   RSTRING (str)->ptr,
                   str_len,
                   &err);

    /* Nikdy by nemelo nastat:
    if (err == U_BUFFER_OVERFLOW_ERROR)
    {
        util_realloc_stack_buf (result, *do_free, UChar, *dest_len);

        err = U_ZERO_ERROR;
        u_strFromUTF8 (result,
                        *dest_len,
                        NULL,
                        RSTRING (str)->ptr,
                        str_len,
                        &err);
    } */

    if (U_FAILURE (err))
    {
        util_free_stack_buf (result, *do_free);
        util_icu_error (err);
    }

    return result;
}

static LocaleCache *
util_get_locale_cache (const char *locale)
{
    LocaleCache *result;

    if (locale == NULL)
        locale = uloc_getDefault ();

    if (!st_lookup (locale_cache, (st_data_t) locale, (st_data_t *) &result))
    {
        result = ALLOC (LocaleCache);
        memset (result, 0, sizeof (LocaleCache));
        st_add_direct (locale_cache, (st_data_t) strdup (locale), (st_data_t) result);
    }

    return result;
}

#define CAPITALIZE_RULES "!!forward;\n.*;\n!!reverse;\n.*;\n!!safe_forward;\n.*;\n!!safe_reverse;\n.*;\n"

/* Vraci nacachovany BreakIterator daneho typu a locales */
static UBreakIterator *
util_get_ubrk (ID type, const char *locale)
{
    UErrorCode         err = U_ZERO_ERROR;
    LocaleCache       *locale_cache;
    UParseError        parseErr;
    UChar              urules[sizeof (CAPITALIZE_RULES)];
    UBreakIteratorType utype;
    static UBreakIterator *ubrk_string = NULL;

    if (type == id_string)
    {
        if (ubrk_string == NULL)
        {
            u_charsToUChars (CAPITALIZE_RULES, urules, sizeof (CAPITALIZE_RULES));
            ubrk_string = ubrk_openRules (urules,
                                          sizeof (CAPITALIZE_RULES) - 1,
                                          NULL,
                                          -1,
                                          &parseErr, 
                                          &err);
            util_icu_error (err);
        }
        return ubrk_string;
    }

    if (type == id_chars)
        utype = UBRK_CHARACTER;
    else if (type == id_words)
        utype = UBRK_WORD;
    else if (type == id_sentences)
        utype = UBRK_SENTENCE;
    else if (type == id_lines)
        utype = UBRK_LINE;
    else
        rb_raise (rb_eArgError, "wrong type");

    locale_cache = util_get_locale_cache (locale);

    if (locale_cache->brk[utype] == NULL)
    {
        locale_cache->brk[utype] = ubrk_open (utype, locale, NULL, -1, &err);
        util_icu_error (err);
    }

    return locale_cache->brk[utype];
}

/* Vraci nacachovany BreakIterator daneho typu a locales a nastavi ho na dany
 * string */
static UBreakIterator *
util_get_ubrk_for_str (VALUE str, ID type, const char *locale)
{
    UErrorCode      err = U_ZERO_ERROR;
    UText          *utext;
    UBreakIterator *bi = util_get_ubrk (type, locale);

    if (ruby_u_encoding == RUBY_U_UTF16)
        utext = utext_openUChars (NULL, 
                                    (UChar *) RSTRING (str)->ptr, 
                                    RSTRING (str)->len / sizeof (UChar),
                                    &err);
    else
        utext = utext_openUTF8 (NULL, 
                                RSTRING (str)->ptr,
                                RSTRING (str)->len, 
                                &err);
    util_icu_error (err);

    ubrk_setUText (bi, utext, &err);
    utext_close (utext);
    util_icu_error (err);

    return bi;
}


/************************************************************************
 *           OTHERS                                                     *
 ************************************************************************/

static VALUE
u_encoding_getter ()
{
    return ID2SYM (ruby_u_encoding == RUBY_U_UTF8 ? id_utf8 : id_utf16);
}

static void
u_encoding_setter (VALUE sym)
{
    ID id;
    
    Check_Type (sym, T_SYMBOL);
    id = SYM2ID (sym);

    if (id == id_utf8)
        ruby_u_encoding = RUBY_U_UTF8;
    else if (id == id_utf16)
        ruby_u_encoding = RUBY_U_UTF16;
    else
        rb_raise (rb_eArgError, "expected symbol :utf8 or :utf16");
}

/************************************************************************
 *           INIT                                                       *
 ************************************************************************/

#include "ustring.cc.inl"
#include "string.cc.inl"
#include "ucollator.cc.inl"
#include "ucalendar.cc.inl"

#define DEFINE_ID(v) id_ ## v = rb_intern ("" #v "");

void
Init_unicode ()
{
    UErrorCode err = U_ZERO_ERROR;

    /* ICU initializing */
    u_init (&err);
    util_icu_error (err);

    locale_cache = st_init_strtable_with_size (50);

    DEFINE_ID (allocate);
    DEFINE_ID (to_str);
    DEFINE_ID (default);
    DEFINE_ID (to_f);
    DEFINE_ID (locale);
    DEFINE_ID (inspect);
    DEFINE_ID (full);
    DEFINE_ID (initialize_copy);
    DEFINE_ID (utf8);
    DEFINE_ID (utf16);
    DEFINE_ID (char32);
    DEFINE_ID (chars32);
    DEFINE_ID (char);
    DEFINE_ID (chars);
    DEFINE_ID (word);
    DEFINE_ID (words);
    DEFINE_ID (sentence);
    DEFINE_ID (sentences);
    DEFINE_ID (line);
    DEFINE_ID (lines);
    DEFINE_ID (string);
    DEFINE_ID (code_points);
    
    rb_cUError = rb_define_class ("UError", rb_eRuntimeError);
    rb_cUInvalidCharError = rb_define_class ("UInvalidCharError", rb_cUError);

    rb_define_virtual_variable ("$U_ENCODING", 
                                VALUEFUNC (u_encoding_getter), 
                                VOID1FUNC (u_encoding_setter));

    string_init ();
    ustring_init ();
    ucollator_init ();
    ucalendar_init ();
}

} /* extern "C" */

