#ifndef CHAIN_HPP
#define CHAIN_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

struct Chain {
    uint64_t chain_id;
    std::string name;
    std::string symbol;
    std::vector<std::string> rpc_urls;
    std::string explorer_url;
    bool is_testnet;
};

namespace ChainRegistry {
    /**
     * Get chain by chain ID
     */
    std::optional<Chain> get_by_id(uint64_t chain_id);
    
    /**
     * Get chain by name (case-insensitive)
     */
    std::optional<Chain> get_by_name(const std::string& name);
    
    /**
     * Get all chains
     */
    const std::vector<Chain>& get_all();
    
    /**
     * Get number of supported chains
     */
    size_t count();

    /**
     * Initialize chain registry (fetch RPCs)
     * @param force_update If true, force download of RPCs even if file exists
     */
    void init(bool force_update = false);
}

#endif /* CHAIN_HPP */
