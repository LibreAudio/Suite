/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_FIFO_HPP_INCLUDED
#define DISTRHO_FIFO_HPP_INCLUDED

#include "DistrhoUtils.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

/**
  Data struct holding a FIFO (first-in, first-out), containing an arbitrary amount of elements of an arbitrary type.
 */
template <typename T, uint32_t numElements>
struct Fifo {
   /**
      Fifo buffer data.
    */
    T buffer[numElements];

   /**
      Current reading position.
      Increments when reading.
    */
    uint32_t readPosition;

   /**
      Current writing position.
      Increments when writing.
    */
    uint32_t writePosition;
};

// --------------------------------------------------------------------------------------------------------------------

/**
   DPF built-in Fifo class.
   FloatFifoControl takes one fifo struct to take control over, and operates over it.

   This is meant for single-writer, single-reader type of control.
   Writing and reading is wait and lock-free.

   Typically usage involves:
   ```
   // definition
   FloatFifo<32> fifoData;
   FloatFifoControl<32> fifo;

   // assign fifo and clear data
   fifo.setFifo(&fifoData, true);

   // writing data
   fifo.write(0.0f);
   fifo.write(0.5f);
   fifo.write(1.0f);

   // reading data
   if (fifo.canRead())
   {
      const float value = fifo.read();
      // do something with "value"
   }
   ```

   @see Fifo
 */
template <typename T, uint32_t numElements>
class FifoControl
{
public:
    /*
     * Constructor for unitialized fifo.
     * A call to setFifo is required to tied this control to a fifo struct;
     *
     */
    FifoControl()
        : fifoPtr(nullptr) {}

    /*
     * Destructor.
     */
    ~FifoControl() {}

    // ----------------------------------------------------------------------------------------------------------------
    // check operations

    inline bool canRead() const noexcept
    {
        return fifoPtr != nullptr && fifoPtr->readPosition != fifoPtr->writePosition;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // clear/reset operations

    /*
     * Clear the entire fifo data, marking the fifo as empty.
     * Requires a fifo struct tied to this class.
     */
    void clearData() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fifoPtr != nullptr,);

        fifoPtr->readPosition = fifoPtr->writePosition = 0;
        std::memset(fifoPtr->buffer, 0, sizeof(T) * numElements);
    }

    // ----------------------------------------------------------------------------------------------------------------

    /*
     * Tie this fifo control to a fifo struct, optionally clearing its data.
     */
    void setFifo(Fifo<T, numElements>* const fifo, const bool clearFifoData = true) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fifoPtr != fifo,);

        fifoPtr = fifo;

        if (clearFifoData && fifo != nullptr)
            clearData();
    }

    // ----------------------------------------------------------------------------------------------------------------

    /*
     * Read one single element.
     */
    T read()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fifoPtr != nullptr, {});

        uint32_t readPosition = fifoPtr->readPosition;
        const T ret = *(fifoPtr->buffer + readPosition);

        if (++readPosition == numElements)
            readPosition = 0;

        fifoPtr->readPosition = readPosition;
        return ret;
    }

    /*
     * Write one single element.
     */
    void write(const T value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fifoPtr != nullptr,);

        uint32_t writePosition = fifoPtr->writePosition;

        *(fifoPtr->buffer + writePosition) = value;

        if (++writePosition == numElements)
            writePosition = 0;

        fifoPtr->writePosition = writePosition;
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    /** Fifo struct pointer. */
    Fifo<T, numElements>* fifoPtr;

    DISTRHO_PREVENT_VIRTUAL_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPYABLE(FifoControl)
};

// --------------------------------------------------------------------------------------------------------------------
// export float type

template <uint32_t numElements>
using FloatFifo = Fifo<float, numElements>;

template <uint32_t numElements>
using FloatFifoControl = FifoControl<float, numElements>;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_FIFO_HPP_INCLUDED
