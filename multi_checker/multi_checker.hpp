#ifndef MULTI_CHECKER_HPP
#define MULTI_CHECKER_HPP

#include <cstdint>
#include <string>
#include <vector>

/**
 * Result of checking an address on a single chain
 */
struct ChainResult {
    uint64_t chain_id;
    std::string chain_name;
    std::string symbol;
    std::string balance_eth;
    uint64_t tx_count;
    bool is_contract;
    bool has_activity;  // balance > 0 OR tx_count > 0
};

namespace MultiChainChecker {

/**
 * Scan an address across all available chains
 * @param address Ethereum address to check
 * @param include_testnets If true, also scan testnet chains
 * @param only_with_activity If true, only return chains with balance > 0 or tx_count > 0
 * @return Vector of ChainResult for each chain checked
 */
std::vector<ChainResult> scan_all(const std::string& address, 
                                   bool include_testnets = false,
                                   bool only_with_activity = true);

/**
 * Print scan results in a formatted table
 * @param results Vector of ChainResult to display
 */
void print_results(const std::vector<ChainResult>& results);

} // namespace MultiChainChecker

#endif // MULTI_CHECKER_HPP
