#ifndef CHAIN_MODEL_HPP
#define CHAIN_MODEL_HPP

#include <string>
#include <vector>
#include <optional>
#include "../include/json.hpp"

namespace Model {

using json = nlohmann::json;

struct Rpc {
    std::string url;
    std::string tracking;
    bool isOpenSource = false;
};

struct NativeCurrency {
    std::string name;
    std::string symbol;
    int decimals;
};

struct Explorer {
    std::string name;
    std::string url;
    std::string standard;
    std::string icon;
};

struct ChainConfig {
    std::string name;
    std::string chain;
    std::string icon;
    std::vector<Rpc> rpc;
    std::vector<Explorer> explorers;
    NativeCurrency nativeCurrency;
    std::string infoURL;
    std::string shortName;
    uint64_t chainId;
    uint64_t networkId;
    bool isTestnet = false;
    std::string chainSlug;
};

// JSON conversions
inline void from_json(const json& j, Rpc& r) {
    r.url = j.value("url", "");
    r.tracking = j.value("tracking", "");
    r.isOpenSource = j.value("isOpenSource", false);
}

inline void from_json(const json& j, NativeCurrency& n) {
    n.name = j.value("name", "");
    n.symbol = j.value("symbol", "");
    n.decimals = j.value("decimals", 18);
}

inline void from_json(const json& j, Explorer& e) {
    e.name = j.value("name", "");
    e.url = j.value("url", "");
    e.standard = j.value("standard", "");
    e.icon = j.value("icon", "");
}

inline void from_json(const json& j, ChainConfig& c) {
    c.name = j.value("name", "Unknown");
    c.chain = j.value("chain", "");
    
    // Handle icon which can be string or object
    if (j.contains("icon")) {
        const auto& icon = j["icon"];
        if (icon.is_string()) {
            c.icon = icon.get<std::string>();
        } else if (icon.is_object() && icon.contains("url")) {
            c.icon = icon["url"].get<std::string>();
        }
    }
    
    if (j.contains("rpc")) {
        c.rpc = j.at("rpc").get<std::vector<Rpc>>();
    }
    
    if (j.contains("explorers")) {
        c.explorers = j.at("explorers").get<std::vector<Explorer>>();
    }
    
    if (j.contains("nativeCurrency")) {
        c.nativeCurrency = j.at("nativeCurrency").get<NativeCurrency>();
    }
    
    c.infoURL = j.value("infoURL", "");
    c.shortName = j.value("shortName", "");
    c.chainId = j.value("chainId", 0ULL);
    c.networkId = j.value("networkId", 0ULL);
    c.isTestnet = j.value("isTestnet", false);
    c.chainSlug = j.value("chainSlug", "");
}

} // namespace Model

#endif // CHAIN_MODEL_HPP
