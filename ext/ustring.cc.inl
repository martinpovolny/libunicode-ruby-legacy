/**********************************************************************
 
    Copyright (C) 2005 Jan Becvar
    Copyright (C) 2005 soLNet.cz

**********************************************************************/

/************************************************************************
 *           UString (UTF-16)                                           *
 ************************************************************************/

/* TODO
 * rb_str_modify
 */


static void 
ustring_free (RUString *str)
{
    if (str->ptr != NULL)
        free (str->ptr);
    free (str);
}

#undef  IS_USTRING
#define IS_USTRING(v) (TYPE (v) == T_DATA && RDATA (v)->dfree == (void (*)(void*))ustring_free)

int
rb_is_ustring (VALUE s)
{
    return IS_USTRING (s) ? 1 : 0;
}

RUString *
rb_ustring_struct (volatile VALUE *ptr)
{
    VALUE s = *ptr;

    if (!IS_USTRING (s))
    {
        s = rb_ustring_from_str (s);
        *ptr = s;
    }

    return ((RUString *) DATA_PTR (s));
}

static VALUE 
ustring_alloc (VALUE klass)
{
    RUString *str = ALLOC (RUString);
    str->len = 0;
    str->ptr = 0;
    str->len_chars = 0;

    return Data_Wrap_Struct (klass, 
                             0, 
                             ustring_free, 
                             str);
}

VALUE 
rb_ustring_new (UChar *ptr, long len)
{
    VALUE       str;
    RUString *cstr;

    str = ustring_alloc (rb_cUString);
    cstr = UStringStruct (str);
    cstr->len = len;
    cstr->ptr = ptr;
    cstr->len_chars = len == 0 ? 0 : -1;
    return str;
}

VALUE
rb_ustring_new0 ()
{
    return ustring_alloc (rb_cUString);
}

VALUE 
rb_ustring_replace (VALUE self, VALUE orig)
{   
    RUString *uself;

    if (self == orig)
        return self;

    CHECK_FROZEN (self);
    uself = UStringStruct (self);

    if (IS_USTRING (orig))
    {
        /* from UString */
        RUString *uorig = UStringStruct (orig);

        if (uorig->len > 0)
        {
            REALLOC_N (uself->ptr, UChar, uorig->len);
            memcpy (uself->ptr, uorig->ptr, uorig->len * sizeof (UChar));
        }
        uself->len = uorig->len;
        uself->len_chars = uorig->len_chars;
    }
    else 
    {
        /* from ruby String */
        long len;
        int32_t dest_len;
        UErrorCode err = U_ZERO_ERROR;

        StringValue (orig);

        len = RSTRING (orig)->len; 
        uself->len = 0;
        uself->len_chars = 0;

        if (len > 0)
        {
            REALLOC_N (uself->ptr, UChar, len);
            u_strFromUTF8 (uself->ptr,
                           len,
                           &dest_len,
                           RSTRING (orig)->ptr,
                           len,
                           &err);
            util_icu_error (err);

            if (dest_len > len)
            {
                /* buffer is too small */
                REALLOC_N (uself->ptr, UChar, dest_len);
                u_strFromUTF8 (uself->ptr, 
                               dest_len,
                               NULL,
                               RSTRING (orig)->ptr,
                               len,
                               &err);
                util_icu_error (err);
            }

            uself->len = dest_len;
            uself->len_chars = -1;
        }
    }
     
    OBJ_INFECT (self, orig);
    return self;
}

VALUE
rb_ustring_from_str (VALUE copy_str)
{
    return rb_ustring_replace (ustring_alloc (rb_cUString),
                               copy_str);
}

VALUE
rb_ustring_from_ascii (const char *ascii_str)
{
    int         len;
    UChar       *ustr;

    if (ascii_str == NULL)
        return rb_ustring_new0 ();

    for (len = 0; ascii_str[len] != 0; len++)
        if (ascii_str[len] < 0)
            rb_raise (rb_eArgError, "string contains non ascii characters");

    if (len == 0)
        return rb_ustring_new0 ();
            
    ustr = ALLOC_N (UChar, len + 1);
    u_uastrcpy (ustr, ascii_str);

    return rb_ustring_new (ustr, len);
}

VALUE
rb_ustring_from_UnicodeString (UnicodeString *uni_str)
{
    int          len;
    const UChar *buff;
    UChar       *str;

    if (uni_str == NULL)
        return rb_ustring_new0 ();

    buff = uni_str->getBuffer ();
    len  = uni_str->length ();

    if (buff == NULL || len == 0)
        return rb_ustring_new0 ();
            
    str = ALLOC_N (UChar, len);
    memcpy (str, buff, len * sizeof (UChar));

    return rb_ustring_new (str, len);
}

static VALUE 
ustring_initialize (int argc, VALUE *argv, VALUE self)
{
    VALUE  str;

    if (rb_scan_args (argc, argv, "01", &str) > 0)
        rb_ustring_replace (self, str);
    return Qnil;
}

static VALUE
ustring_to_s (VALUE self)
{
    UErrorCode   err = U_ZERO_ERROR;
    RUString  *ustr   = UStringStruct (self);
    VALUE      result = rb_str_from_uchars (ustr->ptr, ustr->len, &err);

    util_icu_error (err);
    OBJ_INFECT (result, self);
    return result;
}

static VALUE
ustring_inspect (VALUE self)
{
    return rb_funcall (ustring_to_s (self), id_inspect, 0, 0);
}

UnicodeString
rb_ustring_to_UnicodeString (VALUE self)
{
    UnicodeString result;
    RUString   *cstr = UStringStruct (self);
    result.setTo (false, cstr->ptr, cstr->len);
    return result;
}

static VALUE 
ustring_size (VALUE str)
{
    RUString *ustr = UStringStruct (str);

    if (ustr->len_chars == -1)
        ustr->len_chars = u_countChar32 (ustr->ptr, ustr->len);

    return INT2NUM (ustr->len_chars);
}

static VALUE 
ustring_downcase (VALUE str, VALUE locale)
{
    RUString  *ustr;
    int32_t      dest_len;
    UChar       *dest;
    VALUE        result;
    const char  *clocale;
    UErrorCode   err = U_ZERO_ERROR;

    ustr = UStringStruct (str);

    if (ustr->len == 0)
    {
        result = rb_ustring_new0 ();
    }
    else
    {
        clocale = util_locale2c (locale);
        dest = ALLOC_N (UChar, ustr->len);
        
        dest_len = u_strToLower (dest, 
                                 ustr->len,
                                 ustr->ptr, 
                                 ustr->len,
                                 clocale,
                                 &err);
        util_icu_error_with_free (err, dest);

        if (dest_len > ustr->len)
        {
            /* buffer is too small */
            REALLOC_N (dest, UChar, dest_len);

            dest_len = u_strToLower (dest, 
                                     dest_len,
                                     ustr->ptr, 
                                     ustr->len,
                                     clocale,
                                     &err);
            util_icu_error_with_free (err, dest);
        }

        result = rb_ustring_new (dest, dest_len);
    }

    OBJ_INFECT (result, str);
    return result;
}

static VALUE 
ustring_upcase (VALUE str, VALUE locale)
{
    RUString  *ustr;
    int32_t      dest_len;
    UChar       *dest;
    VALUE        result;
    const char  *clocale;
    UErrorCode   err = U_ZERO_ERROR;

    ustr = UStringStruct (str);

    if (ustr->len == 0)
    {
        result = rb_ustring_new0 ();
    }
    else
    {
        clocale = util_locale2c (locale);
        dest = ALLOC_N (UChar, ustr->len);
        
        dest_len = u_strToUpper (dest, 
                                 ustr->len,
                                 ustr->ptr, 
                                 ustr->len,
                                 clocale,
                                 &err);
        util_icu_error_with_free (err, dest);

        if (dest_len > ustr->len)
        {
            /* buffer is too small */
            REALLOC_N (dest, UChar, dest_len);

            dest_len = u_strToUpper (dest, 
                                     dest_len,
                                     ustr->ptr, 
                                     ustr->len,
                                     clocale,
                                     &err);
            util_icu_error_with_free (err, dest);
        }

        result = rb_ustring_new (dest, dest_len);
    }

    OBJ_INFECT (result, str);
    return result;
}

/************************************************************************
 *           INIT                                                       *
 ************************************************************************/

static void
ustring_init ()
{
    rb_cUString = rb_define_class ("UString", rb_cObject);
    ADD_INIT_METHODS (rb_cUString, ustring_alloc);
    rb_define_method (rb_cUString, "initialize", VALUEFUNC (ustring_initialize), -1);
    rb_define_method (rb_cUString, "initialize_copy", VALUEFUNC (rb_ustring_replace), 1);
    rb_define_method (rb_cUString, "to_s", VALUEFUNC (ustring_to_s), 0);
    rb_define_method (rb_cUString, "to_str", VALUEFUNC (ustring_to_s), 0);
    rb_define_method (rb_cUString, "inspect", VALUEFUNC (ustring_inspect), 0);
    rb_define_method (rb_cUString, "size", VALUEFUNC (ustring_size), 0);
    rb_define_method (rb_cUString, "length", VALUEFUNC (ustring_size), 0);
    rb_define_method (rb_cUString, "downcase", VALUEFUNC (ustring_downcase), 1);
    rb_define_method (rb_cUString, "upcase", VALUEFUNC (ustring_upcase), 1);
}

