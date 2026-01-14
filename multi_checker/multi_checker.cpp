#include "multi_checker.hpp"
#include "../chain/chain.hpp"
#include "../rpc/rpc.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

namespace MultiChainChecker {

// Thread-safe output
static std::mutex cout_mutex;
static std::mutex results_mutex;

// Number of concurrent workers
constexpr size_t NUM_WORKERS = 100;

// Worker function to process a single chain
static void process_chain(const Chain& chain, 
                          const std::string& address,
                          size_t current,
                          size_t total,
                          std::vector<ChainResult>& results,
                          bool only_with_activity) {
    
    // Try each RPC endpoint until we get valid data
    AddressInfo info;
    bool got_valid_response = false;
    
    for (const auto& rpc_url : chain.rpc_urls) {
        // Skip non-HTTP endpoints
        if (rpc_url.find("http") != 0) continue;
        // Skip websocket endpoints
        if (rpc_url.find("wss://") == 0) continue;
        // Skip endpoints with variables
        if (rpc_url.find("${") != std::string::npos) continue;
        if (rpc_url.find("{") != std::string::npos) continue;
        
        // Display progress with RPC URL (thread-safe)
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "[" << current << "/" << total << "] " << chain.name 
                      << " -> " << rpc_url << "\n" << std::flush;
        }
        
        // Query the RPC
        info = RpcClient::check_address(rpc_url, address);
        
        // Check if we got a valid response
        if (!info.balance_wei.empty()) {
            got_valid_response = true;
            break;  // Skip to next chain
        }
        
        // RPC failed, try next one
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "  [" << chain.name << "] no response, trying next RPC...\n";
        }
    }
    
    if (!got_valid_response) {
        return;
    }
    
    // Check if there's any activity
    bool has_activity = (info.balance_eth != "0" && !info.balance_eth.empty()) 
                       || info.tx_count > 0;
    
    // Skip if filtering and no activity
    if (only_with_activity && !has_activity) {
        return;
    }
    
    ChainResult result;
    result.chain_id = chain.chain_id;
    result.chain_name = chain.name;
    result.symbol = chain.symbol;
    result.balance_eth = info.balance_eth.empty() ? "0" : info.balance_eth;
    result.tx_count = info.tx_count;
    result.is_contract = info.is_contract;
    result.has_activity = has_activity;
    
    // Thread-safe add to results
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back(result);
    }
}

std::vector<ChainResult> scan_all(const std::string& address,
                                   bool include_testnets,
                                   bool only_with_activity) {
    std::vector<ChainResult> results;
    const auto& chains = ChainRegistry::get_all();
    
    // Filter chains first
    std::vector<std::pair<size_t, const Chain*>> valid_chains;
    for (size_t i = 0; i < chains.size(); i++) {
        const auto& chain = chains[i];
        
        // Skip testnets if not requested
        if (!include_testnets && chain.is_testnet) {
            continue;
        }
        
        // Skip chains without RPC endpoints
        if (chain.rpc_urls.empty()) {
            continue;
        }
        
        // Check if chain has valid HTTP endpoints
        bool has_http = false;
        for (const auto& rpc_url : chain.rpc_urls) {
            if (rpc_url.find("http") == 0 && 
                rpc_url.find("wss://") != 0 &&
                rpc_url.find("${") == std::string::npos &&
                rpc_url.find("{") == std::string::npos) {
                has_http = true;
                break;
            }
        }
        
        if (has_http) {
            valid_chains.push_back({i + 1, &chain});
        }
    }
    
    size_t total = chains.size();
    std::cout << "Scanning " << valid_chains.size() << " chains with " 
              << NUM_WORKERS << " parallel workers...\n\n";
    
    // Process chains in batches using thread pool
    std::vector<std::thread> workers;
    std::atomic<size_t> chain_index(0);
    
    auto worker_func = [&]() {
        while (true) {
            size_t idx = chain_index.fetch_add(1);
            if (idx >= valid_chains.size()) {
                break;
            }
            
            auto& [original_idx, chain_ptr] = valid_chains[idx];
            process_chain(*chain_ptr, address, original_idx, total, results, only_with_activity);
        }
    };
    
    // Start workers
    for (size_t i = 0; i < NUM_WORKERS; i++) {
        workers.emplace_back(worker_func);
    }
    
    // Wait for all workers to finish
    for (auto& worker : workers) {
        worker.join();
    }
    
    std::cout << "\nScan complete.\n";
    
    // Sort results by chain_id
    std::sort(results.begin(), results.end(), 
              [](const ChainResult& a, const ChainResult& b) {
                  return a.chain_id < b.chain_id;
              });
    
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
