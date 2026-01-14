#include "multi_checker.hpp"
#include "../chain/chain.hpp"
#include "../rpc/rpc.hpp"
#include <iostream>
#include <iomanip>

namespace MultiChainChecker {

std::vector<ChainResult> scan_all(const std::string& address,
                                   bool include_testnets,
                                   bool only_with_activity) {
    std::vector<ChainResult> results;
    const auto& chains = ChainRegistry::get_all();
    
    size_t total = chains.size();
    size_t current = 0;
    
    for (const auto& chain : chains) {
        current++;
        
        // Skip testnets if not requested
        if (!include_testnets && chain.is_testnet) {
            continue;
        }
        
        // Skip chains without RPC endpoints
        if (chain.rpc_urls.empty()) {
            continue;
        }
        
        // Try each RPC endpoint until we get valid data
        AddressInfo info;
        bool got_valid_response = false;
        std::string used_rpc;
        
        for (const auto& rpc_url : chain.rpc_urls) {
            // Skip non-HTTP endpoints
            if (rpc_url.find("http") != 0) continue;
            // Skip websocket endpoints
            if (rpc_url.find("wss://") == 0) continue;
            // Skip endpoints with variables
            if (rpc_url.find("${") != std::string::npos) continue;
            if (rpc_url.find("{") != std::string::npos) continue;
            
            // Display progress with RPC URL
            std::cout << "[" << current << "/" << total << "] " << chain.name 
                      << " -> " << rpc_url << "\n" << std::flush;
            
            // Query the RPC
            info = RpcClient::check_address(rpc_url, address);
            
            // Check if we got a valid response (non-empty balance_wei means RPC responded)
            if (!info.balance_wei.empty()) {
                got_valid_response = true;
                used_rpc = rpc_url;
                break;  // Skip to next chain, we have valid data
            }
            
            // RPC failed, try next one
            std::cout << "  (no response, trying next RPC...)\n";
        }
        
        if (!got_valid_response) {
            continue;
        }
        
        // Check if there's any activity
        bool has_activity = (info.balance_eth != "0" && !info.balance_eth.empty()) 
                           || info.tx_count > 0;
        
        // Skip if filtering and no activity
        if (only_with_activity && !has_activity) {
            continue;
        }
        
        ChainResult result;
        result.chain_id = chain.chain_id;
        result.chain_name = chain.name;
        result.symbol = chain.symbol;
        result.balance_eth = info.balance_eth.empty() ? "0" : info.balance_eth;
        result.tx_count = info.tx_count;
        result.is_contract = info.is_contract;
        result.has_activity = has_activity;
        
        results.push_back(result);
    }
    
    std::cout << "\nScan complete.\n";
    
    return results;
}

void print_results(const std::vector<ChainResult>& results) {
    if (results.empty()) {
        std::cout << "No activity found on any chain.\n";
        return;
    }
    
    std::cout << "Found activity on " << results.size() << " chain(s):\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::left 
              << std::setw(8) << "ChainID"
              << std::setw(25) << "Network"
              << std::setw(8) << "Symbol"
              << std::setw(25) << "Balance"
              << std::setw(10) << "TX Count"
              << "\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (const auto& r : results) {
        std::cout << std::left
                  << std::setw(8) << r.chain_id
                  << std::setw(25) << (r.chain_name.length() > 24 ? r.chain_name.substr(0, 21) + "..." : r.chain_name)
                  << std::setw(8) << r.symbol
                  << std::setw(25) << (r.balance_eth + " " + r.symbol).substr(0, 24)
                  << std::setw(10) << r.tx_count
                  << "\n";
    }
    
    std::cout << std::string(80, '-') << "\n";
}

} // namespace MultiChainChecker
