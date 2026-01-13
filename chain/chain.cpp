#include "chain.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

static const std::vector<Chain> CHAINS = {
    /* Mainnets */
    {1, "Ethereum", "ETH", "https://etherscan.io", false},
    {137, "Polygon", "MATIC", "https://polygonscan.com", false},
    {56, "BNB Smart Chain", "BNB", "https://bscscan.com", false},
    {42161, "Arbitrum One", "ETH", "https://arbiscan.io", false},
    {10, "Optimism", "ETH", "https://optimistic.etherscan.io", false},
    {43114, "Avalanche", "AVAX", "https://snowtrace.io", false},
    {250, "Fantom", "FTM", "https://ftmscan.com", false},
    {8453, "Base", "ETH", "https://basescan.org", false},
    {324, "zkSync Era", "ETH", "https://explorer.zksync.io", false},
    {59144, "Linea", "ETH", "https://lineascan.build", false},
    {534352, "Scroll", "ETH", "https://scrollscan.com", false},
    {5000, "Mantle", "MNT", "https://explorer.mantle.xyz", false},
    {100, "Gnosis", "xDAI", "https://gnosisscan.io", false},
    {42220, "Celo", "CELO", "https://celoscan.io", false},
    {25, "Cronos", "CRO", "https://cronoscan.com", false},
    {1284, "Moonbeam", "GLMR", "https://moonbeam.moonscan.io", false},
    {1285, "Moonriver", "MOVR", "https://moonriver.moonscan.io", false},
    {288, "Boba Network", "ETH", "https://bobascan.com", false},
    {1313161554, "Aurora", "ETH", "https://aurorascan.dev", false},
    {8217, "Klaytn", "KLAY", "https://scope.klaytn.com", false},
    
    /* Testnets */
    {11155111, "Sepolia", "ETH", "https://sepolia.etherscan.io", true},
    {80001, "Mumbai", "MATIC", "https://mumbai.polygonscan.com", true},
    {97, "BSC Testnet", "tBNB", "https://testnet.bscscan.com", true},
    {421614, "Arbitrum Sepolia", "ETH", "https://sepolia.arbiscan.io", true},
    {11155420, "Optimism Sepolia", "ETH", "https://sepolia-optimism.etherscan.io", true},
    {43113, "Avalanche Fuji", "AVAX", "https://testnet.snowtrace.io", true},
    {84532, "Base Sepolia", "ETH", "https://sepolia.basescan.org", true},
};

namespace ChainRegistry {

std::optional<Chain> get_by_id(uint64_t chain_id) {
    for (const auto& chain : CHAINS) {
        if (chain.chain_id == chain_id) {
            return chain;
        }
    }
    return std::nullopt;
}

std::optional<Chain> get_by_name(const std::string& name) {
    auto to_lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };
    
    std::string lower_name = to_lower(name);
    
    for (const auto& chain : CHAINS) {
        if (to_lower(chain.name) == lower_name) {
            return chain;
        }
    }
    return std::nullopt;
}

const std::vector<Chain>& get_all() {
    return CHAINS;
}

size_t count() {
    return CHAINS.size();
}

void init() {
    std::cout << "🔄 Fetching RPCs from chainlist.org...\n";
    
    // Create data directory
    try {
        if (!fs::exists("data")) {
            fs::create_directory("data");
        }
        
        // Fetch JSON
        // Using system call for simplicity as requested
        int ret = std::system("curl -s https://chainlist.org/rpcs.json -o data/rpcs.json");
        
        if (ret != 0) {
            std::cerr << "⚠️  Failed to fetch RPCs. Check your internet connection or curl installation.\n";
        } else {
            std::cout << "✅ RPCs saved to data/rpcs.json\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error initializing chain registry: " << e.what() << "\n";
    }
}

} // namespace ChainRegistry
