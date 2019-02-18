#ifndef BYTES_H
#define BYTES_H

template <class T>
uint32_t read_be32(T ptr)
{
    return (ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | ptr[3];
}

#endif
