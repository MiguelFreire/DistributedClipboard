/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: cbmessage.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "cbmessage.pb-c.h"
void   cbmessage__init
                     (CBMessage         *message)
{
  static const CBMessage init_value = CBMESSAGE__INIT;
  *message = init_value;
}
size_t cbmessage__get_packed_size
                     (const CBMessage *message)
{
  assert(message->base.descriptor == &cbmessage__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t cbmessage__pack
                     (const CBMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &cbmessage__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t cbmessage__pack_to_buffer
                     (const CBMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &cbmessage__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
CBMessage *
       cbmessage__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (CBMessage *)
     protobuf_c_message_unpack (&cbmessage__descriptor,
                                allocator, len, data);
}
void   cbmessage__free_unpacked
                     (CBMessage *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &cbmessage__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCEnumValue cbmessage__type__enum_values_by_number[2] =
{
  { "Request", "CBMESSAGE__TYPE__Request", 0 },
  { "Response", "CBMESSAGE__TYPE__Response", 1 },
};
static const ProtobufCIntRange cbmessage__type__value_ranges[] = {
{0, 0},{0, 2}
};
static const ProtobufCEnumValueIndex cbmessage__type__enum_values_by_name[2] =
{
  { "Request", 0 },
  { "Response", 1 },
};
const ProtobufCEnumDescriptor cbmessage__type__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "CBMessage.Type",
  "Type",
  "CBMessage__Type",
  "",
  2,
  cbmessage__type__enum_values_by_number,
  2,
  cbmessage__type__enum_values_by_name,
  1,
  cbmessage__type__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const ProtobufCEnumValue cbmessage__method__enum_values_by_number[2] =
{
  { "Copy", "CBMESSAGE__METHOD__Copy", 0 },
  { "Paste", "CBMESSAGE__METHOD__Paste", 1 },
};
static const ProtobufCIntRange cbmessage__method__value_ranges[] = {
{0, 0},{0, 2}
};
static const ProtobufCEnumValueIndex cbmessage__method__enum_values_by_name[2] =
{
  { "Copy", 0 },
  { "Paste", 1 },
};
const ProtobufCEnumDescriptor cbmessage__method__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "CBMessage.Method",
  "Method",
  "CBMessage__Method",
  "",
  2,
  cbmessage__method__enum_values_by_number,
  2,
  cbmessage__method__enum_values_by_name,
  1,
  cbmessage__method__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const ProtobufCFieldDescriptor cbmessage__field_descriptors[6] =
{
  {
    "type",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(CBMessage, type),
    &cbmessage__type__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "method",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(CBMessage, method),
    &cbmessage__method__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "region",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(CBMessage, region),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "data",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(CBMessage, has_data),
    offsetof(CBMessage, data),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "size",
    5,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(CBMessage, has_size),
    offsetof(CBMessage, size),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "status",
    6,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BOOL,
    offsetof(CBMessage, has_status),
    offsetof(CBMessage, status),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned cbmessage__field_indices_by_name[] = {
  3,   /* field[3] = data */
  1,   /* field[1] = method */
  2,   /* field[2] = region */
  4,   /* field[4] = size */
  5,   /* field[5] = status */
  0,   /* field[0] = type */
};
static const ProtobufCIntRange cbmessage__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 6 }
};
const ProtobufCMessageDescriptor cbmessage__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "CBMessage",
  "CBMessage",
  "CBMessage",
  "",
  sizeof(CBMessage),
  6,
  cbmessage__field_descriptors,
  cbmessage__field_indices_by_name,
  1,  cbmessage__number_ranges,
  (ProtobufCMessageInit) cbmessage__init,
  NULL,NULL,NULL    /* reserved[123] */
};
