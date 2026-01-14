#include <iostream>
#include <iomanip>
#include <cstring>

#include "address/address.hpp"
#include "chain/chain.hpp"
#include "rpc/rpc.hpp"
#include "multi_checker/multi_checker.hpp"

void print_usage(const char *prog) {
    std::cout << "Usage: " << prog << " <address> [options]\n\n"
              << "Options:\n"
              << "  -c, --checksum       Verify EIP-55 checksum\n"
              << "  -f, --fix            Output checksummed address\n"
              << "  -i, --info <chain>   Show address info (balance, tx, tokens)\n"
              << "  -a, --scan-all       Scan address across all chains (including testnets)\n"
              << "  -l, --list-chains    List supported chains\n"
              << "  -u, --update-rpcs    Update RPCs from chainlist.org\n"
              << "  -h, --help           Show this help\n";
}

void list_chains() {
    const auto& chains = ChainRegistry::get_all();
    
    std::cout << "Supported EVM Chains (" << chains.size() << ")\n";
    std::cout << std::string(50, '-') << "\n";
    
    for (const auto& chain : chains) {
        std::cout << std::left
                  << std::setw(12) << chain.chain_id
                  << std::setw(18) << chain.name
                  << std::setw(6) << chain.symbol
                  << (chain.is_testnet ? "Testnet" : "Mainnet") << "\n";
    }
}

int main(int argc, char *argv[]) {
    // Check for update flag early
    bool force_update = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--update-rpcs") == 0) {
            force_update = true;
            break;
        }
    }

    // Initialize chains (fetch RPCs)
    ChainRegistry::init(force_update);

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *arg1 = argv[1];
    
    if (strcmp(arg1, "-h") == 0 || strcmp(arg1, "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(arg1, "-l") == 0 || strcmp(arg1, "--list-chains") == 0) {
        list_chains();
        return 0;
    }

    if (strcmp(arg1, "-u") == 0 || strcmp(arg1, "--update-rpcs") == 0) {
        // RPCs already updated by init() at start of main
        return 0;
    }
    
    std::string_view address = arg1;
    bool verify_checksum = false;
    bool fix_checksum = false;
    uint64_t info_chain_id = 0;
    bool scan_all = false;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--checksum") == 0) {
            verify_checksum = true;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fix") == 0) {
            fix_checksum = true;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--info") == 0) {
            if (i + 1 < argc) {
                try {
                    info_chain_id = std::stoull(argv[++i]);
                } catch (...) {
                    std::cerr << "Error: Invalid chain ID\n";
                    return 1;
                }
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--scan-all") == 0) {
            scan_all = true;
        }
    }
    
    if (!Address::is_valid(address)) {
        std::cerr << "Error: Invalid address format\n";
        return 1;
    }
    
    std::cout << "Valid EVM address\n";
    
    if (Address::is_zero(address)) {
        std::cout << "Warning: Zero address (burn)\n";
    }
    
    if (verify_checksum) {
        if (Address::verify_checksum(address)) {
            std::cout << "Valid checksum\n";
        } else {
            std::cout << "Error: Invalid checksum\n";
            return 1;
        }
    }
    
    if (fix_checksum) {
        std::string checksummed = Address::to_checksum(address);
        if (!checksummed.empty()) {
            std::cout << "Checksum: " << checksummed << "\n";
        }
    }
    
    // Multi-chain scan (includes testnets by default)
    if (scan_all) {
        std::cout << "\nScanning address across all chains (including testnets)...\n";
        auto results = MultiChainChecker::scan_all(std::string(address), true);  // Always include testnets
        MultiChainChecker::print_results(results);
        return 0;
    }
    
    // Check address info via RPC
    if (info_chain_id > 0) {
        auto chain = ChainRegistry::get_by_id(info_chain_id);
        if (!chain) {
            std::cerr << "Error: Chain ID " << info_chain_id << " not found\n";
            return 1;
        }
        
        if (chain->rpc_urls.empty()) {
            std::cerr << "Error: No RPC endpoints available for " << chain->name << "\n";
            return 1;
        }
        
        std::cout << "\nChecking on " << chain->name << " (" << chain->symbol << ")...\n";
        
        // Try RPC endpoints until one works
        AddressInfo info;
        bool success = false;
        
        for (const auto& rpc_url : chain->rpc_urls) {
            // Skip non-HTTP endpoints
            if (rpc_url.find("http") != 0) continue;
            // Skip wss endpoints
            if (rpc_url.find("wss://") == 0) continue;
            
            info = RpcClient::check_address(rpc_url, std::string(address));
            if (!info.balance_wei.empty() && info.balance_wei != "0x0" && info.balance_eth != "0") {
                success = true;
                break;
            }
            // If balance is 0 but we got a response, also consider it success
            if (!info.balance_wei.empty()) {
                success = true;
                break;
            }
        }
        
        if (!success) {
            std::cerr << "Warning: Could not fetch data from RPC endpoints\n";
            return 1;
        }
        
        std::cout << "Balance: " << info.balance_eth << " " << chain->symbol << "\n";
        std::cout << "TX Count: " << info.tx_count << "\n";
    }
    
    return 0;
}
