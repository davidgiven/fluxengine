// Copyright (C) Pololu Corporation.  See www.pololu.com for details.

/*! \file libusbp.hpp
 *
 * This header files provides the C++ API for libusbp.  The classes and
 * functions here are just thin wrappers around the C API, so you should see
 * libusbp.h for full documentation. */

#pragma once

#include "libusbp.h"
#include <cstddef>
#include <utility>
#include <memory>
#include <string>
#include <vector>

/** Display a nice error if C++11 is not enabled (e.g. --std=c++11 or --std=gnu++11).
 * The __GXX_EXPERIMENTAL_CXX0X__ check is needed for GCC 4.6, which defines __cplusplus as 1.
 * The _MSC_VER check is needed for Visual Studio 2015. */
#if (!defined(__cplusplus) || (__cplusplus < 201103L)) && !defined(__GXX_EXPERIMENTAL_CXX0X__) && !defined(_MSC_VER)
#error This header requires features from C++11.
#endif

namespace libusbp
{
    /*! \cond */
    inline void throw_if_needed(libusbp_error * err);
    /*! \endcond */

    /*! Wrapper for libusbp_error_free(). */
    inline void pointer_free(libusbp_error * p) noexcept
    {
        libusbp_error_free(p);
    }

    /*! Wrapper for libusbp_error_copy(). */
    inline libusbp_error * pointer_copy(libusbp_error * p) noexcept
    {
        return libusbp_error_copy(p);
    }

    /*! Wrapper for libusbp_async_in_pipe_close(). */
    inline void pointer_free(libusbp_async_in_pipe * p) noexcept
    {
        libusbp_async_in_pipe_close(p);
    }

    /*! Wrapper for libusbp_device_free(). */
    inline void pointer_free(libusbp_device * p) noexcept
    {
        libusbp_device_free(p);
    }

    /*! Wrapper for libusbp_device_copy(). */
    inline libusbp_device * pointer_copy(libusbp_device * pointer)
    {
        libusbp_device * copy;
        throw_if_needed(libusbp_device_copy(pointer, &copy));
        return copy;
    }

    /*! Wrapper for libusbp_generic_interface_free(). */
    inline void pointer_free(libusbp_generic_interface * p) noexcept
    {
        libusbp_generic_interface_free(p);
    }

    /*! Wrapper for libusbp_generic_interface_copy(). */
    inline libusbp_generic_interface * pointer_copy(libusbp_generic_interface * pointer)
    {
        libusbp_generic_interface * copy;
        throw_if_needed(libusbp_generic_interface_copy(pointer, &copy));
        return copy;
    }

    /*! Wrapper for libusbp_generic_handle_free(). */
    inline void pointer_free(libusbp_generic_handle * p) noexcept
    {
        libusbp_generic_handle_close(p);
    }

    /*! Wrapper for libusbp_serial_port_copy(). */
    inline libusbp_serial_port * pointer_copy(libusbp_serial_port * pointer)
    {
        libusbp_serial_port * copy;
        throw_if_needed(libusbp_serial_port_copy(pointer, &copy));
        return copy;
    }

    /*! Wrapper for libusbp_serial_port_free(). */
    inline void pointer_free(libusbp_serial_port * p) noexcept
    {
        libusbp_serial_port_free(p);
    }

    /*! This class is not part of the public API of the library and you should
     * not use it directly, but you can use the public methods it provides to
     * the classes that inherit from it.
     *
     * For any type T, if you define pointer_free(T *), then
     * unique_pointer_wrapper<T> will be a well-behaved C++ class that provides
     * a constructor, implicit conversion to a bool, C++ move operations,
     * pointer operations, and forbids C++ copy operations. */
    template<class T>
    class unique_pointer_wrapper
    {
    public:
        /*! Constructor that takes a pointer. */
        explicit unique_pointer_wrapper(T * p = NULL) noexcept
            : pointer(p)
        {
        }

        /*! Move constructor. */
        unique_pointer_wrapper(unique_pointer_wrapper && other) noexcept
        {
            pointer = other.pointer_release();
        }

        /*! Move assignment operator. */
        unique_pointer_wrapper & operator=(unique_pointer_wrapper && other) noexcept
        {
            pointer_reset(other.pointer_release());
            return *this;
        }

        /*! Destructor. */
        ~unique_pointer_wrapper() noexcept
        {
            pointer_reset();
        }

        /*! Implicit conversion to bool.  Returns true if the underlying pointer
         *  is not NULL. */
        operator bool() const noexcept
        {
            return pointer != NULL;
        }

        /*! Returns the underlying pointer. */
        T * pointer_get() const noexcept
        {
            return pointer;
        }

        /*! Sets the underlying pointer to the specified value, freeing the
         * previous pointer and taking ownership of the specified one. */
        void pointer_reset(T * p = NULL) noexcept
        {
            pointer_free(pointer);
            pointer = p;
        }

        /*! Releases the pointer, transferring ownership of it to the caller and
         * resetting the underlying pointer of this object to NULL.  The caller
         * is responsible for freeing the returned pointer if it is not NULL. */
        T * pointer_release() noexcept
        {
            T * p = pointer;
            pointer = NULL;
            return p;
        }

        /*! Returns a pointer to the underlying pointer. */
        T ** pointer_to_pointer_get() noexcept
        {
            return &pointer;
        }

        /*! Copy constructor: forbid. */
        unique_pointer_wrapper(const unique_pointer_wrapper & other) = delete;

        /*! Copy assignment operator: forbid. */
        unique_pointer_wrapper & operator=(const unique_pointer_wrapper & other) = delete;

    protected:
        /*! The underlying pointer that is being wrapped.  This pointer will be
         * freed when the object is destroyed. */
        T * pointer;
    };

    /*! This class is not part of the public API of the library and you should
     * not use it directly, but you can use the public methods it provides to
     * the classes that inherit from it.
     *
     * For any type T, if you define pointer_free(T *) and pointer_copy(T *), then
     * unique_pointer_wrapper_with_copy<T> will be a well-behaved C++ class that provides
     * a constructor, implicit conversion to a bool, C++ move operations, C++ copy operations,
     * and pointer operations. */
    template <class T>
    class unique_pointer_wrapper_with_copy : public unique_pointer_wrapper<T>
    {
    public:
        /*! Constructor that takes a pointer. */
        explicit unique_pointer_wrapper_with_copy(T * p = NULL) noexcept
            : unique_pointer_wrapper<T>(p)
        {
        }

        /*! Move constructor. */
        unique_pointer_wrapper_with_copy(
            unique_pointer_wrapper_with_copy && other) = default;

        /*! Copy constructor */
        unique_pointer_wrapper_with_copy(
            const unique_pointer_wrapper_with_copy & other)
            : unique_pointer_wrapper<T>()
        {
            this->pointer = pointer_copy(other.pointer);
        }

        /*! Copy assignment operator. */
        unique_pointer_wrapper_with_copy & operator=(
          const unique_pointer_wrapper_with_copy & other)
        {
            this->pointer_reset(pointer_copy(other.pointer));
            return *this;
        }

        /*! Move assignment operator. */
        unique_pointer_wrapper_with_copy & operator=(
            unique_pointer_wrapper_with_copy && other) = default;
    };

    /*! Wrapper for a ::libusbp_error pointer. */
    class error : public unique_pointer_wrapper_with_copy<libusbp_error>, public std::exception
    {
    public:
        /*! Constructor that takes a pointer.  */
        explicit error(libusbp_error * p = NULL) noexcept
            : unique_pointer_wrapper_with_copy(p)
        {
        }

        /*! Wrapper for libusbp_error_get_message(). */
        virtual const char * what() const noexcept
        {
            return libusbp_error_get_message(pointer);
        }

        /*! Wrapper for libusbp_error_get_message() that returns a
         *  std::string. */
        std::string message() const
        {
            return what();
        }

        /*! Wrapper for libusbp_error_has_code(). */
        bool has_code(uint32_t error_code) const noexcept
        {
            return libusbp_error_has_code(pointer, error_code);
        }
    };

    /*! \cond */
    inline void throw_if_needed(libusbp_error * err)
    {
        if (err != NULL)
        {
            throw error(err);
        }
    }
    /*! \endcond */

    /*! Wrapper for a ::libusbp_async_in_pipe pointer. */
    class async_in_pipe : public unique_pointer_wrapper<libusbp_async_in_pipe>
    {
    public:
        /*! Constructor that takes a pointer. */
        explicit async_in_pipe(libusbp_async_in_pipe * pointer = NULL)
            : unique_pointer_wrapper(pointer)
        {
        }

        /*! Wrapper for libusbp_async_in_pipe_allocate_transfers(). */
        void allocate_transfers(size_t transfer_count, size_t transfer_size)
        {
            throw_if_needed(libusbp_async_in_pipe_allocate_transfers(
                pointer, transfer_count, transfer_size));
        }

        /*! Wrapper for libusbp_async_in_pipe_start_endless_transfers(). */
        void start_endless_transfers()
        {
            throw_if_needed(libusbp_async_in_pipe_start_endless_transfers(pointer));
        }

        /*! Wrapper for libusbp_async_in_pipe_handle_events(). */
        void handle_events()
        {
            throw_if_needed(libusbp_async_in_pipe_handle_events(pointer));
        }

        /*! Wrapper for libusbp_async_in_pipe_has_pending_transfers(). */
        bool has_pending_transfers()
        {
            bool result;
            throw_if_needed(libusbp_async_in_pipe_has_pending_transfers(pointer, &result));
            return result;
        }

        /*! Wrapper for libusbp_async_in_pipe_handle_finished_transfer(). */
        bool handle_finished_transfer(void * buffer, size_t * transferred,
            error * transfer_error)
        {
            libusbp_error ** error_out = NULL;
            if (transfer_error != NULL)
            {
                transfer_error->pointer_reset();
                error_out = transfer_error->pointer_to_pointer_get();
            }

            bool finished;
            throw_if_needed(libusbp_async_in_pipe_handle_finished_transfer(
                pointer, &finished, buffer, transferred, error_out));
            return finished;
        }

        /*! Wrapper for libusbp_async_in_pipe_cancel_transfers(). */
        void cancel_transfers()
        {
            throw_if_needed(libusbp_async_in_pipe_cancel_transfers(pointer));
        }
    };

    /*! Wrapper for a ::libusbp_device pointer. */
    class device : public unique_pointer_wrapper_with_copy<libusbp_device>
    {
    public:
        /*! Constructor that takes a pointer. */
        explicit device(libusbp_device * pointer = NULL) :
            unique_pointer_wrapper_with_copy(pointer)
        {
        }

        /*! Wrapper for libusbp_device_get_vendor_id(). */
        uint16_t get_vendor_id() const
        {
            uint16_t id;
            throw_if_needed(libusbp_device_get_vendor_id(pointer, &id));
            return id;
        }

        /*! Wrapper for libusbp_device_get_product_id(). */
        uint16_t get_product_id() const
        {
            uint16_t id;
            throw_if_needed(libusbp_device_get_product_id(pointer, &id));
            return id;
        }

        /*! Wrapper for libusbp_device_get_revision(). */
        uint16_t get_revision() const
        {
            uint16_t r;
            throw_if_needed(libusbp_device_get_revision(pointer, &r));
            return r;
        }

        /*! Wrapper for libusbp_device_get_serial_number(). */
        std::string get_serial_number() const
        {
            char * str;
            throw_if_needed(libusbp_device_get_serial_number(pointer, &str));
            std::string serial_number = str;
            libusbp_string_free(str);
            return serial_number;
        }

        /*! Wrapper for libusbp_device_get_os_id(). */
        std::string get_os_id() const
        {
            char * str;
            throw_if_needed(libusbp_device_get_os_id(pointer, &str));
            std::string serial_number = str;
            libusbp_string_free(str);
            return serial_number;
        }
    };

    /*! Wrapper for libusbp_list_connected_devices(). */
    inline std::vector<libusbp::device> list_connected_devices()
    {
        libusbp_device ** device_list;
        size_t size;
        throw_if_needed(libusbp_list_connected_devices(&device_list, &size));
        std::vector<device> vector;
        for(size_t i = 0; i < size; i++)
        {
            vector.push_back(device(device_list[i]));
        }
        libusbp_list_free(device_list);
        return vector;
    }

    /*! Wrapper for libusbp_find_device_with_vid_pid(). */
    inline libusbp::device find_device_with_vid_pid(uint16_t vendor_id, uint16_t product_id)
    {
        libusbp_device * device_pointer;
        throw_if_needed(libusbp_find_device_with_vid_pid(
                vendor_id, product_id, &device_pointer));
        return device(device_pointer);
    }

    /*! Wrapper for a ::libusbp_generic_interface pointer. */
    class generic_interface : public unique_pointer_wrapper_with_copy<libusbp_generic_interface>
    {
    public:
        /*! Constructor that takes a pointer.  This object will free the pointer
         *  when it is destroyed. */
        explicit generic_interface(libusbp_generic_interface * pointer = NULL)
            : unique_pointer_wrapper_with_copy(pointer)
        {
        }

        /*! Wrapper for libusbp_generic_interface_create. */
        generic_interface(const device & device,
            uint8_t interface_number = 0, bool composite = false)
        {
            throw_if_needed(libusbp_generic_interface_create(
                    device.pointer_get(), interface_number, composite, &pointer));
        }

        /*! Wrapper for libusbp_generic_interface_get_os_id(). */
        std::string get_os_id() const
        {
            char * str;
            throw_if_needed(libusbp_generic_interface_get_os_id(pointer, &str));
            std::string id = str;
            libusbp_string_free(str);
            return id;
        }

        /*! Wrapper for libusbp_generic_interface_get_os_filename(). */
        std::string get_os_filename() const
        {
            char * str;
            throw_if_needed(libusbp_generic_interface_get_os_filename(pointer, &str));
            std::string filename = str;
            libusbp_string_free(str);
            return filename;
        }
    };

    /*! Wrapper for a ::libusbp_generic_handle pointer. */
    class generic_handle : public unique_pointer_wrapper<libusbp_generic_handle>
    {
    public:
        /*! Constructor that takes a pointer.  This object will free the pointer
         *  when it is destroyed. */
        explicit generic_handle(libusbp_generic_handle * pointer = NULL) noexcept
            : unique_pointer_wrapper(pointer)
        {
        }

        /*! Wrapper for libusbp_generic_handle_open(). */
        generic_handle(const generic_interface & gi)
        {
            throw_if_needed(libusbp_generic_handle_open(gi.pointer_get(), &pointer));
        }

        /*! Wrapper for libusbp_generic_handle_close(). */
        void close() noexcept
        {
            pointer_reset();
        }

        /*! Wrapper for libusbp_generic_handle_open_async_in_pipe(). */
        async_in_pipe open_async_in_pipe(uint8_t pipe_id)
        {
            libusbp_async_in_pipe * pipe;
            throw_if_needed(libusbp_generic_handle_open_async_in_pipe(
                pointer, pipe_id, &pipe));
            return async_in_pipe(pipe);
        }

        /*! Wrapper for libusbp_generic_handle_set_timeout(). */
        void set_timeout(uint8_t pipe_id, uint32_t timeout)
        {
            throw_if_needed(libusbp_generic_handle_set_timeout(pointer, pipe_id, timeout));
        }

        /*! Wrapper for libusbp_control_transfer(). */
        void control_transfer(
            uint8_t bmRequestType,
            uint8_t bRequest,
            uint16_t wValue,
            uint16_t wIndex,
            void * buffer = NULL,
            uint16_t wLength = 0,
            size_t * transferred = NULL)
        {
            throw_if_needed(libusbp_control_transfer(pointer,
                bmRequestType, bRequest, wValue, wIndex,
                buffer, wLength, transferred));
        }

        /*! Wrapper for libusbp_write_pipe(). */
        void write_pipe(uint8_t pipe_id, const void * buffer,
            size_t size, size_t * transferred)
        {
            throw_if_needed(libusbp_write_pipe(pointer,
                pipe_id, buffer, size, transferred));
        }

        /*! Wrapper for libusbp_read_pipe(). */
        void read_pipe(uint8_t pipe_id, void * buffer,
            size_t size, size_t * transferred)
        {
            throw_if_needed(libusbp_read_pipe(pointer,
                pipe_id, buffer, size, transferred));
        }

        #ifdef _WIN32
        /*! Wrapper for libusbp_generic_handle_get_winusb_handle(). */
        HANDLE get_winusb_handle()
        {
            return libusbp_generic_handle_get_winusb_handle(pointer);
        }
        #endif

        #ifdef __linux__
        /*! Wrapper for libusbp_generic_handle_get_fd(). */
        int get_fd()
        {
            return libusbp_generic_handle_get_fd(pointer);
        }
        #endif

        #ifdef __APPLE__
        /*! Wrapper for libusbp_generic_handle_get_cf_plug_in(). */
        void ** get_cf_plug_in()
        {
            return libusbp_generic_handle_get_cf_plug_in(pointer);
        }
        #endif
    };

    /*! Wrapper for a ::libusbp_serial_port pointer. */
    class serial_port : public unique_pointer_wrapper_with_copy<libusbp_serial_port>
    {
    public:
        /*! Constructor that takes a pointer.  This object will free the pointer
         *  when it is destroyed. */
        explicit serial_port(libusbp_serial_port * pointer = NULL)
            : unique_pointer_wrapper_with_copy(pointer)
        {
        }

        /*! Wrapper for libusbp_serial_port_create(). */
        serial_port(const device & device,
            uint8_t interface_number = 0, bool composite = false)
        {
            throw_if_needed(libusbp_serial_port_create(
                    device.pointer_get(), interface_number, composite, &pointer));
        }

        /*! Wrapper for libusbp_serial_port_get_name(). */
        std::string get_name() const
        {
            char * str;
            throw_if_needed(libusbp_serial_port_get_name(pointer, &str));
            std::string id = str;
            libusbp_string_free(str);
            return id;
        }
    };
}

