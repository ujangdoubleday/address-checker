#include "chain.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

#include <fstream>
#include "include/json.hpp" // Changed include path as per plan

// Using nlohmann::json
using json = nlohmann::json;

static std::vector<Chain> current_chains;

namespace ChainRegistry {

std::optional<Chain> get_by_id(uint64_t chain_id) {
    for (const auto& chain : current_chains) {
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
    
    for (const auto& chain : current_chains) {
        if (to_lower(chain.name) == lower_name) {
            return chain;
        }
    }
    return std::nullopt;
}

const std::vector<Chain>& get_all() {
    return current_chains;
}

size_t count() {
    return current_chains.size();
}

void init(bool force_update) {
    fs::path data_dir = "data";
    fs::path rpcs_file = data_dir / "rpcs.json";
    
    // Create data directory
    try {
        if (!fs::exists(data_dir)) {
            fs::create_directory(data_dir);
        }
        
        bool file_exists = fs::exists(rpcs_file);
        
        if (!file_exists || force_update) {
            std::cout << "🔄 Fetching RPCs from chainlist.org...\n";
            std::string cmd = "curl -s https://chainlist.org/rpcs.json -o " + rpcs_file.string();
            int ret = std::system(cmd.c_str());
            
            if (ret != 0) {
                std::cerr << "⚠️  Failed to fetch RPCs. Check your internet connection or curl installation.\n";
            } else {
                std::cout << "✅ RPCs saved to " << rpcs_file << "\n";
            }
        }
        
        // Load chains from JSON
        if (fs::exists(rpcs_file)) {
            std::ifstream f(rpcs_file);
            json data = json::parse(f);
            
            current_chains.clear();
            
            for (const auto& item : data) {
                Chain chain;
                chain.chain_id = item.value("chainId", 0ULL);
                chain.name = item.value("name", "Unknown");
                chain.is_testnet = item.value("isTestnet", false);
                
                // Native currency symbol
                if (item.contains("nativeCurrency") && item["nativeCurrency"].contains("symbol")) {
                    chain.symbol = item["nativeCurrency"]["symbol"];
                } else {
                    chain.symbol = "ETH"; // Default fallback
                }
                
                // RPC URLs
                if (item.contains("rpc")) {
                    for (const auto& rpc : item["rpc"]) {
                        if (rpc.contains("url")) {
                            chain.rpc_urls.push_back(rpc["url"]);
                        }
                    }
                }
                
                // Explorer URL
                if (item.contains("explorers") && !item["explorers"].empty()) {
                    chain.explorer_url = item["explorers"][0].value("url", "");
                } else if (item.contains("infoURL")) {
                    chain.explorer_url = item.value("infoURL", "");
                }
                
                if (chain.chain_id != 0) {
                    current_chains.push_back(chain);
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error initializing chain registry: " << e.what() << "\n";
    }
}

} // namespace ChainRegistry
