#pragma once

#include <string>
#include <unordered_map>

#include "inja/environment.hpp"

namespace {

inja::Environment GetInjaEnvironment() {
  inja::Environment environment;
  environment.set_expression("${", "}");
  environment.set_html_autoescape(false);
  return environment;
}

}  // namespace

namespace devkit::TemplateUtils {

inline std::string RenderTemplate(const std::string& templateString,
                                  const std::unordered_map<std::string, std::string>& env) {
  static inja::Environment injaEnvironment = GetInjaEnvironment();

  return injaEnvironment.render(templateString, env);
}

}  // namespace devkit::TemplateUtils