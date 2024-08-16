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

``` git clone https://github.com/yourusername/stickylogs.git
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

