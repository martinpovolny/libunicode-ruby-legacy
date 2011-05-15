/**********************************************************************
 
    Copyright (C) 2005 Jan Becvar
    Copyright (C) 2005 soLNet.cz

**********************************************************************/

/************************************************************************
 *           UCollator                                                  *
 ************************************************************************/

/* TODO
 *
 * static void
 * time_modify(time)
 * VALUE time;
 * {
 *         rb_check_frozen(time);
 *         if (!OBJ_TAINTED(time) && rb_safe_level() >= 4)
 *            rb_raise(rb_eSecurityError, "Insecure: can't modify
 *            Time");
 *  }
 *
 * na rb_ zacinaji metody?
 *
 */

static void 
ucollator_free (UCollator *col)
{
    if (col != NULL)
        ucol_close (col);
}

#undef  IS_UCOLLATOR
#define IS_UCOLLATOR(v) (TYPE (v) == T_DATA && RDATA (v)->dfree == (void (*)(void*))ucollator_free)

int
rb_is_ucollator (VALUE c)
{
    return IS_UCOLLATOR (c) ? 1 : 0;
}

UCollator *
rb_ucollator_object (volatile VALUE *ptr)
{
    VALUE v = *ptr;

    if (!IS_UCOLLATOR (v))
        rb_raise (rb_eTypeError, "wrong argument type (expected UCollator)");

    if (DATA_PTR (v) == NULL)
    {
        UErrorCode err = U_ZERO_ERROR;
        UCollator *ucol = ucol_open (NULL, &err);
        util_icu_error (err);
        DATA_PTR (v) = ucol;
    }

    return ((UCollator *) DATA_PTR (v));
}

static VALUE 
ucollator_alloc (VALUE klass)
{
    return Data_Wrap_Struct (klass, 
                             0, 
                             ucollator_free, 
                             NULL);
}

static void
ucollator_set_locale (VALUE self, const char *locale)
{
    UErrorCode err = U_ZERO_ERROR;
    UCollator *col = (UCollator *) DATA_PTR (self);

    CHECK_FROZEN (self);

    if (col != NULL)
    {
        if (locale != NULL)
        {
            const char *alocale = ucol_getLocaleByType (col, ULOC_REQUESTED_LOCALE, &err);
            util_icu_error (err);

            if (alocale != NULL && strcmp (locale, alocale) == 0)
                return;
        }

        ucol_close (col);
    }

    col = ucol_open (locale, &err);
    util_icu_error (err);

    DATA_PTR (self) = col;
}

VALUE 
rb_ucollator_new (const char *locale)
{
    VALUE col;

    col = ucollator_alloc (rb_cUCollator);
    ucollator_set_locale (col, locale);
    return col;
}

static VALUE 
ucollator_initialize (int argc, VALUE *argv, VALUE self)
{
    VALUE  locale;
    if (rb_scan_args (argc, argv, "01", &locale) > 0)
        ucollator_set_locale (self, util_locale2c (locale));
    return Qnil;
}

static VALUE 
ucollator_init_copy (VALUE self, VALUE orig)
{
    UErrorCode err = U_ZERO_ERROR;
    const char *locale;

    if (self == orig) 
        return self;

    CHECK_FROZEN (self);

    locale = ucol_getLocaleByType (UCollatorObject (orig),
                                   ULOC_REQUESTED_LOCALE, &err);
    util_icu_error (err);

    ucollator_set_locale (self, locale);
    return self;
}

static VALUE 
ucollator_set_locale_r (VALUE self, VALUE locale)
{
    ucollator_set_locale (self, util_locale2c (locale));
    return locale;
}

static VALUE 
ucollator_get_locale (VALUE self)
{
    UErrorCode err = U_ZERO_ERROR;
    const char *locale;
    
    locale = ucol_getLocaleByType (UCollatorObject (self),
                                   ULOC_REQUESTED_LOCALE, 
                                   &err);
    util_icu_error (err);

    return locale == NULL ? Qnil : ID2SYM (rb_intern (locale));
}

static VALUE 
ucollator_compare (VALUE self, VALUE str1, VALUE str2)
{
    UCollationResult colresult;
    UCollator       *ucol = UCollatorObject (self);

    if (IS_USTRING (str1) && IS_USTRING (str2))
    {
        /* utf-16 collation */
        RUString *ustr1 = UStringStruct (str1);
        RUString *ustr2 = UStringStruct (str2);

        if (ustr1->len == 0)
            return INT2FIX (ustr2->len == 0 ? 0 : -1);
        if (ustr2->len == 0)
            return INT2FIX (1);

        colresult = ucol_strcoll (ucol, 
                                  ustr1->ptr, 
                                  ustr1->len, 
                                  ustr2->ptr, 
                                  ustr2->len);
    }
    else 
    {
        /* utf-8 collation */
        UCharIterator iter1, iter2;
        UErrorCode err = U_ZERO_ERROR;

        StringValue (str1);
        StringValue (str2);

        uiter_setUTF8 (&iter1, RSTRING (str1)->ptr, RSTRING (str1)->len);
        uiter_setUTF8 (&iter2, RSTRING (str2)->ptr, RSTRING (str2)->len);

        colresult = ucol_strcollIter (ucol, &iter1, &iter2, &err);
        util_icu_error (err);
    }

    switch (colresult)
    {
        case UCOL_LESS:
            return INT2FIX (-1);
        case UCOL_GREATER:
            return INT2FIX (1);
        default:
            return INT2FIX (0);
    }
}

/************************************************************************
 *           INIT                                                       *
 ************************************************************************/

static void
ucollator_init ()
{
    rb_cUCollator = rb_define_class ("UCollator", rb_cObject);
    ADD_INIT_METHODS (rb_cUCollator, ucollator_alloc);
    rb_define_method (rb_cUCollator, "initialize", VALUEFUNC (ucollator_initialize), -1);
    rb_define_method (rb_cUCollator, "initialize_copy", VALUEFUNC (ucollator_init_copy), 1);
    rb_define_method (rb_cUCollator, "locale=", VALUEFUNC (ucollator_set_locale_r), 1);
    rb_define_method (rb_cUCollator, "locale", VALUEFUNC (ucollator_get_locale), 0);
    rb_define_method (rb_cUCollator, "compare", VALUEFUNC (ucollator_compare), 2);
    rb_define_method (rb_cUCollator, "cmp", VALUEFUNC (ucollator_compare), 2);
}

