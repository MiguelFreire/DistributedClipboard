#include "clipboard_protocol.h"
/*
@Name: new_message()
@Args: (message_type) type - message type (Request/Response)
       (message_method) method - message method (Copy/Paste/Wait/Sync)
       (size_t) region - region number
       (void*) data - pointer of data to appende into the message
       (size_t) count - size of message/size of data
       (bool) has_status - flag for status
       (bool) status - status (1 - OK) (0-ERROR)
       (bool) has_lower_copy - flag for lower_copy
       (bool) lower_copy - bool to force lower_copy
@Desc: Creates a suited message for network communication
@Return: (packaged_message) return an object suited for sending over the network
*/
packed_message new_message(message_type type, message_method method, size_t region, void *data, size_t count, bool has_status, bool status, bool has_lower_copy, bool lower_copy)
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

