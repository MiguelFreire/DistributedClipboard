#include "clipboard_protocol.h"

packed_message new_message(message_type type, message_method method, int region, void *data, size_t count, bool has_status, bool status, bool has_lower_copy, bool lower_copy)
{
    CBMessage msg = CBMESSAGE__INIT;

    size_t packed_size;
    void *buffer;
    packed_message package = {NULL, 0};

    msg.type = type;
    msg.method = method;
    msg.region = region;
    if (data != NULL)
    {
        msg.n_data = 1;
        msg.data = smalloc(sizeof(ProtobufCBinaryData));
        msg.data->data = data;
        msg.data->len = count;
    }

    if (count != 0)
    {
        msg.has_size = 1;
        msg.size = count;
    }
    if (has_lower_copy)
    {
        msg.has_lower_copy = true;
        msg.lower_copy = true;
    }
    if (has_status)
    {
        msg.has_status = 1;
        msg.status = status;
    }
    packed_size = cbmessage__get_packed_size(&msg);

    buffer = smalloc(packed_size);
    cbmessage__pack(&msg, buffer);

    package.buf = buffer;
    package.size = packed_size;
    free(msg.data);

    return package;
}

