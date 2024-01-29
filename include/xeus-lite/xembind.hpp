/***************************************************************************
* Copyright (c) 2021, Thorsten Beier                                       *
* Copyright (c) 2022, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XEUS_LITE_XEMBIND_HPP
#define XEUS_LITE_XEMBIND_HPP

#include <string>

#include "xeus/xkernel.hpp"
#include "xeus/xeus_context.hpp"
#include "xeus/xinterpreter.hpp"
#include "xeus/xmessage.hpp"

#include "xserver_emscripten.hpp"

namespace nl = nlohmann;
namespace ems = emscripten;

namespace xeus
{

    void buffer_sequence_from_js_buffer(buffer_sequence& self, ems::val buffers);
    xmessage xmessage_from_js_message(ems::val js_message);
    ems::val js_message_from_xmessage(const xmessage & message, bool copy);
    ems::val js_message_from_xmessage(const xpub_message & message, bool copy);

    void export_server_emscripten();
    void export_core();

    xeus::xserver * get_server(xeus::xkernel * kernel);

    template<class interpreter_type>
    std::unique_ptr<xkernel> make_xkernel()
    {
        xeus::xconfiguration config;

        using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;
        history_manager_ptr hist = xeus::make_in_memory_history_manager();

        using interpreter_ptr = std::unique_ptr<interpreter_type>;
        
        auto interpreter = interpreter_ptr(new interpreter_type());
        auto context = xeus::make_empty_context();

        xeus::xkernel * kernel = new xeus::xkernel(config,
                             xeus::get_user_name(),
                             std::move(context),
                             std::move(interpreter),
                             xeus::make_xserver_emscripten,
                             std::move(hist),
                             nullptr
                             );
        return std::unique_ptr<xkernel>{kernel};
    }

    template<class interpreter_type>
    std::unique_ptr<xkernel> make_parametrized_xkernel(
        nl::json parameters
    )
    {
        xeus::xconfiguration config;

        using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;
        history_manager_ptr hist = xeus::make_in_memory_history_manager();

        using interpreter_ptr = std::unique_ptr<interpreter_type>;
        
        auto interpreter = interpreter_ptr(new interpreter_type(parameters));
        auto context = xeus::make_empty_context();

        xeus::xkernel * kernel = new xeus::xkernel(config,
                             xeus::get_user_name(),
                             std::move(context),
                             std::move(interpreter),
                             xeus::make_xserver_emscripten,
                             std::move(hist),
                             nullptr
                             );
        return std::unique_ptr<xkernel>{kernel};
    }



    template<class interpreter_type, class f_extra_type>
    void export_kernel_core(const std::string kernel_name, f_extra_type && f_extra)
    {
        auto kernel_js_class = ems::class_<xkernel>(kernel_name.c_str());

        kernel_js_class
            .constructor<>(&make_xkernel<interpreter_type>)
            .function("get_server", &get_server, ems::allow_raw_pointers())
            .function("start", &xkernel::start)
        ;
        // kernels may export additional functions
        f_extra(kernel_js_class);


        // we need a helper function to translate exception ptrs 
        // to the actual exception message since on the JS side,
        // we cannot access the exception message directly.
        ems::function("get_exception_message", 
            ems::select_overload<std::string(int)>([](int exceptionPtr) {
            return std::string(reinterpret_cast<std::exception *>(exceptionPtr)->what());
        }));
    }


    template<class interpreter_type>
    void export_kernel(const std::string kernel_name)
    {
        export_kernel_core<interpreter_type>(kernel_name, [](auto & kernel_js_class){});
    }


    template<class interpreter_type, class f_extra_type>
    void export_kernel(const std::string kernel_name, f_extra_type && f_extra)
    {
        export_kernel_core<interpreter_type>(kernel_name, std::forward<f_extra_type>(f_extra));
    }

    template<class interpreter_type, class f_extra_type>
    void export_parameterizable_kernel(const std::string kernel_name, f_extra_type && f_extra)
    {
        export_kernel_core<interpreter_type>(kernel_name,  [&](auto & kernel_js_class){

            // extra constructor for parametrized kernels
            kernel_js_class
                . template constructor<nl::json>(&make_parametrized_xkernel<interpreter_type>)
            ;

            // kernels may export additional functions
            f_extra(kernel_js_class);
        });
    }

    template<class interpreter_type, class f_extra_type>
    void export_parameterizable_kernel(const std::string kernel_name)
    {
        export_parameterizable_kernel<interpreter_type>(kernel_name, [](auto & kernel_js_class){});
    }

}

#endif
