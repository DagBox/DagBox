#pragma once

#include <stdexcept>

/*! \brief Declare an exception class.
 *
 * Declares an exception class with the name `name` that inherits from
 * `base` class. The declared class will retain all constructors of
 * the base class.
 */
#define EXCEPTION(name, base)                        \
    class name                                       \
        : public base                                \
    {using base::base;}



/*! \brief General exceptions.
 */
namespace exception
{
    using std::runtime_error;

    /*! \brief An unexpected error that can't be recovered from.
     *
     * This exception is used for things that shouldn't happen under
     * normal conditions, such as a memort allocation failing.
     */
    EXCEPTION(fatal, runtime_error);
}
