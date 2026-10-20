#include "http_request.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void fragmented(const char *request)
{
    size_t n = strlen(request);
    /* Every possible TCP fragmentation point, without a trailing NUL. */
    for (size_t i = 0; i < n; ++i) {
        assert(http_request_parse(request, i) == HTTP_REQUEST_INCOMPLETE);
    }
    assert(http_request_parse(request, n) == HTTP_REQUEST_TEMPERATURE);
}

int main(void)
{
    fragmented("GET /api/temperature HTTP/1.1\r\nHost: probe\r\n\r\n");
    fragmented("GET / HTTP/1.0\r\n\r\n");
    const char *pipelined = "GET /api/temperature HTTP/1.1\r\n\r\nGET / HTTP/1.1\r\n\r\n";
    assert(http_request_parse(pipelined, strlen(pipelined)) == HTTP_REQUEST_TEMPERATURE);
    const char *missing = "GET /missing HTTP/1.1\r\n\r\n";
    assert(http_request_parse(missing, strlen(missing)) == HTTP_REQUEST_NOT_FOUND);
    const char *malformed = "GET /\r\n\r\n";
    assert(http_request_parse(malformed, strlen(malformed)) == HTTP_REQUEST_BAD);
    char too_large[HTTP_REQUEST_LIMIT];
    memset(too_large, 'x', sizeof too_large);
    assert(http_request_parse(too_large, sizeof too_large) == HTTP_REQUEST_TOO_LARGE);
    too_large[10] = 0;
    assert(http_request_parse(too_large, sizeof too_large) == HTTP_REQUEST_BAD);
    puts("PASS: fragmented HTTP headers, HTTP versions, pipelining, invalid and oversized requests");
}
