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

// Thread-safe counters
static std::mutex cout_mutex;
static std::mutex results_mutex;
static std::atomic<size_t> completed_count(0);



// Worker function to process a single chain
static void process_chain(const Chain& chain, 
                          const std::string& address,
                          std::vector<ChainResult>& results,
                          bool only_with_activity) {
    
    // Try each RPC endpoint until we get valid data
    AddressInfo info;
    bool got_valid_response = false;
    
    for (const auto& rpc_url : chain.rpc_urls) {
        // Skip non-HTTP endpoints
        if (rpc_url.find("http") != 0) continue;
        if (rpc_url.find("wss://") == 0) continue;
        if (rpc_url.find("${") != std::string::npos) continue;
        if (rpc_url.find("{") != std::string::npos) continue;
        
        // Query the RPC (no display, just try it)
        info = RpcClient::check_address(rpc_url, address);
        
        // Check if we got a valid response
        if (!info.balance_wei.empty()) {
            got_valid_response = true;
            break;  // Success - move to next chain
        }
        // RPC failed - try next RPC
    }
    
    if (!got_valid_response) {
        return;  // All RPCs failed for this chain
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
    result.explorer_url = chain.explorer_url;
    
    // Thread-safe add to results
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back(result);
    }
}

std::vector<ChainResult> scan_all(const std::string& address,
                                   bool include_testnets,
                                   bool only_with_activity,
                                   size_t num_threads) {
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
    
    size_t total_valid = valid_chains.size();
    completed_count = 0;
    
    // determine optimal worker count: min of requested threads and available chains
    size_t actual_workers = std::min(num_threads, total_valid);
    if (actual_workers < 1) actual_workers = 1;
    
    std::cout << "Scanning " << total_valid << " chains with " << actual_workers << " thread(s)...\n" << std::flush;
    
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
            process_chain(*chain_ptr, address, results, only_with_activity);
            
            // Update progress
            size_t done = completed_count.fetch_add(1) + 1;
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "\rProgress: " << done << "/" << total_valid << " chains checked" << std::flush;
            }
        }
    };
    
    // start workers
    for (size_t i = 0; i < actual_workers; i++) {
        workers.emplace_back(worker_func);
    }
    
    // Wait for all workers to finish
    for (auto& worker : workers) {
        worker.join();
    }
    
    std::cout << "\rProgress: " << total_valid << "/" << total_valid << " chains checked\n";
    std::cout << "Scan complete.\n";
    
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
    std::cout << std::string(120, '-') << "\n";
    std::cout << std::left 
              << std::setw(8) << "ChainID"
              << std::setw(20) << "Network"
              << std::setw(8) << "Symbol"
              << std::setw(22) << "Balance"
              << std::setw(10) << "TX Count"
              << "Explorer"
              << "\n";
    std::cout << std::string(120, '-') << "\n";
    
    for (const auto& r : results) {
        std::cout << std::left
                  << std::setw(8) << r.chain_id
                  << std::setw(20) << (r.chain_name.length() > 19 ? r.chain_name.substr(0, 16) + "..." : r.chain_name)
                  << std::setw(8) << r.symbol
                  << std::setw(22) << (r.balance_eth + " " + r.symbol).substr(0, 21)
                  << std::setw(10) << r.tx_count
                  << (r.explorer_url.empty() ? "-" : r.explorer_url)
                  << "\n";
    }
    
    std::cout << std::string(120, '-') << "\n";
}

} // namespace MultiChainChecker
