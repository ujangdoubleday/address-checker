#include "chain.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

#include <fstream>
#include "model/chain_model.hpp"

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
        
        // Load chains from JSON using Model
        if (fs::exists(rpcs_file)) {
            std::ifstream f(rpcs_file);
            json data = json::parse(f);
            
            // Deserialize into model
            auto configs = data.get<std::vector<Model::ChainConfig>>();
            
            current_chains.clear();
            current_chains.reserve(configs.size());
            
            for (const auto& config : configs) {
                if (config.chainId == 0) continue;

                Chain chain;
                chain.chain_id = config.chainId;
                chain.name = config.name;
                chain.symbol = config.nativeCurrency.symbol.empty() ? "ETH" : config.nativeCurrency.symbol;
                chain.is_testnet = config.isTestnet;
                
                // Map RPCs
                for (const auto& rpc : config.rpc) {
                    if (!rpc.url.empty()) {
                        chain.rpc_urls.push_back(rpc.url);
                    }
                }
                
                // Map Explorer (prefer first one, fallback to infoURL)
                if (!config.explorers.empty()) {
                    chain.explorer_url = config.explorers[0].url;
                } else {
                    chain.explorer_url = config.infoURL;
                }
                
                current_chains.push_back(chain);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error initializing chain registry: " << e.what() << "\n";
    }
}

} // namespace ChainRegistry
