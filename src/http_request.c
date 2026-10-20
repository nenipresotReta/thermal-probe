#include "http_request.h"
#include <string.h>

enum http_request_result http_request_parse(const char *data, size_t length)
{
    size_t end = 0, line = 0;
    for (size_t i = 0; i < length; ++i) {
        if (!data[i]) return HTTP_REQUEST_BAD;
        if (i >= 3 && !memcmp(data + i - 3, "\r\n\r\n", 4)) {
            end = i + 1;
            break;
        }
    }
    if (!end) return length >= HTTP_REQUEST_LIMIT
        ? HTTP_REQUEST_TOO_LARGE : HTTP_REQUEST_INCOMPLETE;
    if (end > HTTP_REQUEST_LIMIT) return HTTP_REQUEST_TOO_LARGE;
    while (line + 1 < end && memcmp(data + line, "\r\n", 2)) ++line;
    if (line < 9 || (memcmp(data + line - 9, " HTTP/1.1", 9) &&
                     memcmp(data + line - 9, " HTTP/1.0", 9))) {
        return HTTP_REQUEST_BAD;
    }
    size_t target_length = line - 9;
    if ((target_length == 20 && !memcmp(data, "GET /api/temperature", 20)) ||
        (target_length == 5 && !memcmp(data, "GET /", 5))) {
        return HTTP_REQUEST_TEMPERATURE;
    }
    return HTTP_REQUEST_NOT_FOUND;
}
