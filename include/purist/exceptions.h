#pragma once

#include <stdexcept>

namespace purist {

class errcode_exception : public std::runtime_error {
public:
    const int errcode;
    const std::string message;

    errcode_exception(int errcode, const std::string& message) : 
        std::runtime_error("System error " + std::to_string(errcode) + ": " + message), 
        errcode(errcode), 
        message(message) { }
};

class cant_get_connector_exception : public errcode_exception {
public:
    cant_get_connector_exception(int errcode, const std::string& message) : errcode_exception(errcode, message) { }
};

}