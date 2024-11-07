#include <libusbp_internal.h>

#ifdef _WIN32
#if (defined(__GNUC__) && !defined(__USE_MINGW_ANSI_STDIO)) || (defined(_MSC_VER) && _MSC_VER < 1900)
#error This code depends on vsnprintf returning a number of characters.
#endif
#endif

struct libusbp_error
{
    bool do_not_free;
    char * message;
    size_t code_count;
    uint32_t * code_array;
};

static uint32_t mem_error_code_array[1] = { LIBUSBP_ERROR_MEMORY };

static char error_no_memory_msg[] = "Failed to allocate memory.";
libusbp_error error_no_memory =
{
    .do_not_free = true,
    .message = error_no_memory_msg,
    .code_count = 1,
    .code_array = mem_error_code_array,
};

static char error_masked_by_no_memory_msg[] = "Failed to allocate memory for reporting an error.";
static libusbp_error error_masked_by_no_memory =
{
    .do_not_free = true,
    .message = error_masked_by_no_memory_msg,
    .code_count = 1,
    .code_array = mem_error_code_array,
};

static libusbp_error error_blank =
{
    .do_not_free = true,
    .message = NULL,
    .code_count = 0,
    .code_array = NULL,
};

void libusbp_error_free(libusbp_error * error)
{
    if (error != NULL && !error->do_not_free)
    {
        free(error->message);
        free(error->code_array);
        free(error);
    }
}

// Copies the error.  If the input is not NULL, the output will always
// be not NULL, but it might be a immutable error (do_not_free=1).
libusbp_error * libusbp_error_copy(const libusbp_error * src_error)
{
    if (src_error == NULL) { return NULL; }

    const char * src_message = src_error->message;
    if (src_message == NULL) { src_message = ""; }
    size_t message_length = strlen(src_message);

    size_t code_count = src_error->code_count;
    if (src_error->code_array == NULL) { code_count = 0; }

    // Allocate memory.
    libusbp_error * new_error = malloc(sizeof(libusbp_error));
    char * new_message = malloc(message_length + 1);
    uint32_t * new_code_array = malloc(code_count * sizeof(uint32_t));
    if (new_error == NULL || new_message == NULL ||
        (code_count != 0 && new_code_array == NULL))
    {
        free(new_error);
        free(new_message);
        free(new_code_array);
        return &error_masked_by_no_memory;
    }

    if (code_count != 0)
    {
        memcpy(new_code_array, src_error->code_array, code_count * sizeof(uint32_t));
    }
    strncpy(new_message, src_message, message_length + 1);
    new_error->do_not_free = false;
    new_error->message = new_message;
    new_error->code_count = code_count;
    new_error->code_array = new_code_array;
    return new_error;
}

// Tries to make the error mutable.  Always returns a non-NULL error,
// but it might return an immutable error (do_not_free == 1) if memory
// cannot be allocated.
static libusbp_error * error_make_mutable(libusbp_error * error)
{
    if (error == NULL) { error = &error_blank; }
    if (error->do_not_free)
    {
      error = libusbp_error_copy(error);
    }
    return error;
}

// Tries to add a message to the error.  After calling this, you
// should not use the error you passed as an input, because it might
// have been freed.
libusbp_error * error_add_v(libusbp_error * error, const char * format, va_list ap)
{
    if (format == NULL) { return error; }

    error = error_make_mutable(error);
    if (error == NULL || error->do_not_free) { return error; }

    if (error->message == NULL) { error->message = ""; }

    // Determine all the string lengths.
    size_t outer_message_length = 0;
    {
        char x[1];
        va_list ap2;
        va_copy(ap2, ap);
        int result = vsnprintf(x, 0, format, ap2);
        if (result > 0)
        {
            outer_message_length = result;
        }
        va_end(ap2);
    }
    size_t inner_message_length = strlen(error->message);
    size_t separator_length = (inner_message_length && outer_message_length) ? 2 : 0;
    size_t message_length = outer_message_length + separator_length + inner_message_length;

    char * message = malloc(message_length + 1);
    if (message == NULL)
    {
        libusbp_error_free(error);
        return &error_masked_by_no_memory;
    }

    // Assemble the message.
    vsnprintf(message, outer_message_length + 1, format, ap);
    if (separator_length)
    {
      strncpy(message + outer_message_length, "  ", separator_length + 1);
    }
    strncpy(message + outer_message_length + separator_length,
        error->message, inner_message_length + 1);
    message[message_length] = 0;

    free(error->message);
    error->message = message;
    return error;
}


// Tries to add the specified code to the error.
// This is just like error_add_v in terms of pointer ownership.
libusbp_error * error_add_code(libusbp_error * error, uint32_t code)
{
    error = error_make_mutable(error);
    if (error == NULL || error->do_not_free) { return error; }
    if (error->code_count >= SIZE_MAX / sizeof(uint32_t)) { return error; }

    size_t size = (error->code_count + 1) * sizeof(uint32_t);
    uint32_t * new_array = realloc(error->code_array, size);
    if (new_array == NULL)
    {
        libusbp_error_free(error);
        return &error_masked_by_no_memory;
    }
    error->code_array = new_array;
    error->code_array[error->code_count++] = code;
    return error;
}

// Variadic version of error_add_v.
libusbp_error * error_add(libusbp_error * error, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    error = error_add_v(error, format, ap);
    va_end(ap);
    return error;
}

// Creates a new error.
libusbp_error * error_create(const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    libusbp_error * error = error_add_v(NULL, format, ap);
    va_end(ap);
    return error;
}

bool libusbp_error_has_code(const libusbp_error * error, uint32_t code)
{
    if (error == NULL) { return false; }

    for (size_t i = 0; i < error->code_count; i++)
    {
        if (error->code_array[i] == code)
        {
            return true;
        }
    }
    return false;
}

const char * libusbp_error_get_message(const libusbp_error * error)
{
    if (error == NULL)
    {
        return "No error.";
    }
    if (error->message == NULL)
    {
        return "";
    }
    return error->message;
}
