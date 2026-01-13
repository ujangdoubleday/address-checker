#include <iostream>
#include <iomanip>
#include <cstring>

#include "address/address.hpp"
#include "chain/chain.hpp"

void print_usage(const char *prog) {
    std::cout << "Usage: " << prog << " <address> [options]\n\n"
              << "Options:\n"
              << "  -c, --checksum    Verify EIP-55 checksum\n"
              << "  -f, --fix         Output checksummed address\n"
              << "  -l, --list-chains List supported chains\n"
              << "  -u, --update-rpcs Update RPCs from chainlist.org\n"
              << "  -h, --help        Show this help\n";
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
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--checksum") == 0) {
            verify_checksum = true;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fix") == 0) {
            fix_checksum = true;
        }
    }
    
    if (!Address::is_valid(address)) {
        std::cerr << "❌ Invalid address format\n";
        return 1;
    }
    
    std::cout << "✅ Valid EVM address\n";
    
    if (Address::is_zero(address)) {
        std::cout << "⚠️  Zero address (burn)\n";
    }
    
    if (verify_checksum) {
        if (Address::verify_checksum(address)) {
            std::cout << "✅ Valid checksum\n";
        } else {
            std::cout << "❌ Invalid checksum\n";
            return 1;
        }
    }
    
    if (fix_checksum) {
        std::string checksummed = Address::to_checksum(address);
        if (!checksummed.empty()) {
            std::cout << "📋 " << checksummed << "\n";
        }
    }
    
    return 0;
}
