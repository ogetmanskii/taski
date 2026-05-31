#pragma once

#include "inja/inja.hpp"

namespace devkit {

    static inline inja::Environment GetInjaEnvironment() {
        inja::Environment injaEnvironment;
        injaEnvironment.set_expression("${", "}");
        injaEnvironment.set_html_autoescape(false);
        return injaEnvironment;
    }

}