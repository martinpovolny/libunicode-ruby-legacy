/**********************************************************************
 
    Copyright (C) 2005 Jan Becvar
    Copyright (C) 2005 soLNet.cz

**********************************************************************/


/**********************************************************************
            String extensions (UTF-8/UTF-16)                                  
***********************************************************************/

/* TODO
 * rb_str_modify
 */

/* Na +str+ musi byt predem zavolano rb_str_modify! */
UErrorCode
rb_str_replace_uchars (VALUE str, const UChar *uptr, int32_t ulen)
{
    int32_t      dest_len;
    UErrorCode   err = U_ZERO_ERROR;

    if (ruby_u_encoding == RUBY_U_UTF8)
    {
        // UTF8
        if (RSTRING (str)->len < ulen)
            rb_str_resize (str, ulen * 3);

        u_strToUTF8 (RSTRING (str)->ptr,
                     RSTRING (str)->len,
                     &dest_len,
                     uptr,
                     ulen,
                     &err);

        rb_str_resize (str, dest_len);

        if (err == U_BUFFER_OVERFLOW_ERROR)
        {
            err = U_ZERO_ERROR;
            u_strToUTF8 (RSTRING (str)->ptr,
                         dest_len,
                         NULL,
                         uptr,
                         ulen,
                         &err);
        }
    }
    else
    {
        // UTF16
        rb_str_resize (str, ulen * sizeof (UChar));
        memcpy (RSTRING (str)->ptr, uptr, ulen * sizeof (UChar));
    }


    return err;
}

VALUE
rb_str_from_uchars (const UChar *uptr, int32_t ulen, UErrorCode *err)
{
    VALUE result;

    result = rb_str_new (NULL, 0);
    *err = rb_str_replace_uchars (result, uptr, ulen);
    return result;
}

VALUE
rb_str_from_UnicodeString (UnicodeString *uni_str, UErrorCode *err)
{
    if (uni_str == NULL)
        return rb_str_from_uchars (NULL, 0, err);
    else
        return rb_str_from_uchars (uni_str->getBuffer (), uni_str->length (), err);
}

/* 
 * Vraci pocet char32 znaku (code points) v UTF-8/UTF-16 retezci. Korektne
 * pracuje pouze s validnim UTF-8 (s UTF-16 pracuje spravne vzdy) - kazdy
 * nevalidni UTF-8 byte je zapocitan jako jeden nebo zadny char32 znak. Pro
 * korektni pocet znaku v nevalidnim UTF-8 pouzijte metody +u_count(:chars32)+
 * nebo +u_count(:code_points)+, ktere jsou ale nekolikanasobne pomalejsi. 
 *
 */
static VALUE 
rb_str_u_size (VALUE str)
{
    StringValue (str);

    if (ruby_u_encoding == RUBY_U_UTF8)
    {
        // UTF8
        uint8_t  *ptr8, *end8;
        uint32_t *ptr32, *end32, d;
        long      size = 0;

        end8 = (uint8_t *) (RSTRING (str)->ptr + RSTRING (str)->len);
        end32 = (uint32_t *) (end8 - 4);

        for (ptr32 = (uint32_t *) RSTRING (str)->ptr; ptr32 < end32; ptr32++)
        {
            d = *ptr32;
            if (!U8_IS_TRAIL (d))
                size++;
            d = d >> 1;
            if (!U8_IS_TRAIL (d))
                size++;
            d = d >> 1;
            if (!U8_IS_TRAIL (d))
                size++;
            d = d >> 1;
            if (!U8_IS_TRAIL (d))
                size++;
        }

        for (ptr8 = (uint8_t *) ptr32; ptr8 < end8; ptr8++)
            if (!U8_IS_TRAIL (*ptr8))
                size++;

        return INT2NUM (size);
    }
    else
    {
        // UTF16
        return INT2NUM (u_countChar32 ((UChar *) RSTRING (str)->ptr, 
                                       util_check_string_len (str) / 2));
    }
}

/* 
 *  call-seq:
 *     str.u_upcase!(locale = nil) => str
 *
 * Funguje podobne jako +String#upcase!+, ale vzdy vraci self a 
 * a vyhodi vyjimku v pripade vadneho UTF-8 bytu.
 *
 */

static VALUE 
rb_str_u_upcase_bang (int argc, VALUE *argv, VALUE self)
{
    char         src_buf[STACK_BUF_SIZE], dst_buf[STACK_BUF_SIZE];
    int          src_free, dst_free = 0;
    int32_t      src_len,  dst_len = STACK_BUF_SIZE / sizeof (UChar);
    UChar       *src,     *dst = (UChar *) dst_buf;
    VALUE        locale = Qnil;
    const char  *clocale;
    UErrorCode   err = U_ZERO_ERROR;

    rb_scan_args (argc, argv, "01", &locale);
    clocale = util_locale2c (locale);

    rb_str_modify (self);

    src = util_str_to_uchars_try_stack (self, src_buf, &src_len, &src_free);

    if (src_len > dst_len)
    {
        dst_len = src_len;
        dst_free = 1;
        dst = ALLOC_N (UChar, dst_len);
    }
    dst_len = u_strToUpper (dst,
                            dst_len,
                            src,
                            src_len,
                            clocale,
                            &err);

    if (err == U_BUFFER_OVERFLOW_ERROR)
    {
        /* buffer is too small */
        util_realloc_stack_buf (dst, dst_free, UChar, dst_len);
        err = U_ZERO_ERROR;
        dst_len = u_strToUpper (dst, 
                                dst_len,
                                src, 
                                src_len,
                                clocale,
                                &err);
    }

    util_free_stack_buf (src, src_free);

    if (U_SUCCESS (err))
        err = rb_str_replace_uchars (self, dst, dst_len);

    util_free_stack_buf (dst, dst_free);
    util_icu_error (err);
    return self;
}

static VALUE 
rb_str_u_upcase (int argc, VALUE *argv, VALUE self)
{
    return rb_str_u_upcase_bang (argc, argv, rb_str_dup (self));
}

/* 
 *  call-seq:
 *     str.u_downcase!(locale = nil) => str
 *
 * Funguje podobne jako +String#downcase!+, ale vzdy vraci self a 
 * a vyhodi vyjimku v pripade vadneho UTF-8 bytu.
 *
 */

static VALUE 
rb_str_u_downcase_bang (int argc, VALUE *argv, VALUE self)
{
    char         src_buf[STACK_BUF_SIZE], dst_buf[STACK_BUF_SIZE];
    int          src_free, dst_free = 0;
    int32_t      src_len,  dst_len = STACK_BUF_SIZE / sizeof (UChar);
    UChar       *src,     *dst = (UChar *) dst_buf;
    VALUE        locale = Qnil;
    const char  *clocale;
    UErrorCode   err = U_ZERO_ERROR;

    rb_scan_args (argc, argv, "01", &locale);
    clocale = util_locale2c (locale);

    rb_str_modify (self);

    src = util_str_to_uchars_try_stack (self, src_buf, &src_len, &src_free);

    if (src_len > dst_len)
    {
        dst_len = src_len;
        dst_free = 1;
        dst = ALLOC_N (UChar, dst_len);
    }
    dst_len = u_strToLower (dst,
                            dst_len,
                            src,
                            src_len,
                            clocale,
                            &err);

    if (err == U_BUFFER_OVERFLOW_ERROR)
    {
        /* buffer is too small */
        util_realloc_stack_buf (dst, dst_free, UChar, dst_len);
        err = U_ZERO_ERROR;
        dst_len = u_strToLower (dst, 
                                dst_len,
                                src, 
                                src_len,
                                clocale,
                                &err);
    }

    util_free_stack_buf (src, src_free);

    if (U_SUCCESS (err))
        err = rb_str_replace_uchars (self, dst, dst_len);

    util_free_stack_buf (dst, dst_free);
    util_icu_error (err);
    return self;
}

static VALUE 
rb_str_u_downcase (int argc, VALUE *argv, VALUE self)
{
    return rb_str_u_downcase_bang (argc, argv, rb_str_dup (self));
}

/* 
 *  call-seq:
 *     str.u_titlecase!(type = :words, locale = nil) => str
 *
 * Titlecase a string. Casing is locale-dependent and context-sensitive.
 * +Type+ muze byt jeden z nasledujicich symbolu:
 *   * :words - zvetsi prvni pismeno kazdeho slova,
 *   * :sentences - zvetsi prvni pismeno kazde vety
 *   * :string - zvetsi prvni pismeno v retezci. Zadavat locale zrejme nema
 *               smysl.
 *
 * Vyhodi vyjimku v pripade vadneho UTF-8 bytu. 
 *
 */

static VALUE 
rb_str_u_titlecase_bang (int argc, VALUE *argv, VALUE self)
{
    char            src_buf[STACK_BUF_SIZE], dst_buf[STACK_BUF_SIZE];
    int             src_free, dst_free = 0;
    int32_t         src_len,  dst_len = STACK_BUF_SIZE / sizeof (UChar);
    UChar          *src,     *dst = (UChar *) dst_buf;
    VALUE           locale = Qnil, type = ID2SYM (id_words);
    ID              id;
    const char     *clocale;
    UErrorCode      err = U_ZERO_ERROR;
    UBreakIterator *bi;
    

    if (RSTRING (self)->len == 0)
        return self;

    rb_scan_args (argc, argv, "02", &type, &locale);
    clocale = util_locale2c (locale);

    Check_Type (type, T_SYMBOL);
    id = SYM2ID (type);

    if (id == id_string || id == id_words || id == id_sentences)
        bi = util_get_ubrk (id, clocale);
    else
        rb_raise (rb_eArgError, "wrong u_titlecase type");

    rb_str_modify (self);

    src = util_str_to_uchars_try_stack (self, src_buf, &src_len, &src_free);

    ubrk_setText (bi, src, src_len, &err);

    if (U_SUCCESS (err))
    {

        if (src_len > dst_len)
        {
            dst_len = src_len;
            dst_free = 1;
            dst = ALLOC_N (UChar, dst_len);
        }
        
        dst_len = u_strToTitle (dst,
                                dst_len,
                                src,
                                src_len,
                                bi,
                                clocale,
                                &err);

        if (err == U_BUFFER_OVERFLOW_ERROR)
        {
            /* buffer is too small */
            util_realloc_stack_buf (dst, dst_free, UChar, dst_len);
            err = U_ZERO_ERROR;
            dst_len = u_strToTitle (dst, 
                                    dst_len,
                                    src, 
                                    src_len,
                                    bi,
                                    clocale,
                                    &err);
        }
    }

    util_free_stack_buf (src, src_free);

    if (U_SUCCESS (err))
        err = rb_str_replace_uchars (self, dst, dst_len);

    util_free_stack_buf (dst, dst_free);
    util_icu_error (err);
    return self;
}

static VALUE 
rb_str_u_titlecase (int argc, VALUE *argv, VALUE self)
{
    return rb_str_u_titlecase_bang (argc, argv, rb_str_dup (self));
}

/* 
 *  call-seq:
 *     str.u_capitalize!(locale = nil) => str
 *
 * To same jako String#u_titlecase!(:string, locale).
 *
 */

static VALUE 
rb_str_u_capitalize_bang (int argc, VALUE *argv, VALUE self)
{
    VALUE *argv_new;

    argv_new = ALLOCA_N (VALUE, 2);
    argv_new[0] = ID2SYM (id_string);
    argv_new[1] = argc > 0 ? argv[0] : Qnil;

    return rb_str_u_titlecase_bang (2, argv_new, self);
}

static VALUE 
rb_str_u_capitalize (int argc, VALUE *argv, VALUE self)
{
    return rb_str_u_capitalize_bang (argc, argv, rb_str_dup (self));
}

/* 
 *  call-seq:
 *     str.u_split_to(type = :chars, locale = nil) => array
 *
 * Rozdeli retezec na pole retezcu podle zadaneho rozdelovaciho algoritmu.
 * +Type+ muze byt jeden z nasledujicich symbolu:
 *   * :chars32      - vraci code points jako retezce (1 - 4 chars in UTF8, 2 - 4 in UTF16).
 *                     Vadna sekvence bytu (korektni UTF-8
 *                     zacatek az vadny byte) je vracena jako jeden string. 
 *                     Locale se nepouzivaji.
 *   * :code_points  - vraci pole integeru. Vadna sekvence bytu (korektni UTF-8
 *                     zacatek az vadny byte) je vracena jako -1.  Locale se nepouzivaji.
 *   * :chars        - jedna se o tzv. "grapheme clusters", muze se jednat o 
 *                     nekolik code pointu (napr. je-li Á zapsano jako A a carka). 
 *                     Locale se zrejme nepouzivaji.
 *   * :words
 *   * :sentences
 *   * :lines        - lame na mistech, kde lze retezec rozdelit na vice radku.
 *
 * _Nevyhazuje_ vyjimku v pripade vadneho UTF-8 bytu.
 *
 */
static VALUE 
rb_str_u_split_to (int argc, VALUE *argv, VALUE self)
{
    VALUE           locale = Qnil, type = ID2SYM (id_chars);
    ID              id;
    VALUE           result;
    int32_t         self_len;
    char           *self_ptr;
    
    result = rb_ary_new ();

    self_len = util_check_string_len (self);
    self_ptr = RSTRING (self)->ptr;

    if (self_len == 0)
        return result;

    rb_scan_args (argc, argv, "02", &type, &locale);

    Check_Type (type, T_SYMBOL);
    id = SYM2ID (type);

    if (id == id_chars32)
    {
        VALUE       s;
        int32_t     pos = 0, old_pos;

        if (ruby_u_encoding == RUBY_U_UTF16)
        {
            self_len /= 2;

            while (pos < self_len)
            {
                old_pos = pos;
                U16_FWD_1 ((UChar *) self_ptr, pos, self_len);

                s = rb_str_new5 (self, 
                                (char *) (((UChar *) self_ptr + old_pos)), 
                                (pos - old_pos) * 2);
                OBJ_INFECT (s, self);
                rb_ary_push (result, s);
            }
        }
        else
        {
            while (pos < self_len)
            {
                old_pos = pos;
                U8_FWD_1 (self_ptr, pos, self_len);

                s = rb_str_new5 (self, self_ptr + old_pos, pos - old_pos);
                OBJ_INFECT (s, self);
                rb_ary_push (result, s);
            }
        }
    }
    else if (id == id_code_points)
    {
        UChar32     c;
        int32_t     pos = 0;

        if (ruby_u_encoding == RUBY_U_UTF16)
        {
            self_len /= 2;

            while (pos < self_len)
            {
                U16_NEXT ((UChar *) self_ptr, pos, self_len, c);
                rb_ary_push (result, INT2NUM (c));
            }
        }
        else
        {
            while (pos < self_len)
            {
                U8_NEXT (self_ptr, pos, self_len, c);
                rb_ary_push (result, INT2NUM (c));
            }
        }
    }
    else
    {
        VALUE   str;
        UBreakIterator *bi = util_get_ubrk_for_str (self, id, util_locale2c (locale));
        int32_t start = ubrk_first (bi), end = ubrk_next (bi);

        for (; end != UBRK_DONE; start = end, end = ubrk_next (bi))
        {
            if (ruby_u_encoding == RUBY_U_UTF16)
                str = rb_str_new5 (self, 
                                   (char *) (((UChar *) self_ptr) + start),
                                   (end - start) * 2);
            else
                str = rb_str_new5 (self, self_ptr + start, end - start);

            OBJ_INFECT (str, self);
            rb_ary_push (result, str);
        }
    }

    return result;
}

/* 
 *  call-seq:
 *     str.u_count(type = :chars, locale = nil) => integer
 *
 * To same jako str.u_split_to(type, locale).size, ale rychlejsi.
 *
 */
static VALUE 
rb_str_u_count (int argc, VALUE *argv, VALUE self)
{
    VALUE           locale = Qnil, type = ID2SYM (id_chars);
    ID              id;
    int32_t         result = 0;
    int32_t         self_len;
    
    self_len = util_check_string_len (self);

    if (self_len == 0)
        return INT2FIX (0);

    rb_scan_args (argc, argv, "02", &type, &locale);

    Check_Type (type, T_SYMBOL);
    id = SYM2ID (type);

    if (id == id_chars32 || id == id_code_points)
    {
        if (ruby_u_encoding == RUBY_U_UTF16)
        {
            result = u_countChar32 ((UChar *) RSTRING (self)->ptr, self_len / 2);
        }
        else
        {
            for (int32_t pos = 0; pos < self_len; )
            {
                U8_FWD_1 (RSTRING (self)->ptr, pos, self_len);
                result++;
            }
        }
    }
    else
    {
        UBreakIterator *bi = util_get_ubrk_for_str (self, id, util_locale2c (locale));

        ubrk_first (bi);

        for (int32_t end = ubrk_next (bi); end != UBRK_DONE; end = ubrk_next (bi))
            result++;
    }

    return INT2NUM (result);
}

static inline void 
str_mod_check (VALUE s, char *p, long len)
{
    if (RSTRING(s)->ptr != p || RSTRING(s)->len != len)
        rb_raise (rb_eRuntimeError, "string modified");
}


static VALUE 
rb_str_u_each (int argc, VALUE *argv, VALUE self)
{
    VALUE           locale = Qnil, type = ID2SYM (id_chars);
    ID              id;
    int32_t         self_len;
    char           *self_ptr;
    VALUE result;
    
    self_len = util_check_string_len (self);
    self_ptr = RSTRING (self)->ptr;

    if (self_len == 0)
        return self;

    rb_scan_args (argc, argv, "02", &type, &locale);

    Check_Type (type, T_SYMBOL);
    id = SYM2ID (type);

    if (id == id_chars32)
    {
        VALUE       s;
        int32_t     pos = 0, old_pos;

        if (ruby_u_encoding == RUBY_U_UTF16)
        {
            self_len /= 2;

            while (pos < self_len)
            {
                old_pos = pos;
                U16_FWD_1 ((UChar *) self_ptr, pos, self_len);

                s = rb_str_new5 (self,
                                (char *) (((UChar *) self_ptr + old_pos)),
                                (pos - old_pos) * 2);
                OBJ_INFECT (s, self);
                rb_ary_push (result, s);
            }
        }
        else
        {
            while (pos < self_len)
            {
                old_pos = pos;
                U8_FWD_1 (self_ptr, pos, self_len);

                s = rb_str_new5 (self, self_ptr + old_pos, pos - old_pos);
                OBJ_INFECT (s, self);
                rb_ary_push (result, s);
            }
        }
    }
    else if (id == id_code_points)
    {
        UChar32     c;
        int32_t     pos = 0;

        if (ruby_u_encoding == RUBY_U_UTF16)
        {
            int32_t len = self_len / 2;

            while (pos < len)
            {
                U16_NEXT ((UChar *) self_ptr, pos, len, c);
                rb_yield (INT2NUM (c));
                str_mod_check (self, self_ptr, self_len);
                rb_ary_push (result, INT2NUM (c));
            }
        }
        else
        {
            while (pos < self_len)
            {
                U8_NEXT (self_ptr, pos, self_len, c);
                rb_ary_push (result, INT2NUM (c));
            }
        }
    }
    else
    {
        VALUE   str;
        UBreakIterator *bi = util_get_ubrk_for_str (self, id, util_locale2c (locale));
        int32_t start = ubrk_first (bi), end = ubrk_next (bi);

        for (; end != UBRK_DONE; start = end, end = ubrk_next (bi))
        {
            if (ruby_u_encoding == RUBY_U_UTF16)
                str = rb_str_new5 (self, 
                                   (char *) (((UChar *) self_ptr) + start),
                                   (end - start) * 2);
            else
                str = rb_str_new5 (self, self_ptr + start, end - start);

            OBJ_INFECT (str, self);
            rb_ary_push (result, str);
        }
    }

    return INT2NUM (result);
//    for (s = p, p += rslen; p < pend; p++)
//    {
//        if (rslen == 0 && *p == '\n') {
//            if (*++p != '\n') continue;
//            while (*p == '\n') p++;
//        }
//        if (RSTRING(str)->ptr < p && p[-1] == newline &&
//            (rslen <= 1 ||
//             rb_memcmp(RSTRING(rs)->ptr, p-rslen, rslen) == 0)) {
//            line = rb_str_new5(str, s, p - s);
//            OBJ_INFECT(line, str);
//            rb_yield(line);
//            str_mod_check(str, ptr, len);
//            s = p;
//        }
//    }
//
//    if (s != pend) {
//        if (p > pend) p = pend;
//        line = rb_str_new5(str, s, p - s);
//        OBJ_INFECT(line, str);
//        rb_yield(line);
//    }
//
//    return self;
}


/************************************************************************
 *           INIT                                                       *
 ************************************************************************/

static void
string_init ()
{
    rb_define_method (rb_cString, "u_size", VALUEFUNC (rb_str_u_size), 0);
    rb_define_method (rb_cString, "u_length", VALUEFUNC (rb_str_u_size), 0);

    rb_define_method (rb_cString, "u_upcase", VALUEFUNC (rb_str_u_upcase), -1);
    rb_define_method (rb_cString, "u_downcase", VALUEFUNC (rb_str_u_downcase), -1);
    rb_define_method (rb_cString, "u_capitalize", VALUEFUNC (rb_str_u_capitalize), -1);
    rb_define_method (rb_cString, "u_titlecase", VALUEFUNC (rb_str_u_titlecase), -1);
    rb_define_method (rb_cString, "u_split_to", VALUEFUNC (rb_str_u_split_to), -1);
    rb_define_method (rb_cString, "u_count", VALUEFUNC (rb_str_u_count), -1);

    rb_define_method (rb_cString, "u_upcase!", VALUEFUNC (rb_str_u_upcase_bang), -1);
    rb_define_method (rb_cString, "u_downcase!", VALUEFUNC (rb_str_u_downcase_bang), -1);
    rb_define_method (rb_cString, "u_capitalize!", VALUEFUNC (rb_str_u_capitalize_bang), -1);
    rb_define_method (rb_cString, "u_titlecase!", VALUEFUNC (rb_str_u_titlecase_bang), -1);
}

