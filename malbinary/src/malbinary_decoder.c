#include "malbinary.h"

mal_decoder_t *malbinary_decoder_new(bool varint_supported) {
  mal_decoder_t *self = (mal_decoder_t *) malloc(sizeof(mal_decoder_t));
  if (!self)
    return NULL;

  self->encoding_format_code = MALBINARY_FORMAT_CODE;
  self->varint_supported = varint_supported;
  self->logger = CLOG_WARN_LEVEL;

  malbinary_init_decode_functions(self);

  return self;
}

void *malbinary_decoder_new_cursor(char *bytes, unsigned int length, unsigned int offset) {
  malbinary_cursor_t *cursor = (malbinary_cursor_t *) malloc(sizeof(malbinary_cursor_t));
  if (! cursor) return NULL;

  malbinary_cursor_init(cursor, bytes, length, offset);

  return (void *) cursor;
}

void  malbinary_decoder_cursor_reset(void *cursor,
    char *bytes, unsigned int length, unsigned int offset) {
  malbinary_cursor_reset(cursor);
  malbinary_cursor_init(cursor, bytes, length, offset);
}

short malbinary_read16(void *cursor) {
  unsigned int index = ((malbinary_cursor_t *) cursor)->body_offset;;
  short res = (short) ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFF) << 8);
  res = (short) (res | (((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFF));
  ((malbinary_cursor_t *) cursor)->body_offset = index;
  return res;
}

int malbinary_read32(void *cursor) {
  unsigned int index = ((malbinary_cursor_t *) cursor)->body_offset;
  int res = (((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFF) << 24;
  res = res | ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFF) << 16);
  res = res | ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFF) << 8);
  res = res | (((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFF);
  ((malbinary_cursor_t *) cursor)->body_offset = index;
  return res;
}

long malbinary_read64(void *cursor) {
  unsigned int index = ((malbinary_cursor_t *) cursor)->body_offset;
  long res = ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFFL) << 56);
  res = res | ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFFL) << 48);
  res = res | ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFFL) << 40);
  res = res | ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFFL) << 32);
  res = res | ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFFL) << 24);
  res = res | ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFFL) << 16);
  res = res | ((((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFFL) << 8);
  res = res | (((malbinary_cursor_t *) cursor)->body_ptr[index++] & 0xFFL);
  ((malbinary_cursor_t *) cursor)->body_offset = index;
  return res;
}

unsigned short malbinary_read_uvarshort(void *cursor) {
  unsigned int index = ((malbinary_cursor_t *) cursor)->body_offset;
  unsigned int value = 0;
  int i;
  int b;
  for (i = 0; ((b = ((malbinary_cursor_t *) cursor)->body_ptr[index++]) & 0x80) != 0; i += 7) {
    value |= (b & 0x7f) << i;
  }
  ((malbinary_cursor_t *) cursor)->body_offset = index;
  return (value | b << i) & 0xFFFF;
}

unsigned int malbinary_read_uvarint(void *cursor) {
  unsigned int index = ((malbinary_cursor_t *) cursor)->body_offset;
  unsigned int value = 0;
  int i;
  int b;
  for (i = 0; ((b = ((malbinary_cursor_t *) cursor)->body_ptr[index++]) & 0x80) != 0; i += 7) {
    value |= (b & 0x7f) << i;
  }
  ((malbinary_cursor_t *) cursor)->body_offset = index;
  return value | b << i;
}

unsigned long malbinary_read_uvarlong(void *cursor) {
  unsigned int index = ((malbinary_cursor_t *) cursor)->body_offset;
  unsigned long value = 0L;
  int i;
  long b;
  for (i = 0; ((b = ((malbinary_cursor_t *) cursor)->body_ptr[index++]) & 0x80L) != 0L; i += 7) {
    value |= (b & 0x7fL) << i;
  }
  ((malbinary_cursor_t *) cursor)->body_offset = index;
  return value | b << i;
}

short malbinary_read_varshort(void *cursor) {
  unsigned int i = malbinary_read_uvarint(cursor);
  return ((i >> 1) ^ -(i & 1));
}

int malbinary_read_varint(void *cursor) {
  unsigned int i = malbinary_read_uvarint(cursor);
  return ((i >> 1) ^ -(i & 1));
}

long malbinary_read_varlong(void *cursor) {
  unsigned long l = malbinary_read_uvarlong(cursor);
  return ((l >> 1) ^ -(l & 1));
}

char *malbinary_read_str(mal_decoder_t *self, void *cursor) {
  unsigned int length;
  if (self->varint_supported)
    length = malbinary_read_uvarint(cursor);
  else
    length = malbinary_read32(cursor);

  char *array = (char *) malloc(length + 1);
  if (array == NULL)
    return NULL;

  bcopy(& ((malbinary_cursor_t *) cursor)->body_ptr[((malbinary_cursor_t *) cursor)->body_offset], array, length);
  array[length] = '\0';

  ((malbinary_cursor_t *) cursor)->body_offset += length;

  return array;
}

void malbinary_read_array(char *array, unsigned int length, void *cursor) {
  unsigned int index = ((malbinary_cursor_t *) cursor)->body_offset;
  for (int i = 0; i < length; i++) {
    array[i] = ((malbinary_cursor_t *) cursor)->body_ptr[index++];
  }
  ((malbinary_cursor_t *) cursor)->body_offset = index;
}

char malbinary_read(void *cursor) {
  unsigned int index = ((malbinary_cursor_t *) cursor)->body_offset;
  char res = ((malbinary_cursor_t *) cursor)->body_ptr[index++];
  ((malbinary_cursor_t *) cursor)->body_offset = index;
  return res;
}

int malbinary_decoder_decode_string(mal_decoder_t *self, void *cursor, mal_string_t **result) {
  int rc = 0;
  (*result) = malbinary_read_str(self, cursor);
  if (*result == NULL)
    return -1;
  return rc;
}

int malbinary_decoder_decode_presence_flag(mal_decoder_t *self,
    void *cursor, bool *result) {
  int rc = 0;
  char flag = malbinary_read(cursor);
  if (flag == 1) {
    (*result) = true;
  } else {
    (*result) = false;
  }
  return rc;
}

int malbinary_decoder_decode_integer(mal_decoder_t *self, void *cursor, mal_integer_t *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_varint(cursor);
  else
    (*result) = malbinary_read32(cursor);
  return rc;
}

int malbinary_decoder_decode_short_form(mal_decoder_t *self, void *cursor, long *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_varlong(cursor);
  else
    (*result) = malbinary_read64(cursor);
  return rc;
}

int malbinary_decoder_decode_list_size(mal_decoder_t *self, void *cursor, unsigned int *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_uvarint(cursor);
  else
    (*result) = malbinary_read32(cursor);
  return rc;
}

int malbinary_decoder_decode_small_enum(mal_decoder_t *self, void *cursor, int *result) {
  int rc = 0;
  (*result) = malbinary_read(cursor);
  return rc;
}

int malbinary_decoder_decode_medium_enum(mal_decoder_t *self, void *cursor, int *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_uvarshort(cursor);
  else
    (*result) = malbinary_read16(cursor);
  return rc;
}

int malbinary_decoder_decode_large_enum(mal_decoder_t *self, void *cursor, int *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_uvarint(cursor);
  else
    (*result) = malbinary_read32(cursor);
  return rc;
}

int malbinary_decoder_decode_uri(mal_decoder_t *self, void *cursor, mal_uri_t **result) {
  int rc = 0;
  (*result) = malbinary_read_str(self, cursor);
  return rc;
}

int malbinary_decoder_decode_blob(mal_decoder_t *self, void *cursor, mal_blob_t **result) {
  int rc = 0;
  unsigned int length;
  if (self->varint_supported)
    length = malbinary_read_uvarint(cursor);
  else
    length = malbinary_read32(cursor);
  mal_blob_t *blob = mal_blob_new(length);
  char *blob_content = mal_blob_get_content(blob);
  malbinary_read_array(blob_content, length, cursor);
  (*result) = blob;
  return rc;
}

int malbinary_decoder_decode_time(mal_decoder_t *self, void *cursor, mal_time_t *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_uvarlong(cursor);
  else
    (*result) = malbinary_read64(cursor);
  return rc;
}

int malbinary_decoder_decode_uinteger(mal_decoder_t *self, void *cursor, mal_uinteger_t *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_uvarint(cursor);
  else
    (*result) = malbinary_read32(cursor);
  return rc;
}

int malbinary_decoder_decode_identifier(mal_decoder_t *self, void *cursor, mal_identifier_t **result) {
  int rc = 0;
  (*result) = malbinary_read_str(self, cursor);
  return rc;
}

int malbinary_decoder_decode_uoctet(mal_decoder_t *self, void *cursor, mal_uoctet_t *result) {
  int rc = 0;
  (*result) = malbinary_read(cursor);
  return rc;
}

int malbinary_decoder_decode_long(mal_decoder_t *self, void *cursor, mal_long_t *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_varlong(cursor);
  else
    (*result) = malbinary_read64(cursor);
  return rc;
}

int malbinary_decoder_decode_ushort(mal_decoder_t *self, void *cursor, mal_ushort_t *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_uvarshort(cursor);
  else
    (*result) = malbinary_read16(cursor);
  return rc;
}

int malbinary_decoder_decode_boolean(mal_decoder_t *self, void *cursor, mal_boolean_t *result) {
  int rc = 0;
  char flag = malbinary_read(cursor);
  if (flag == 1) {
    (*result) = true;
  } else {
    (*result) = false;
  }
  return rc;
}

int malbinary_decoder_decode_attribute_tag(mal_decoder_t *self, void *cursor, unsigned char *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_varint(cursor);
  else
    (*result) = malbinary_read32(cursor);
  return rc;
}

int malbinary_decoder_decode_duration(mal_decoder_t *self, void *cursor, mal_duration_t *result) {
  int rc = 0;
  //TODO: malbinary_decoder_decode_duration
  return rc;
}

float intBitsToFloat(int x) {
  union {
    float f;  // assuming 32-bit IEEE 754 single-precision
    int i;    // assuming 32-bit 2's complement int
  } u;
  u.i = x;
  return u.f;
}

int malbinary_decoder_decode_float(mal_decoder_t *self, void *cursor, mal_float_t *result) {
  int rc = 0;
  mal_integer_t i;
  malbinary_decoder_decode_integer(self, cursor, &i);
  (*result) = intBitsToFloat(i);
  return rc;
}

double longBitsToDouble(long x) {
  union {
    double d;
    long l;
  } u;
  u.l = x;
  return u.d;
}

int malbinary_decoder_decode_double(mal_decoder_t *self, void *cursor, mal_double_t *result) {
  int rc = 0;
  mal_long_t l;
  malbinary_decoder_decode_long(self, cursor, &l);
  (*result) = longBitsToDouble(l);
  return rc;
}

int malbinary_decoder_decode_octet(mal_decoder_t *self, void *cursor, mal_octet_t *result) {
  int rc = 0;
  (*result) = malbinary_read(cursor);
  return rc;
}

int malbinary_decoder_decode_short(mal_decoder_t *self, void *cursor, mal_short_t *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_varshort(cursor);
  else
    (*result) = malbinary_read16(cursor);
  return rc;
}

int malbinary_decoder_decode_ulong(mal_decoder_t *self, void *cursor, mal_ulong_t *result) {
  int rc = 0;
  if (self->varint_supported)
    (*result) = malbinary_read_uvarlong(cursor);
  else
    (*result) = malbinary_read64(cursor);
  return rc;
}

int malbinary_decoder_decode_finetime(mal_decoder_t *self, void *cursor, mal_finetime_t *result) {
  int rc = 0;
  //TODO: malbinary_decoder_decode_finetime
  return rc;
}

int malbinary_decoder_decode_attribute(mal_decoder_t *decoder, void *cursor,
    unsigned char attribute_tag, union mal_attribute_t self) {
  int rc = 0;
  switch (attribute_tag) {
  case MAL_BLOB_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_blob(decoder, cursor, &self.blob_value);
    break;
  case MAL_BOOLEAN_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_boolean(decoder, cursor, &self.boolean_value);
    break;
  case MAL_DURATION_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_duration(decoder, cursor, &self.duration_value);
    break;
  case MAL_FLOAT_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_float(decoder, cursor, &self.float_value);
    break;
  case MAL_DOUBLE_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_double(decoder, cursor, &self.double_value);
    break;
  case MAL_IDENTIFIER_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_identifier(decoder, cursor, &self.identifier_value);
    break;
  case MAL_OCTET_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_octet(decoder, cursor, &self.octet_value);
    break;
  case MAL_UOCTET_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_uoctet(decoder, cursor, &self.uoctet_value);
    break;
  case MAL_SHORT_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_short(decoder, cursor, &self.short_value);
    break;
  case MAL_USHORT_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_ushort(decoder, cursor, &self.ushort_value);
    break;
  case MAL_INTEGER_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_integer(decoder, cursor, &self.integer_value);
    break;
  case MAL_UINTEGER_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_uinteger(decoder, cursor, &self.uinteger_value);
    break;
  case MAL_LONG_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_long(decoder, cursor, &self.long_value);
    break;
  case MAL_ULONG_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_ulong(decoder, cursor, &self.ulong_value);
    break;
  case MAL_STRING_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_string(decoder, cursor, &self.string_value);
    break;
  case MAL_TIME_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_time(decoder, cursor, &self.time_value);
    break;
  case MAL_FINETIME_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_finetime(decoder, cursor, &self.finetime_value);
    break;
  case MAL_URI_ATTRIBUTE_TAG:
    rc = malbinary_decoder_decode_uri(decoder, cursor, &self.uri_value);
    break;
  default:
    //nothing to do
    break;
  }
  return rc;
}

// TODO (AF): The malbinary decoding functions should be private and only used through
// the mal_decoder_t structure.

mal_decoder_initialize_functions_fn malbinary_decoder_initialize_functions;

void malbinary_init_decode_functions(mal_decoder_t *self) {
  // TODO (AF): Currently the use of the MAL generic initialization function
  // (malbinary_decoder_initialize_functions) is not possible as it causes a
  // circular dependency with the mal module.
  // The malbinary_decoder_initialize_functions below must have exactly the
  // same signature and code of the corresponding MAL function.
  malbinary_decoder_initialize_functions(self,
      malbinary_decoder_new_cursor,
      malbinary_decoder_cursor_reset,
      malbinary_cursor_destroy,
      malbinary_cursor_get_length,
      malbinary_cursor_get_offset,
      malbinary_cursor_assert,
      malbinary_read16,
      malbinary_read32,
      malbinary_read64,
      malbinary_decoder_decode_string,
      malbinary_decoder_decode_presence_flag,
      malbinary_decoder_decode_short_form,
      malbinary_decoder_decode_small_enum,
      malbinary_decoder_decode_medium_enum,
      malbinary_decoder_decode_large_enum,
      malbinary_decoder_decode_integer,
      malbinary_decoder_decode_list_size,
      malbinary_decoder_decode_uri,
      malbinary_decoder_decode_blob,
      malbinary_decoder_decode_time,
      malbinary_decoder_decode_uinteger,
      malbinary_decoder_decode_identifier,
      malbinary_decoder_decode_uoctet,
      malbinary_decoder_decode_long,
      malbinary_decoder_decode_ushort,
      malbinary_decoder_decode_boolean,
      malbinary_decoder_decode_duration,
      malbinary_decoder_decode_float,
      malbinary_decoder_decode_double,
      malbinary_decoder_decode_octet,
      malbinary_decoder_decode_short,
      malbinary_decoder_decode_ulong,
      malbinary_decoder_decode_finetime,
      malbinary_decoder_decode_attribute,
      malbinary_decoder_decode_attribute_tag);
}

// TODO: This function below must always have exactly the same signature and code
// of the corresponding MAL function (malbinary_decoder_initialize_functions).
void malbinary_decoder_initialize_functions(
    mal_decoder_t *self,
    mal_decoder_new_cursor_fn *new_cursor,
    mal_decoder_cursor_reset_fn *cursor_reset,
    mal_decoder_cursor_destroy_fn *cursor_destroy,
    mal_decoder_cursor_get_length_fn *cursor_get_length,
    mal_decoder_cursor_get_offset_fn *cursor_get_offset,
    mal_decoder_cursor_assert_fn *cursor_assert,
    mal_read16_fn *mal_read16,
    mal_read32_fn *mal_read32,
    mal_read64_fn *mal_read64,
    mal_decoder_decode_string_fn *mal_decoder_decode_string,
    mal_decoder_decode_presence_flag_fn *mal_decoder_decode_presence_flag,
    mal_decoder_decode_short_form_fn *mal_decoder_decode_short_form,
    mal_decoder_decode_small_enum_fn *mal_decoder_decode_small_enum,
    mal_decoder_decode_medium_enum_fn *mal_decoder_decode_medium_enum,
    mal_decoder_decode_large_enum_fn *mal_decoder_decode_large_enum,
    mal_decoder_decode_integer_fn *mal_decoder_decode_integer,
    mal_decoder_decode_list_size_fn *mal_decoder_decode_list_size,
    mal_decoder_decode_uri_fn *mal_decoder_decode_uri,
    mal_decoder_decode_blob_fn *mal_decoder_decode_blob,
    mal_decoder_decode_time_fn *mal_decoder_decode_time,
    mal_decoder_decode_uinteger_fn *mal_decoder_decode_uinteger,
    mal_decoder_decode_identifier_fn *mal_decoder_decode_identifier,
    mal_decoder_decode_uoctet_fn *mal_decoder_decode_uoctet,
    mal_decoder_decode_long_fn *mal_decoder_decode_long,
    mal_decoder_decode_ushort_fn *mal_decoder_decode_ushort,
    mal_decoder_decode_boolean_fn *mal_decoder_decode_boolean,
    mal_decoder_decode_duration_fn *mal_decoder_decode_duration,
    mal_decoder_decode_float_fn *mal_decoder_decode_float,
    mal_decoder_decode_double_fn *mal_decoder_decode_double,
    mal_decoder_decode_octet_fn *mal_decoder_decode_octet,
    mal_decoder_decode_short_fn *mal_decoder_decode_short,
    mal_decoder_decode_ulong_fn *mal_decoder_decode_ulong,
    mal_decoder_decode_finetime_fn *mal_decoder_decode_finetime,
    mal_decoder_decode_attribute_fn *mal_decoder_decode_attribute,
    mal_decoder_decode_attribute_tag_fn *mal_decoder_decode_attribute_tag) {
  // TODO (AF): Initializes the structure with parameters !!
  self->new_cursor = malbinary_decoder_new_cursor;
  self->cursor_reset = malbinary_decoder_cursor_reset;
  self->cursor_destroy = malbinary_cursor_destroy;
  self->cursor_get_length = malbinary_cursor_get_length;
  self->cursor_get_offset = malbinary_cursor_get_offset;
  self->cursor_assert = malbinary_cursor_assert;

  self->mal_read16 = malbinary_read16;
  self->mal_read32 = malbinary_read32;
  self->mal_read64 = malbinary_read64;
  self->mal_decoder_decode_string = malbinary_decoder_decode_string;
  self->mal_decoder_decode_presence_flag = malbinary_decoder_decode_presence_flag;
  self->mal_decoder_decode_short_form = malbinary_decoder_decode_short_form;
  self->mal_decoder_decode_small_enum = malbinary_decoder_decode_small_enum;
  self->mal_decoder_decode_medium_enum = malbinary_decoder_decode_medium_enum;
  self->mal_decoder_decode_large_enum = malbinary_decoder_decode_large_enum;
  self->mal_decoder_decode_integer = malbinary_decoder_decode_integer;
  self->mal_decoder_decode_list_size = malbinary_decoder_decode_list_size;
  self->mal_decoder_decode_uri = malbinary_decoder_decode_uri;
  self->mal_decoder_decode_blob = malbinary_decoder_decode_blob;
  self->mal_decoder_decode_time = malbinary_decoder_decode_time;
  self->mal_decoder_decode_uinteger = malbinary_decoder_decode_uinteger;
  self->mal_decoder_decode_identifier = malbinary_decoder_decode_identifier;
  self->mal_decoder_decode_uoctet = malbinary_decoder_decode_uoctet;
  self->mal_decoder_decode_long = malbinary_decoder_decode_long;
  self->mal_decoder_decode_ushort = malbinary_decoder_decode_ushort;
  self->mal_decoder_decode_boolean = malbinary_decoder_decode_boolean;
  self->mal_decoder_decode_duration = malbinary_decoder_decode_duration;
  self->mal_decoder_decode_float = malbinary_decoder_decode_float;
  self->mal_decoder_decode_double = malbinary_decoder_decode_double;
  self->mal_decoder_decode_octet = malbinary_decoder_decode_octet;
  self->mal_decoder_decode_short = malbinary_decoder_decode_short;
  self->mal_decoder_decode_ulong = malbinary_decoder_decode_ulong;
  self->mal_decoder_decode_finetime = malbinary_decoder_decode_finetime;
  self->mal_decoder_decode_attribute = malbinary_decoder_decode_attribute;
  self->mal_decoder_decode_attribute_tag = malbinary_decoder_decode_attribute_tag;
}

// Test

void malbinary_decoder_test(bool verbose) {
  printf(" * malbinary_decoder: ");
  if (verbose)
    printf("\n");

  //  @selftest
  // ...
  //  @end
  printf("OK\n");
}

