/**********************************************************************
  
    Copyright (C) 2005 Jan Becvar
    Copyright (C) 2005 soLNet.cz

**********************************************************************/

#ifndef RUBY_UNICODE_H
#define RUBY_UNICODE_H

#include <ruby.h>
#include <unicode/calendar.h>
#include <unicode/datefmt.h>
#include <unicode/smpdtfmt.h>
#include <unicode/ustring.h>
#include <unicode/uclean.h>
#include <unicode/ucol.h>
#include <unicode/utf.h>
#include <unicode/udat.h>
#include <unicode/ubrk.h>

extern "C" {

#define U_MAX_STRING_LEN 1000000000

EXTERN VALUE rb_mUTF8;
EXTERN VALUE rb_cUError;
EXTERN VALUE rb_cUString;
EXTERN VALUE rb_cUCollator;
EXTERN VALUE rb_cUTime;
EXTERN VALUE rb_cUTimeFormat;

/* USTRING */

struct RUString
{
    long  len;        /* size of string (number of UChars) */
    UChar *ptr;       /* may be NULL when len is 0 */
    long  len_chars;  /* count of characters in string (-1 if not calculated) */
};
typedef struct RUString RUString;

#define UStringStruct(v) rb_ustring_struct (&(v))
#define IS_USTRING(v) rb_is_ustring (v)

int rb_is_ustring (VALUE v);
RUString * rb_ustring_struct (volatile VALUE *ptr); /* checks VALUE type */

VALUE rb_ustring_new (UChar *ptr, long len);
VALUE rb_ustring_from_str (VALUE copy_str);
VALUE rb_ustring_from_ascii (const char *ascii_str);
VALUE rb_ustring_replace (VALUE self, VALUE orig);

VALUE rb_str_to_ustring (VALUE str);
UnicodeString rb_ustring_to_UnicodeString (VALUE self);


/* STRING */

enum RUEncoding {
    RUBY_U_UTF8,
    RUBY_U_UTF16,
};

UErrorCode rb_str_replace_uchars (VALUE str, const UChar *uptr, int32_t ulen);
VALUE rb_str_from_uchars (const UChar *uptr, int32_t ulen, UErrorCode *err);
VALUE rb_str_from_UnicodeString (UnicodeString *uni_str, UErrorCode *err);

/* UCOLLATOR */

#define UCollatorObject(v) rb_ucollator_object (&(v))
#define IS_UCOLLATOR(v) rb_is_ucollator (v)

int rb_is_ucollator (VALUE v);
UCollator * rb_ucollator_object (volatile VALUE *ptr); /* checks VALUE type */

/* UTIME */

#define UTimeObject(v) rb_utime_object (&(v))
#define IS_UTIME(v) rb_is_udate (v)

int rb_is_udate (VALUE v);
Calendar * rb_utime_object (volatile VALUE *ptr); /* checks VALUE type */

VALUE rb_utime_new (const char *zone, const char *locale);
VALUE rb_utime_new_from_object (Calendar *cal);

/* UTIME_FORMAT */

#define UTimeFormatObject(v) rb_utime_format_object (&(v))
#define IS_UTIME_FORMAT(v) rb_is_utime_format (v)

int rb_is_utime_format (VALUE v);
SimpleDateFormat * rb_utime_format_object (volatile VALUE *ptr); /* checks VALUE type */

} /* extern "C" */

#endif /* ifndef RUBY_UNICODE_H */

