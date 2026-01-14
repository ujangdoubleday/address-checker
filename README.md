# Address Checker

A fast multi-chain EVM address validation and balance checker tool. Scans addresses across 2500+ EVM-compatible blockchain networks with parallel workers.

## Features

- Validate EVM addresses (format and checksum)
- Check balance and transaction count on any EVM chain
- Scan address across ALL chains in parallel (100 concurrent workers)
- Batch RPC calls for faster scanning
- Includes both mainnet and testnet networks
- Auto-fetch RPC endpoints from chainlist.org

## Requirements

- g++ with C++17 support
- curl
- make

## Installation

Clone the repository:

```bash
git clone git@github.com:ujangdoubleday/address-checker.git
cd address-checker
```

Build:

```bash
make
```

For clean rebuild:

```bash
make clean && make
```

## Usage

```bash
./checker <address> [options]
```

### Options

| Option                  | Description                                         |
| ----------------------- | --------------------------------------------------- |
| `-c, --checksum`        | Verify EIP-55 checksum                              |
| `-f, --fix`             | Output checksummed address                          |
| `-i, --info <chain_id>` | Show balance and tx count on specific chain         |
| `-a, --scan-all`        | Scan address across all chains (including testnets) |
| `-l, --list-chains`     | List all supported chains                           |
| `-u, --update-rpcs`     | Update RPC endpoints from chainlist.org             |
| `-h, --help`            | Show help                                           |

### Examples

Validate an address:

```bash
./checker 0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045
```

Check balance on Ethereum Mainnet (chain ID 1):

```bash
./checker 0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045 --info 1
```

Scan all chains for balance/activity:

```bash
./checker 0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045 --scan-all
```

Verify checksum:

```bash
./checker 0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045 --checksum
```

Fix/convert to checksummed address:

```bash
./checker 0xd8da6bf26964af9d7eed9e03e53415d37aa96045 --fix
```

List supported chains:

```bash
./checker --list-chains
```

Update chain data:

```bash
./checker --update-rpcs
```

## Chain Data

Chain and RPC data is fetched from [chainlist.org](https://chainlist.org). On first run, the tool automatically downloads `rpcs.json` containing 2500+ EVM networks with their RPC endpoints.

To manually update the chain data:

```bash
./checker --update-rpcs
```

Or download directly:

```bash
curl -o data/rpcs.json https://chainlist.org/rpcs.json
```

## How It Works

1. Loads all chain configurations from `data/rpcs.json`
2. For each chain, finds HTTP RPC endpoints
3. Uses 100 parallel workers to query chains simultaneously
4. Sends batch RPC (`eth_getBalance` + `eth_getTransactionCount`) in one HTTP request
5. Shows only chains with activity (balance > 0 or tx count > 0)

## License

[MIT](LICENSE)
