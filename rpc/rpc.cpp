#include "rpc.hpp"
#include "../include/json.hpp"
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <array>
#include <memory>
#include <iostream>

using json = nlohmann::json;

namespace {

// Execute curl command and return output
std::string exec_curl(const std::string& cmd) {
    std::array<char, 4096> buffer;
    std::string result;
    
    auto pipe_deleter = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipe_deleter)> pipe(popen(cmd.c_str(), "r"), pipe_deleter);
    if (!pipe) {
        return "";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

// Make JSON-RPC request
std::optional<json> json_rpc_call(const std::string& rpc_url, 
                                  const std::string& method, 
                                  const json& params) {
    json request = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params},
        {"id", 1}
    };
    
    std::string json_str = request.dump();
    
    // Escape quotes for shell
    std::string escaped;
    for (char c : json_str) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else escaped += c;
    }
    
    std::string cmd = "curl -s -X POST -H \"Content-Type: application/json\" "
                      "-d \"" + escaped + "\" \"" + rpc_url + "\" 2>/dev/null";
    
    std::string response = exec_curl(cmd);
    
    if (response.empty()) {
        return std::nullopt;
    }
    
    try {
        json result = json::parse(response);
        if (result.contains("result")) {
            return result["result"];
        }
    } catch (...) {
        // Parse error
    }
    
    return std::nullopt;
}

// Convert hex string to uint64_t
uint64_t hex_to_uint64(const std::string& hex) {
    if (hex.empty()) return 0;
    
    std::string h = hex;
    if (h.substr(0, 2) == "0x" || h.substr(0, 2) == "0X") {
        h = h.substr(2);
    }
    
    try {
        return std::stoull(h, nullptr, 16);
    } catch (...) {
        return 0;
    }
}

} // anonymous namespace

namespace RpcClient {

std::optional<std::string> get_balance(const std::string& rpc_url, const std::string& address) {
    json params = json::array({address, "latest"});
    auto result = json_rpc_call(rpc_url, "eth_getBalance", params);
    
    if (result && result->is_string()) {
        return result->get<std::string>();
    }
    
    return std::nullopt;
}

std::optional<uint64_t> get_transaction_count(const std::string& rpc_url, const std::string& address) {
    json params = json::array({address, "latest"});
    auto result = json_rpc_call(rpc_url, "eth_getTransactionCount", params);
    
    if (result && result->is_string()) {
        return hex_to_uint64(result->get<std::string>());
    }
    
    return std::nullopt;
}

bool has_token_activity(const std::string& rpc_url, const std::string& address) {
    // ERC20 Transfer event topic: keccak256("Transfer(address,address,uint256)")
    // = 0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef
    const std::string transfer_topic = "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef";
    
    // Pad address to 32 bytes (topic format)
    std::string addr = address;
    if (addr.substr(0, 2) == "0x") {
        addr = addr.substr(2);
    }
    // Pad to 64 chars (32 bytes)
    while (addr.length() < 64) {
        addr = "0" + addr;
    }
    addr = "0x" + addr;
    
    // Check for transfers TO this address (receiving tokens)
    json params = json::array({
        json::object({
            {"fromBlock", "earliest"},
            {"toBlock", "latest"},
            {"topics", json::array({
                transfer_topic,
                nullptr,  // from any address
                addr      // to this address
            })}
        })
    });
    
    auto result = json_rpc_call(rpc_url, "eth_getLogs", params);
    
    if (result && result->is_array() && !result->empty()) {
        return true;
    }
    
    // Also check for transfers FROM this address (sending tokens)
    params = json::array({
        json::object({
            {"fromBlock", "earliest"},
            {"toBlock", "latest"},
            {"topics", json::array({
                transfer_topic,
                addr,     // from this address
                nullptr   // to any address
            })}
        })
    });
    
    result = json_rpc_call(rpc_url, "eth_getLogs", params);
    
    if (result && result->is_array() && !result->empty()) {
        return true;
    }
    
    return false;
}

bool is_contract(const std::string& rpc_url, const std::string& address) {
    json params = json::array({address, "latest"});
    auto result = json_rpc_call(rpc_url, "eth_getCode", params);
    
    if (result && result->is_string()) {
        std::string code = result->get<std::string>();
        // If code is "0x" or empty, it's not a contract (EOA)
        return code.length() > 2;
    }
    
    return false;
}

std::string wei_to_eth(const std::string& wei_hex) {
    if (wei_hex.empty() || wei_hex == "0x0") {
        return "0";
    }
    
    std::string hex = wei_hex;
    if (hex.substr(0, 2) == "0x" || hex.substr(0, 2) == "0X") {
        hex = hex.substr(2);
    }
    
    // Convert hex to decimal string (simple method for display)
    // For large numbers, we do integer division
    __uint128_t value = 0;
    for (char c : hex) {
        value *= 16;
        if (c >= '0' && c <= '9') value += c - '0';
        else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') value += c - 'A' + 10;
    }
    
    // Convert to ETH (divide by 10^18)
    constexpr __uint128_t ETH_DECIMALS = 1000000000000000000ULL;  // 10^18
    
    __uint128_t whole = value / ETH_DECIMALS;
    __uint128_t fraction = value % ETH_DECIMALS;
    
    // Convert whole part to string
    std::string whole_str;
    if (whole == 0) {
        whole_str = "0";
    } else {
        while (whole > 0) {
            whole_str = char('0' + (whole % 10)) + whole_str;
            whole /= 10;
        }
    }
    
    // Convert fraction part to string (with leading zeros)
    std::string frac_str;
    for (int i = 0; i < 18; i++) {
        frac_str = char('0' + (fraction % 10)) + frac_str;
        fraction /= 10;
    }
    
    // Trim trailing zeros from fraction
    while (frac_str.length() > 1 && frac_str.back() == '0') {
        frac_str.pop_back();
    }
    
    if (frac_str == "0") {
        return whole_str;
    }
    
    return whole_str + "." + frac_str;
}

AddressInfo check_address(const std::string& rpc_url, const std::string& address) {
    AddressInfo info;
    info.tx_count = 0;
    info.has_token_activity = false;
    info.is_contract = false;
    
    // Get balance
    auto balance = get_balance(rpc_url, address);
    if (balance) {
        info.balance_wei = *balance;
        info.balance_eth = wei_to_eth(*balance);
    } else {
        info.balance_wei = "0x0";
        info.balance_eth = "0";
    }
    
    // Get transaction count
    auto tx_count = get_transaction_count(rpc_url, address);
    if (tx_count) {
        info.tx_count = *tx_count;
    }
    
    // Check if contract
    info.is_contract = is_contract(rpc_url, address);
    
    // Check token activity (only for EOA, skip for contracts to save time)
    if (!info.is_contract) {
        info.has_token_activity = has_token_activity(rpc_url, address);
    }
    
    return info;
}

} // namespace RpcClient
