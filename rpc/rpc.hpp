#ifndef RPC_HPP
#define RPC_HPP

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

/**
 * Address information from RPC queries
 */
struct AddressInfo {
    std::string balance_wei;     // Balance in wei (hex string)
    std::string balance_eth;     // Balance in ETH (human readable)
    uint64_t tx_count;           // Transaction count (nonce)
    bool has_token_activity;     // Has ERC20 transfer events
    bool is_contract;            // Is a contract address
};

namespace RpcClient {

/**
 * Get ETH balance of an address
 * @param rpc_url RPC endpoint URL
 * @param address Ethereum address (0x...)
 * @return Balance in wei as hex string, or empty on error
 */
std::optional<std::string> get_balance(const std::string& rpc_url, const std::string& address);

/**
 * Get transaction count (nonce) of an address
 * @param rpc_url RPC endpoint URL
 * @param address Ethereum address (0x...)
 * @return Transaction count, or nullopt on error
 */
std::optional<uint64_t> get_transaction_count(const std::string& rpc_url, const std::string& address);

/**
 * Check if address has ERC20 token activity
 * Uses eth_getLogs to check for Transfer events
 * @param rpc_url RPC endpoint URL
 * @param address Ethereum address (0x...)
 * @return true if has token transfers, false otherwise
 */
bool has_token_activity(const std::string& rpc_url, const std::string& address);

/**
 * Check if address is a contract
 * @param rpc_url RPC endpoint URL
 * @param address Ethereum address (0x...)
 * @return true if contract, false if EOA
 */
bool is_contract(const std::string& rpc_url, const std::string& address);

/**
 * Get address info (balance, tx count)
 * @param rpc_url RPC endpoint URL
 * @param address Ethereum address (0x...)
 * @return AddressInfo struct with balance and tx count
 */
AddressInfo check_address(const std::string& rpc_url, const std::string& address);

/**
 * Convert wei hex string to ETH decimal string
 * @param wei_hex Wei value as hex string (e.g., "0x1234")
 * @return Human-readable ETH value
 */
std::string wei_to_eth(const std::string& wei_hex);

} // namespace RpcClient

#endif // RPC_HPP
