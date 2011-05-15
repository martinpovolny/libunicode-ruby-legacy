/**********************************************************************
 
    Copyright (C) 2005 Jan Becvar
    Copyright (C) 2005 soLNet.cz

**********************************************************************/

/************************************************************************
 *           UTime                                                      *
 ************************************************************************/

ENUM_CREATE_FUNCS (UCalendarDateFields, date_field);
ENUM_CREATE_FUNCS (DateFormat::EStyle, date_format_style);

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
 *  jak se uvolnuji objekty na stacku v pripade raise?
 *
 */

static void 
utime_free (Calendar *date)
{
    if (date)
        delete date;
}

#undef  IS_UTIME
#define IS_UTIME(v) (TYPE (v) == T_DATA && RDATA (v)->dfree == (void (*)(void*))utime_free)

int
rb_is_utime (VALUE v)
{
    return IS_UTIME (v) ? 1 : 0;
}

Calendar *
rb_utime_object (volatile VALUE *ptr)
{
    VALUE v = *ptr;

    if (!IS_UTIME (v))
        rb_raise (rb_eTypeError, "wrong argument type (expected UTime)");

    if (DATA_PTR (v) == NULL)
    {
        UErrorCode  err = U_ZERO_ERROR;
        Calendar   *cal = Calendar::createInstance (err);
        util_icu_error_with_delete (err, cal);
        DATA_PTR (v) = cal;
    }

    return ((Calendar *) DATA_PTR (v));
}

static VALUE 
utime_alloc (VALUE klass)
{
    return Data_Wrap_Struct (klass, 
                             0, 
                             utime_free, 
                             NULL);
}

VALUE 
rb_utime_new (const char *zone, const char *locale)
{
    UErrorCode   err = U_ZERO_ERROR;
    Calendar    *cal;

    cal = Calendar::createInstance (util_create_timezone (zone),
                                    util_create_locale (locale),
                                    err);
    util_icu_error_with_delete (err, cal);

    return Data_Wrap_Struct (rb_cUTime, 
                             0, 
                             utime_free, 
                             cal);
}

VALUE 
rb_utime_new_from_object (Calendar *cal)
{
    return Data_Wrap_Struct (rb_cUTime, 
                             0, 
                             utime_free, 
                             cal);
}

static VALUE 
utime_init (int argc, VALUE *argv, VALUE self)
{
    VALUE          zone = Qnil;
    VALUE          locale = Qnil;
    struct timeval tv;
    UErrorCode     err = U_ZERO_ERROR;
    Calendar      *cal = (Calendar *) DATA_PTR (self);

    CHECK_FROZEN (self);

    rb_scan_args (argc, argv, "02", &zone, &locale);

    if (cal != NULL)
    {
        delete cal;
        DATA_PTR (self) = NULL;
    }

    cal = Calendar::createInstance (util_create_timezone (util_zone2c (zone)),
                                    util_create_locale (util_locale2c (locale)),
                                    err);

    util_icu_error_with_delete (err, cal);

    DATA_PTR (self) = cal;

    /* Calendar::createInstance doesn't set miliseconds */
    if (gettimeofday (&tv, 0) < 0)
        rb_sys_fail ("gettimeofday");

    cal->setTime (((double) tv.tv_sec) * 1000 + ((double) tv.tv_usec) / 1000, err);
    util_icu_error (err);
        
    return Qnil;
}

static VALUE 
utime_now (int argc, VALUE *argv, VALUE klass)
{
    VALUE obj = utime_alloc (klass);
    rb_obj_call_init (obj, argc, argv);
    return obj;
}

static VALUE 
utime_at (int argc, VALUE *argv, VALUE klass)
{
    VALUE       time;
    VALUE       zone = Qnil;
    VALUE       locale = Qnil;
    UDate       date;
    UErrorCode  err = U_ZERO_ERROR;
    Calendar    *calendar;

    rb_scan_args (argc, argv, "12", &time, &zone, &locale);
    date = NUM2DBL (time);

    calendar = Calendar::createInstance (util_create_timezone (util_zone2c (zone)),
                                         util_create_locale (util_locale2c (locale)),
                                         err);
    util_icu_error_with_delete (err, calendar);

    calendar->setTime (date * 1000, err);
    util_icu_error_with_delete (err, calendar);
        
    return Data_Wrap_Struct (klass, 
                             0, 
                             utime_free, 
                             calendar);
}

static VALUE 
utime_init_copy (VALUE self, VALUE orig)
{
    Calendar    *cal_self = (Calendar *) DATA_PTR (self);
    Calendar    *cal_orig = UTimeObject (orig);

    if (self == orig) 
        return self;

    CHECK_FROZEN (self);

    if (cal_self != NULL)
        delete cal_self;

    DATA_PTR (self) = cal_orig->clone ();

    return self;
}

static VALUE 
utime_get_type (VALUE self)
{
    return ID2SYM (rb_intern (UTimeObject (self)->getType ()));
}

static VALUE 
utime_get_zone (VALUE self)
{
    UnicodeString zone_str;

    UTimeObject (self)->getTimeZone ().getID (zone_str);

    return rb_ustring_from_UnicodeString (&zone_str);
}

static VALUE
utime_set_zone (VALUE self, VALUE zone)
{
    UTimeObject (self)->adoptTimeZone (util_create_timezone (util_zone2c (zone)));
    return zone;
}

static VALUE 
utime_get_locale (VALUE self)
{
    UErrorCode    err = U_ZERO_ERROR;
    Locale        locale;

    locale = UTimeObject (self)->getLocale (ULOC_VALID_LOCALE , err);
    util_icu_error (err);
    
    return rb_str_new2 (locale.getBaseName ());
}

static VALUE
utime_get_field_1 (VALUE self, UCalendarDateFields field)
{
    int32_t     result;
    UErrorCode  err = U_ZERO_ERROR;

    result = UTimeObject (self)->get (field, err);
    util_icu_error (err);

    return INT2NUM (result);
}

static void
utime_set_field_1 (VALUE self, UCalendarDateFields field, VALUE value)
{
    CHECK_FROZEN (self);
    UTimeObject (self)->set (field, NUM2INT (value));
}

#define ENUM_ADD_DATE_FIELD(symbol, type_item) { \
    ID id = rb_intern (#symbol); \
    st_add_direct (tbl_id2 ## date_field, id, type_item); \
    st_add_direct (tbl_ ## date_field ## 2id, type_item, id); \
    rb_define_method (rb_cUTime, #symbol, VALUEFUNC (utime_get_ ## symbol), 0); \
    rb_define_method (rb_cUTime, #symbol "=", VALUEFUNC (utime_set_ ## symbol), 1); \
}

#define UTIME_ADD_GET_SET(symbol, field) \
static VALUE \
utime_get_ ## symbol (VALUE self) \
{ \
    return utime_get_field_1 (self, field); \
} \
\
static VALUE \
utime_set_ ## symbol (VALUE self, VALUE v) \
{ \
    utime_set_field_1 (self, field, v); \
    return v; \
}

UTIME_ADD_GET_SET (era,                  UCAL_ERA);                  // Example: 0..1
UTIME_ADD_GET_SET (year,                 UCAL_YEAR);                 // Example: 1..big number
UTIME_ADD_GET_SET (yweek,                UCAL_WEEK_OF_YEAR);         // Example: 1..53
UTIME_ADD_GET_SET (mweek,                UCAL_WEEK_OF_MONTH);        // Example: 1..4
UTIME_ADD_GET_SET (mday,                 UCAL_DATE);                 // Example: 1..31
UTIME_ADD_GET_SET (yday,                 UCAL_DAY_OF_YEAR);          // Example: 1..365
UTIME_ADD_GET_SET (wday_in_month,        UCAL_DAY_OF_WEEK_IN_MONTH); // Example: 1..4, may be specified as -1
UTIME_ADD_GET_SET (am_pm,                UCAL_AM_PM);                // Example: 0..1
UTIME_ADD_GET_SET (hour12,               UCAL_HOUR);                 // Example: 0..11
UTIME_ADD_GET_SET (hour,                 UCAL_HOUR_OF_DAY);          // Example: 0..23
UTIME_ADD_GET_SET (min,                  UCAL_MINUTE);               // Example: 0..59
UTIME_ADD_GET_SET (sec,                  UCAL_SECOND);               // Example: 0..59
UTIME_ADD_GET_SET (msec,                 UCAL_MILLISECOND);          // Example: 0..999
UTIME_ADD_GET_SET (zone_offset,          UCAL_ZONE_OFFSET);          // Example: -12*U_MILLIS_PER_HOUR..12*U_MILLIS_PER_HOUR
UTIME_ADD_GET_SET (dst_offset,           UCAL_DST_OFFSET);           // Example: 0 or U_MILLIS_PER_HOUR
UTIME_ADD_GET_SET (year_woy,             UCAL_YEAR_WOY);             // Example: 1..big number - Year of Week of Year
UTIME_ADD_GET_SET (wday_local,           UCAL_DOW_LOCAL);            // Example: 1..7 - Day of Week / Localized


/* in icu wday has value 1..7 */
static VALUE
utime_get_wday (VALUE self)
{
    int32_t     result;
    UErrorCode  err = U_ZERO_ERROR;

    result = UTimeObject (self)->get (UCAL_DAY_OF_WEEK, err);
    util_icu_error (err);

    return INT2NUM (result - 1);
}

static VALUE
utime_set_wday (VALUE self, VALUE value)
{
    CHECK_FROZEN (self);
    UTimeObject (self)->set (UCAL_DAY_OF_WEEK, NUM2INT (value) + 1);
    return value;
}

/* in icu month has value 0..11 */
static VALUE
utime_get_month (VALUE self)
{
    int32_t     result;
    UErrorCode  err = U_ZERO_ERROR;

    result = UTimeObject (self)->get (UCAL_MONTH, err);
    util_icu_error (err);

    return INT2NUM (result + 1);
}

static VALUE
utime_set_month (VALUE self, VALUE value)
{
    CHECK_FROZEN (self);
    UTimeObject (self)->set (UCAL_MONTH, NUM2INT (value) - 1);
    return value;
}

/* Recalculate the current time field values if the time value has been changed
 * by a call to setTime(). Return zero for unset fields if any fields have been
 * explicitly set by a call to set(). To force a recomputation of all fields
 * regardless of the previous state, call complete(). This method is
 * semantically const, but may alter the object in memory. */

static VALUE 
utime_get_field (VALUE self, VALUE field)
{
    int32_t     result;
    UErrorCode  err = U_ZERO_ERROR;
    ID          id;

    Check_Type (field, T_SYMBOL);

    id = SYM2ID (field);
    
    if (id == id_month)
        return utime_get_month (self);
    if (id == id_wday)
        return utime_get_wday (self);

    result = UTimeObject (self)->get (enum_id2date_field (id), err);
    util_icu_error (err);

    return INT2NUM (result);
}

static VALUE 
utime_set_field (VALUE self, VALUE field, VALUE value)
{
    ID          id;

    CHECK_FROZEN (self);
    Check_Type (field, T_SYMBOL);

    id = SYM2ID (field);
    
    if (id == id_month)
        return utime_set_month (self, value);
    if (id == id_wday)
        return utime_set_wday (self, value);

    UTimeObject (self)->set (enum_id2date_field (id), NUM2INT (value));
    return value;
}

static VALUE 
utime_add (VALUE self, VALUE field, VALUE value)
{
    ID          id;
    VALUE       dup;
    UErrorCode  err = U_ZERO_ERROR;

    CHECK_FROZEN (self);
    Check_Type (field, T_SYMBOL);

    id = SYM2ID (field);
    
    dup = utime_alloc (rb_cUTime);
    utime_init_copy (dup, self);

    UTimeObject (dup)->add (enum_id2date_field (id), NUM2INT (value), err);
    util_icu_error (err);

    return dup;
}

static VALUE 
utime_add_e (VALUE self, VALUE field, VALUE value)
{
    ID          id;
    UErrorCode  err = U_ZERO_ERROR;

    CHECK_FROZEN (self);
    Check_Type (field, T_SYMBOL);

    id = SYM2ID (field);
    
    UTimeObject (self)->add (enum_id2date_field (id), NUM2INT (value), err);
    util_icu_error (err);

    return self;
}

static VALUE 
utime_to_s (VALUE self)
{
    UnicodeString  str;
    FieldPosition  pos(0);
    VALUE          result;
    UErrorCode     err = U_ZERO_ERROR;

    UTimeFormatObject (date_default_format)->format (*UTimeObject (self), str, pos);
    result = rb_str_from_UnicodeString (&str, &err);
    util_icu_error (err);

    return result;
}

static VALUE 
utime_to_f (VALUE self)
{
    UDate      d;
    UErrorCode err = U_ZERO_ERROR;

    d = UTimeObject (self)->getTime (err);
    util_icu_error (err);

    return rb_float_new (d / 1000.0);
}

static VALUE 
utime_to_i (VALUE self)
{
    long int   d;
    UErrorCode err = U_ZERO_ERROR;

    d = (long int) (UTimeObject (self)->getTime (err) / 1000.0);
    util_icu_error (err);

    return INT2NUM (d);
}

static VALUE 
utime_to_time (VALUE self)
{
    UDate      d;
    UErrorCode err = U_ZERO_ERROR;
    double     div, mod;

    d = UTimeObject (self)->getTime (err);
    util_icu_error (err);

    util_float_divmod (d, 1000.0, &div, &mod);
    return rb_time_new ((time_t) div, (time_t) (mod * 1000.0));
}

static VALUE 
utime_cmp (VALUE self, VALUE other)
{
    double a, b;
    UErrorCode err = U_ZERO_ERROR;

    if (self == other)
        return INT2FIX (0);

    a = UTimeObject (self)->getTime (err);
    util_icu_error (err);
    a /= 1000.0;

    if (IS_UTIME (other))
    {
        b = UTimeObject (other)->getTime (err);
        util_icu_error (err);
        b /= 1000.0;
    }
    else
        b = NUM2DBL (other);

    if (a == b) 
        return INT2FIX (0);
    if (a > b) 
        return INT2FIX (1);
    if (a < b) 
        return INT2FIX (-1);

    rb_raise (rb_eFloatDomainError, "comparing NaN");
}

static VALUE 
utime_equal (VALUE self, VALUE other)
{
    double     d;
    UErrorCode err = U_ZERO_ERROR;

    if (self == other)
        return Qtrue;

    if (IS_UTIME (other))
        return UTimeObject (self)->equals (*UTimeObject (other), err) && U_SUCCESS (err) ? Qtrue : Qfalse;
    
    d = UTimeObject (self)->getTime (err);

    if (U_FAILURE (err))
        return Qfalse;

    return (NUM2DBL (other) == d / 1000.0) ? Qtrue : Qfalse;
}

static VALUE 
utime_eql (VALUE self, VALUE other)
{
    if (self == other)
        return Qtrue;

    if (!IS_UTIME (other))
        return Qfalse;

    return *UTimeObject (self) == *UTimeObject (other) ? Qtrue : Qfalse;
}

static VALUE 
utime_hash (VALUE self)
{
    double     d;
    char      *c;
    unsigned int i;
    int        hash;
    UErrorCode err = U_ZERO_ERROR;

    d = UTimeObject (self)->getTime (err);

    if (U_FAILURE (err))
        return rb_obj_id (self);
        
    if (d == 0)
        d = fabs (d);
    c = (char*) &d;

    for (hash = 0, i = 0; i < sizeof (double); i++)
        hash += c[i] * 971;

    if (hash < 0)
        hash = -hash; 

    return INT2FIX (hash);
}

static VALUE 
utime_plus (VALUE self, VALUE other)
{
    double     d, v;
    VALUE      dup;
    Calendar  *cal;
    UErrorCode err = U_ZERO_ERROR;

    if (IS_UTIME (other))
        rb_raise (rb_eTypeError, "utime + utime?");

    if (rb_obj_class (other) == rb_cTime)
        rb_raise (rb_eTypeError, "utime + time?");

    v = NUM2DBL (other);

    dup = utime_alloc (rb_cUTime);
    utime_init_copy (dup, self);

    cal = UTimeObject (dup);
    d = cal->getTime (err);
    util_icu_error (err);

    cal->setTime (d + v * 1000, err);
    util_icu_error (err);

    return dup;
}

static VALUE 
utime_minus (VALUE self, VALUE other)
{
    double     d, v;
    VALUE      dup;
    Calendar  *cal;
    UErrorCode err = U_ZERO_ERROR;

    if (IS_UTIME (other))
    {
        double d1, d2; 

        d1 = UTimeObject (self)->getTime (err);
        util_icu_error (err);

        d2 = UTimeObject (other)->getTime (err);
        util_icu_error (err);

        return rb_float_new ((d2 - d1) / 1000.0);
    }

    if (rb_obj_class (other) == rb_cTime)
    {
        d = UTimeObject (self)->getTime (err);
        util_icu_error (err);

        return rb_float_new (NUM2DBL (other) - d / 1000.0);
    }

    v = NUM2DBL (other);

    dup = utime_alloc (rb_cUTime);
    utime_init_copy (dup, self);

    cal = UTimeObject (dup);
    d = cal->getTime (err);
    util_icu_error (err);

    cal->setTime (d - v * 1000, err);
    util_icu_error (err);

    return dup;
}

static VALUE 
utime_succ (VALUE self, VALUE other)
{
    double     d;
    VALUE      dup;
    Calendar  *cal;
    UErrorCode err = U_ZERO_ERROR;

    dup = utime_alloc (rb_cUTime);
    utime_init_copy (dup, self);

    cal = UTimeObject (dup);
    d = cal->getTime (err);
    util_icu_error (err);

    cal->setTime (d + 1, err);
    util_icu_error (err);

    return dup;
}

static VALUE 
utime_get_lenient (VALUE self)
{
    return UTimeObject (self)->isLenient () ? Qtrue : Qfalse;
}

static VALUE 
utime_set_lenient (VALUE self, VALUE value)
{
    UTimeObject (self)->setLenient (RTEST (value));
    return value;
}

/************************************************************************
 *           UTimeFormat                                                *
 ************************************************************************/

static void 
utime_format_free (SimpleDateFormat *date_format)
{
    if (date_format)
        delete date_format;
}

#undef  IS_UTIME_FORMAT
#define IS_UTIME_FORMAT(v) (TYPE (v) == T_DATA && RDATA (v)->dfree == (void (*)(void*))utime_format_free)

int
rb_is_utime_format (VALUE v)
{
    return IS_UTIME_FORMAT (v) ? 1 : 0;
}

SimpleDateFormat *
rb_utime_format_object (volatile VALUE *ptr)
{
    VALUE v = *ptr;

    if (!IS_UTIME_FORMAT (v))
        rb_raise (rb_eTypeError, "wrong argument type (expected UTimeFormat)");

    if (DATA_PTR (v) == NULL)
    {
        DateFormat *df = DateFormat::createInstance ();
        DATA_PTR (v) = df;
        df->setLenient (0);
    }

    return ((SimpleDateFormat *) DATA_PTR (v));
}

static VALUE 
utime_format_alloc (VALUE klass)
{
    return Data_Wrap_Struct (klass, 
                             0, 
                             utime_format_free, 
                             NULL);
}

static void
utime_format_set_locale (VALUE self, const char *locale)
{
    rb_ivar_set (self, id_locale, locale == NULL ? Qnil : ID2SYM (rb_intern (locale)));
}

VALUE 
rb_utime_format_new (ID date_style, ID time_style, const char *zone, const char *locale)
{
    DateFormat  *df;
    VALUE       v;

    df = DateFormat::createDateTimeInstance (enum_id2date_format_style (date_style),
                                             enum_id2date_format_style (time_style),
                                             util_create_locale (locale));

    v = Data_Wrap_Struct (rb_cUTimeFormat, 
                          0, 
                          utime_format_free, 
                          df);
    df->setLenient (0);

    if (zone != NULL)
        df->adoptTimeZone (util_create_timezone (zone));

    utime_format_set_locale (v, locale);

    return v;
}

/* TODO:
VALUE 
rb_utime_format_new1 (const char *pattern, const char *locale)
{
}*/

static VALUE
utime_format_set_zone (VALUE self, VALUE zone)
{
    SimpleDateFormat *df = UTimeFormatObject (self);
    
    CHECK_FROZEN (self);

    df->adoptTimeZone (util_create_timezone (util_zone2c (zone)));

    return zone;
}

static VALUE
utime_format_get_zone (VALUE self)
{
    UnicodeString zone_str;
    SimpleDateFormat *df = UTimeFormatObject (self);

    df->getTimeZone ().getID (zone_str);

    return rb_ustring_from_UnicodeString (&zone_str);
}

static VALUE
utime_format_get_locale (VALUE self)
{
    return rb_ivar_get (self, id_locale);
}

static VALUE 
utime_format_init (int argc, VALUE *argv, VALUE self)
{
    VALUE       time_style;
    VALUE       date_style;
    VALUE       locale = Qnil;
    VALUE       zone   = Qnil;
    int         count;
    const char *clocale;
    DateFormat *df = (DateFormat *) DATA_PTR (self);

    CHECK_FROZEN (self);

    count = rb_scan_args (argc, argv, "04", &date_style, &time_style, &zone, &locale);

    if (count < 1)
        date_style = ID2SYM (id_default);
    else
        Check_Type (date_style, T_SYMBOL);

    if (count < 2)
        time_style = ID2SYM (id_default);
    else
        Check_Type (time_style, T_SYMBOL);

    if (df != NULL)
    {
        delete df;
        DATA_PTR (self) = NULL;
    }

    clocale = util_locale2c (locale);

    df = DateFormat::createDateTimeInstance (enum_id2date_format_style (SYM2ID (date_style)),
                                             enum_id2date_format_style (SYM2ID (time_style)),
                                             util_create_locale (clocale));

    DATA_PTR (self) = df;

    df->setLenient (0);

    utime_format_set_locale (self, clocale);

    if (!NIL_P (zone))
        utime_format_set_zone (self, zone);

    return Qnil;
}

static VALUE 
utime_format_init_copy (VALUE self, VALUE orig)
{
    SimpleDateFormat *df_self = (SimpleDateFormat *) DATA_PTR (self);
    SimpleDateFormat *df_orig = UTimeFormatObject (orig);

    if (self == orig) 
        return self;

    CHECK_FROZEN (self);

    if (df_self != NULL)
        delete df_self;

    DATA_PTR (self) = df_orig->clone ();

    return self;
}

static VALUE 
utime_format_get_lenient (VALUE self)
{
    return UTimeFormatObject (self)->isLenient () ? Qtrue : Qfalse;
}

static VALUE 
utime_format_set_lenient (VALUE self, VALUE value)
{
    UTimeFormatObject (self)->setLenient (RTEST (value));
    return value;
}

static VALUE 
utime_format_format (VALUE self, VALUE time)
{
    UnicodeString  str;

    UTimeFormatObject (self)->format (NUM2DBL (time) * 1000, str);
    return rb_ustring_from_UnicodeString (&str);
}

static VALUE 
utime_format_parse (VALUE self, VALUE str)
{
    UDate                d;
    ParsePosition        pos (0);
    Calendar            *calendar;
    UErrorCode           err = U_ZERO_ERROR;
    SimpleDateFormat    *df = UTimeFormatObject (self);

    d = df->parse (rb_ustring_to_UnicodeString (str), pos);

    if (pos.getIndex () == 0)
        rb_raise (rb_eArgError, "failed to parse string");

    calendar = Calendar::createInstance (df->getTimeZone ().clone (), 
                                         util_create_locale (util_locale2c (utime_format_get_locale (self))), 
                                         err);
    util_icu_error_with_delete (err, calendar);

    calendar->setTime (d, err);
    util_icu_error_with_delete (err, calendar);

    return rb_utime_new_from_object (calendar);
}

/************************************************************************
 *           INIT                                                       *
 ************************************************************************/


static void
ucalendar_init ()
{
    /*********  UTime  *********/
    
    /* TODO: use #to_time for implicit type convertion (no #to_f) */
    /* TODO: #locale is losed in Calendar->clone () method */
    
    rb_cUTime = rb_define_class ("UTime", rb_cObject);
    rb_include_module (rb_cUTime, rb_mComparable);
    ADD_INIT_METHODS (rb_cUTime, utime_alloc);
    rb_define_singleton_method (rb_cUTime, "at", VALUEFUNC (utime_at), -1);
    rb_define_singleton_method (rb_cUTime, "now", VALUEFUNC (utime_now), -1);
    //rb_define_singleton_method (rb_cUTime, "mktime", VALUEFUNC (utime_mktime), -1);
    
    rb_define_method (rb_cUTime, "initialize", VALUEFUNC (utime_init), -1);
    rb_define_method (rb_cUTime, "initialize_copy", VALUEFUNC (utime_init_copy), 1);
    rb_define_method (rb_cUTime, "type", VALUEFUNC (utime_get_type), 0);
    rb_define_method (rb_cUTime, "zone", VALUEFUNC (utime_get_zone), 0);
    rb_define_method (rb_cUTime, "zone=", VALUEFUNC (utime_set_zone), 1);
    rb_define_method (rb_cUTime, "locale", VALUEFUNC (utime_get_locale), 0);
    rb_define_method (rb_cUTime, "lenient", VALUEFUNC (utime_get_lenient), 0);
    rb_define_method (rb_cUTime, "lenient=", VALUEFUNC (utime_set_lenient), 1);

    rb_define_method (rb_cUTime, "to_i", VALUEFUNC (utime_to_i), 0);
    rb_define_method (rb_cUTime, "to_f", VALUEFUNC (utime_to_f), 0);
    rb_define_method (rb_cUTime, "to_time", VALUEFUNC (utime_to_time), 0);
    rb_define_method (rb_cUTime, "<=>", VALUEFUNC (utime_cmp), 1);
    rb_define_method (rb_cUTime, "==", VALUEFUNC (utime_equal), 1); 
    rb_define_method (rb_cUTime, "eql?", VALUEFUNC (utime_eql), 1);
    rb_define_method (rb_cUTime, "hash", VALUEFUNC (utime_hash), 0);

    // TODO: rb_define_method (rb_cUTime, "localtime", VALUEFUNC (utime_localtime), 0);
    // TODO: rb_define_method (rb_cUTime, "gmtime", VALUEFUNC (utime_gmtime), 0);
    // TODO: rb_define_method (rb_cUTime, "utc", VALUEFUNC (utime_gmtime), 0);
    // TODO: rb_define_method (rb_cUTime, "getlocal", VALUEFUNC (utime_getlocaltime), 0);
    // TODO: rb_define_method (rb_cUTime, "getgm", VALUEFUNC (utime_getgmtime), 0);
    // TODO: rb_define_method (rb_cUTime, "getutc", VALUEFUNC (utime_getgmtime), 0);

    // TODO: rb_define_method (rb_cUTime, "ctime", VALUEFUNC (utime_asctime), 0);
    // TODO: rb_define_method (rb_cUTime, "asctime", VALUEFUNC (utime_asctime), 0);
    rb_define_method (rb_cUTime, "to_s", VALUEFUNC (utime_to_s), 0);
    rb_define_method (rb_cUTime, "inspect", VALUEFUNC (utime_to_s), 0);
    // TODO: rb_define_method (rb_cUTime, "to_a", VALUEFUNC (utime_to_a), 0);
    
    rb_define_method (rb_cUTime, "+", VALUEFUNC (utime_plus), 1);
    rb_define_method (rb_cUTime, "-", VALUEFUNC (utime_minus), 1);
    rb_define_method (rb_cUTime, "succ", VALUEFUNC (utime_succ), 0);

    
    // TODO: rb_define_method (rb_cUTime, "utc?", VALUEFUNC (utime_utc_p), 0);
    // TODO: rb_define_method (rb_cUTime, "gmt?", VALUEFUNC (utime_utc_p), 0);

    // TODO: rb_define_method (rb_cUTime, "tv_sec", VALUEFUNC (utime_to_i), 0);
    // TODO: rb_define_method (rb_cUTime, "tv_usec", VALUEFUNC (utime_usec), 0);
    // TODO: rb_define_method (rb_cUTime, "usec", VALUEFUNC (utime_usec), 0);

    // TODO: rb_define_method (rb_cUTime, "strftime", VALUEFUNC (utime_strftime), 1);

    /* methods for marshaling */
    // TODO: rb_define_method (rb_cUTime, "_dump", VALUEFUNC (utime_dump), -1);
    // TODO: rb_define_singleton_method (rb_cUTime, "_load", VALUEFUNC (utime_load), 1);

    rb_define_method (rb_cUTime, "add", VALUEFUNC (utime_add), 2);
    rb_define_method (rb_cUTime, "add!", VALUEFUNC (utime_add_e), 2);
    rb_define_method (rb_cUTime, "[]", VALUEFUNC (utime_get_field), 1);
    rb_define_method (rb_cUTime, "[]=", VALUEFUNC (utime_set_field), 2);

    ENUM_INIT (date_field);
    ENUM_ADD_DATE_FIELD (era,           UCAL_ERA);                  // Example: 0..1
    ENUM_ADD_DATE_FIELD (year,          UCAL_YEAR);                 // Example: 1..big number
    ENUM_ADD_DATE_FIELD (month,         UCAL_MONTH);                // Example: 1..12
    ENUM_ADD_DATE_FIELD (yweek,         UCAL_WEEK_OF_YEAR);         // Example: 1..53
    ENUM_ADD_DATE_FIELD (mweek,         UCAL_WEEK_OF_MONTH);        // Example: 1..4
    ENUM_ADD_DATE_FIELD (mday,          UCAL_DATE);                 // Example: 1..31
    ENUM_ADD_DATE_FIELD (yday,          UCAL_DAY_OF_YEAR);          // Example: 1..365
    ENUM_ADD_DATE_FIELD (wday,          UCAL_DAY_OF_WEEK);          // Example: 0..6
    ENUM_ADD_DATE_FIELD (wday_in_month, UCAL_DAY_OF_WEEK_IN_MONTH); // Example: 1..4, may be specified as -1
    ENUM_ADD_DATE_FIELD (am_pm,         UCAL_AM_PM);                // Example: 0..1
    ENUM_ADD_DATE_FIELD (hour12,        UCAL_HOUR);                 // Example: 0..11
    ENUM_ADD_DATE_FIELD (hour,          UCAL_HOUR_OF_DAY);          // Example: 0..23
    ENUM_ADD_DATE_FIELD (min,           UCAL_MINUTE);               // Example: 0..59
    ENUM_ADD_DATE_FIELD (sec,           UCAL_SECOND);               // Example: 0..59
    ENUM_ADD_DATE_FIELD (msec,          UCAL_MILLISECOND);          // Example: 0..999
    ENUM_ADD_DATE_FIELD (zone_offset,   UCAL_ZONE_OFFSET);          // Example: -12*U_MILLIS_PER_HOUR..12*U_MILLIS_PER_HOUR
    ENUM_ADD_DATE_FIELD (dst_offset,    UCAL_DST_OFFSET);           // Example: 0 or U_MILLIS_PER_HOUR
    ENUM_ADD_DATE_FIELD (year_woy,      UCAL_YEAR_WOY);             // Example: 1..big number - Year of (Week of Year)
    ENUM_ADD_DATE_FIELD (wday_local,    UCAL_DOW_LOCAL);            // Example: 1..7 - Day of Week / Localized

    rb_define_method (rb_cUTime, "day", VALUEFUNC (utime_get_mday), 0);
    rb_define_method (rb_cUTime, "day=", VALUEFUNC (utime_set_mday), 1);
    
    rb_define_method (rb_cUTime, "mon", VALUEFUNC (utime_get_month), 0);
    rb_define_method (rb_cUTime, "mon=", VALUEFUNC (utime_set_month), 1);

    /* TODO: isdst?, dst? */
    /* TODO: zone - ruby returns CEST, UTC, .... */
    /* TODO: ruby gmtoff, gmt_offset, utc_offset == (zone_offset + dst_offset) / 1000 in icu */

    /*********  UTime::Format  *********/
    rb_cUTimeFormat = rb_define_class_under (rb_cUTime, "Format", rb_cObject);
    ADD_INIT_METHODS (rb_cUTimeFormat, utime_format_alloc);
    rb_define_method (rb_cUTimeFormat, "initialize", VALUEFUNC (utime_format_init), -1);
    rb_define_method (rb_cUTimeFormat, "initialize_copy", VALUEFUNC (utime_format_init_copy), 1);
    rb_define_method (rb_cUTimeFormat, "zone", VALUEFUNC (utime_format_get_zone), 0);
    rb_define_method (rb_cUTimeFormat, "zone=", VALUEFUNC (utime_format_set_zone), 1);
    rb_define_method (rb_cUTimeFormat, "locale", VALUEFUNC (utime_format_get_locale), 0);
    rb_define_method (rb_cUTimeFormat, "lenient", VALUEFUNC (utime_format_get_lenient), 0);
    rb_define_method (rb_cUTimeFormat, "lenient=", VALUEFUNC (utime_format_set_lenient), 1);
    rb_define_method (rb_cUTimeFormat, "format", VALUEFUNC (utime_format_format), 1);
    rb_define_method (rb_cUTimeFormat, "parse", VALUEFUNC (utime_format_parse), 1);

    ENUM_INIT (date_format_style);
    ENUM_ADD  (date_format_style, full,        DateFormat::FULL);
    ENUM_ADD  (date_format_style, long,        DateFormat::LONG);
    ENUM_ADD  (date_format_style, medium,      DateFormat::MEDIUM);
    ENUM_ADD  (date_format_style, short,       DateFormat::SHORT);
    ENUM_ADD  (date_format_style, default,     DateFormat::DEFAULT);
    ENUM_ADD  (date_format_style, date_offset, DateFormat::DATE_OFFSET);
    ENUM_ADD  (date_format_style, none,        DateFormat::NONE);
    ENUM_ADD  (date_format_style, date_time,   DateFormat::DATE_TIME);

    date_default_format = rb_utime_format_new (id_full, id_full, NULL, "en_US");
    rb_global_variable (&date_default_format);
    OBJ_FREEZE (date_default_format);
    rb_define_const (rb_cUTime, "DEFAULT_FORMAT", date_default_format);
}

