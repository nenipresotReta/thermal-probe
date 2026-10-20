#pragma once
#include <stddef.h>

#define HTTP_REQUEST_LIMIT 512
enum http_request_result {
    HTTP_REQUEST_INCOMPLETE,
    HTTP_REQUEST_TEMPERATURE,
    HTTP_REQUEST_NOT_FOUND,
    HTTP_REQUEST_BAD,
    HTTP_REQUEST_TOO_LARGE
};
/* Parse a bounded, not necessarily NUL-terminated TCP receive buffer. */
enum http_request_result http_request_parse(const char *data, size_t length);
