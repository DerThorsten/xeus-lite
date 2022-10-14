/* Copyright (c) 2021, Thorsten Beier                                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include "xmock_interpreter.hpp"
#include "xeus-lite/xembind.hpp"


EMSCRIPTEN_BINDINGS(my_module)
{
    xeus::export_core();
    using interpreter_type = xeus::xmock_interpreter;
    xeus::export_kernel<interpreter_type>("xkernel");
}
