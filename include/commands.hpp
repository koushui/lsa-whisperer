#pragma once
#include <cloudap.hpp>
#include <kerberos.hpp>
#include <negotiate.hpp>
#include <msv1_0.hpp>
#include <pku2u.hpp>
#include <schannel.hpp>
#include <wdigest.hpp>

// Disable warnings generated by cxxopts
#pragma warning(disable : 6495)
#include <cxxopts.hpp>

namespace Cloudap {
    bool HandleFunction(std::ostream& out, const Proxy& proxy, const std::string& function, const cxxopts::ParseResult& options);
    void Parse(std::ostream& out, const std::vector<std::string>& args);
}

namespace Kerberos {
    bool HandleFunction(std::ostream& out, const Proxy& proxy, const std::string& function, const cxxopts::ParseResult& options);
    void Parse(std::ostream& out, const std::vector<std::string>& args);
}

namespace Msv1_0 {
    bool HandleFunction(std::ostream& out, const Proxy& proxy, const std::string& function, const cxxopts::ParseResult& options);
    void Parse(std::ostream& out, const std::vector<std::string>& args);
}

namespace Negotiate {
    bool HandleFunction(std::ostream& out, const Proxy& proxy, const std::string& function, const cxxopts::ParseResult& options);
    void Parse(std::ostream& out, const std::vector<std::string>& args);
}

namespace Pku2u {
    bool HandleFunction(std::ostream& out, const Proxy& proxy, const std::string& function, const cxxopts::ParseResult& options);
    void Parse(std::ostream& out, const std::vector<std::string>& args);
}

namespace Schannel {
    bool HandleFunction(std::ostream& out, const Proxy& proxy, const std::string& function, const cxxopts::ParseResult& options);
    void Parse(std::ostream& out, const std::vector<std::string>& args);
}

namespace Wdigest {
    bool HandleFunction(std::ostream& out, const Proxy& proxy, const cxxopts::ParseResult& options);
    void Parse(std::ostream& out, const std::vector<std::string>& args);
}