#include <libusbp_internal.h>

// Simple wrapper around strdup to make a copy of a string, either for internal
// use or for returning the string to the user.
libusbp_error * string_copy(const char * input_string, char ** output_string)
{
    assert(input_string != NULL);
    assert(output_string != NULL);

    *output_string = NULL;

    char * new_string = strdup(input_string);
    if (new_string == NULL)
    {
        return &error_no_memory;
    }

    *output_string = new_string;
    return NULL;
}

void libusbp_string_free(char * string)
{
    free(string);
}
