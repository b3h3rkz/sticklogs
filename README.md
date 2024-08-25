# StickyLogs

StickyLogs is a high-performance, concurrent logging server designed to handle massive volumes of log entries with exceptional efficiency. It provides a robust API for inserting and querying logs, making it ideal for applications that demand fast, reliable, and scalable logging capabilities.

## Features

- Ultra-fast log insertion with support for high concurrency
- Efficient log retrieval, including bulk queries
- Advanced concurrency support, handling thousands of simultaneous connections
- RESTful API for seamless integration
- Optimized for high throughput and low latency
- Built on RocksDB for exceptional performance and durability

## Performance Highlights

- Capable of handling 1 million+ log insertions per minute
- Supports 2000+ concurrent connections
- Sub-millisecond latency for individual log insertions
- Efficient bulk retrieval of millions of logs

## Prerequisites

- Ubuntu Server (20.04 LTS or later recommended)
- CMake (version 3.10 or later)
- C++17 compatible compiler (GCC 9+ or Clang 9+)
- RocksDB (version 6.10.2 or later)
- Boost libraries (version 1.71 or later)
- nlohmann/json library (version 3.9.0 or later)

## Installation

1. Update your system:
   ```
   sudo apt update && sudo apt upgrade -y
   ```

2. Install dependencies:
   ```
   sudo apt install -y cmake g++ libboost-all-dev librocksdb-dev nlohmann-json3-dev
   ```

3. Clone the repository:
   ```
   git clone https://github.com/b3h3rkz/stickylogs.git
   cd stickylogs
   ```

4. Build the project:
   ```
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make
   ```

   For faster builds on multi-core systems, you can use:
   ```
   make -j$(nproc)
   ```
   Note: This may increase memory usage during compilation.

## Configuration

Edit the `config/db_config.yaml` file to optimize your database settings:

```yaml
db_path: "/path/to/your/database"
max_open_files: 10000
write_buffer_size: 64MB
max_write_buffer_number: 3
target_file_size_base: 64MB
max_background_jobs: 4
max_background_compactions: 4
max_background_flushes: 2
allow_concurrent_memtable_write: true
enable_pipelined_write: true
```

Adjust these values based on your system's capabilities and requirements. For high-performance scenarios, consider increasing `max_open_files`, `write_buffer_size`, and `max_background_jobs`.

## Usage

1. Start the StickyLogs server:
   ```
   ./stickylogs /path/to/your/database
   ```

2. The server will start listening on port 54321 by default.

3. Use the provided API to insert and query logs:

   - Insert a single log:
     ```
     POST http://your-server-ip:54321
     {
       "action": "insert",
       "reference": "unique_log_reference",
       "metadata": {
         "event": "user_login",
         "details": "User logged in successfully",
         "user_id": "12345",
         "ip_address": "192.168.1.1"
       }
     }
     ```

   - Batch insert multiple logs:
     ```
     POST http://your-server-ip:54321
     {
       "action": "batch_insert",
       "logs": [
         {
           "reference": "log_ref_1",
           "metadata": {
             "event": "page_view",
             "page": "/home",
             "user_id": "12345"
           }
         },
         {
           "reference": "log_ref_2",
           "metadata": {
             "event": "button_click",
             "button_id": "submit_form",
             "user_id": "12345"
           }
         }
       ]
     }
     ```

   - Query all logs:
     ```
     POST http://your-server-ip:54321
     {
       "action": "query_all"
     }
     ```

   - Query by reference:
     ```
     POST http://your-server-ip:54321
     {
       "action": "query_by_reference",
       "reference": "unique_log_reference"
     }
     ```

   - Query with filters:
     ```
     POST http://your-server-ip:54321
     {
       "action": "query",
       "filters": {
         "event": "user_login",
         "user_id": "12345"
       }
     }
     ```

## Performance Optimization

StickyLogs is designed for high performance and concurrency. Here are some tips to get the most out of your setup:

1. **Increase open file limit**: Set a high limit for open files in your system to allow RocksDB to maintain many open files:
   ```
   sudo ulimit -n 65535
   ```

2. **Optimize for your storage**: If using SSDs, you can increase `write_buffer_size` and `target_file_size_base` for better performance.

3. **Tune RocksDB**: Adjust RocksDB settings in `db_config.yaml` based on your workload. For write-heavy workloads, increase `max_background_flushes`.

4. **Network tuning**: Optimize your network stack for high concurrency:
   ```
   sudo sysctl -w net.core.somaxconn=65535
   sudo sysctl -w net.ipv4.tcp_max_syn_backlog=65535
   ```

5. **Use batch inserts**: For high-volume logging, use the batch insert API to reduce network overhead.

## Performance Testing

A Python script (`test_server.py`) is provided for comprehensive performance testing. It can simulate high concurrency, batch inserts, and measure query performance.

To run the performance test:

1. Install Python dependencies:
   ```
   pip install aiohttp psutil py-cpuinfo
   ```

2. Run the test script:
   ```
   python test_server.py
   ```

This script will perform the following tests:
- Insert 1 million logs using 2000 concurrent connections
- Perform batch inserts of varying sizes
- Query all logs using 10 concurrent threads
- Measure and report on latency, throughput, and system resource usage

## Monitoring

For production deployments, consider setting up comprehensive monitoring:

- Use `Prometheus` and `Grafana` for real-time monitoring and alerting
- Monitor key metrics:
  - Requests per second
  - Latency percentiles (p50, p95, p99)
  - CPU and memory usage
  - Disk I/O and network throughput
- Set up log rotation for StickyLogs server logs

## Troubleshooting

- If experiencing high latency, check system resources and consider scaling horizontally
- For insertion failures, verify that references are unique
- If queries are slow, check your RocksDB configuration and consider adding indexes
- Use `strace` to diagnose system call issues: `strace -p <pid_of_stickylogs>`

## Contributing

We welcome contributions! Please read our CONTRIBUTING.md file for guidelines on how to make StickyLogs even better.

## License

This project is licensed under the MIT License 