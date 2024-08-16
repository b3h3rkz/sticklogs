# StickyLogs

StickyLogs is a high-performance transaction logging system built with C++ and RocksDB. It provides efficient storage and retrieval of financial transactions with support for bulk operations and date range queries.

## Architecture

StickyLogs is structured around the following key components:

1. **Transaction**: A class representing individual financial transactions. Each transaction includes:
   - ID
   - Reference
   - Currency
   - Amount (in smallest unit)
   - Timestamp

2. **DBWrapper**: A wrapper class for RocksDB operations. It provides methods for:
   - Inserting single transactions
   - Bulk inserting transactions
   - Retrieving transactions by ID
   - Querying transactions by date range

3. **Main Application**: Demonstrates the basic usage of the system.

4. **Benchmark Suite**: A separate executable for performance testing and optimization.

The system uses RocksDB as its underlying storage engine, providing high-performance, persistent key-value storage.

## Setup Instructions

### Prerequisites

- CMake (version 3.10 or higher)
- C++ compiler with C++17 support
- RocksDB library

### Installation

1. Clone the repository:

  ```
    git clone https://github.com/b3h3rkz/stickylogs.git
    cd stickylogs
  ```
2. Install RocksDB:
- On macOS with Homebrew:
  ```
  brew install rocksdb
  ```
- On Ubuntu:
  ```
  sudo apt-get install librocksdb-dev
  ```

3. Build the project:
  ```
  mkdir build && cd build
  cmake .. && make
  ```
This will create two executables: `stickylogs` (main application) and `stickylogs_benchmark` (benchmark suite).

### Running the Application

To run the main application:
  ```
  ./stickylogs
  ```
To run the benchmark suite:
  ```
  ./stickylogs_benchmark
  ```
## Usage

The main application demonstrates basic usage of the StickyLogs system, including inserting transactions and querying them.

For more advanced usage, refer to the `DBWrapper` class in `db_wrapper.h`. Key methods include:

- `insert_transaction(const Transaction& transaction)`
- `bulk_insert_transactions(const std::vector<Transaction>& transactions)`
- `get_transaction(const std::string& id)`
- `get_transactions_by_date_range(int64_t start_timestamp, int64_t end_timestamp)`

## Benchmarking

The benchmark suite (`stickylogs_benchmark`) provides performance metrics for key operations. Use this to test the system's performance under various conditions and to guide optimization efforts.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

