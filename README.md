# Multithreaded Segmented File Downloader

A concurrent file downloader implemented in C that splits files into segments and downloads them using multiple threads. This project demonstrates the use of POSIX threads, mutex locks, and libcurl for efficient file downloading.

## Features

- **Concurrent Downloads**: Downloads file segments in parallel using multiple threads
- **Segment Management**: Splits large files into smaller segments for efficient downloading
- **Progress Tracking**: Shows download progress for each segment
- **Error Handling**: Implements robust error checking and reporting
- **Automatic Directory Creation**: Creates a 'downloads' directory to store downloaded files

## Prerequisites

- GCC compiler
- POSIX Threads library
- libcurl development package
- Make build system

### System Requirements

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential libcurl4-openssl-dev
```

#### Fedora/RHEL
```bash
sudo dnf install gcc make libcurl-devel
```

## Building the Project

```bash
# Clone the repository
git clone https://github.com/OS-OGs/Multithreaded-downloader
cd file-download-manager

# Build the project
make clean
make
```

## Usage

```bash
# Basic usage
./file_download_manager download "URL"

# Example
./file_download_manager download "https://example.com/file.txt"
```

### Output Location
Downloaded files are automatically saved in the `downloads` directory within the project folder.

## Project Structure

```
.
├── include/          # Header files
├── src/             # Source files
├── downloads/       # Downloaded files directory
├── Makefile        # Build configuration
└── README.md       # Project documentation
```

## Implementation Details

- Uses POSIX threads for concurrent downloading
- Implements mutex locks for thread synchronization
- Uses libcurl for HTTP/HTTPS downloads
- Handles file segments with proper error checking
- Merges segments into the final file

## Synchronization Features
- Dining Philosophers: Thread resource management
- Reader-Writer: Segment file access control
- Producer-Consumer: Download buffer management

## Error Handling

The program includes comprehensive error handling for:
- Network connectivity issues
- Invalid URLs
- File system errors
- Memory allocation failures
- Thread creation/management issues

## Contributing

Feel free to submit issues and pull requests for:
- Bug fixes
- Feature improvements
- Documentation updates
- Code optimization

## License

This project is licensed under the MIT License - see the LICENSE file for details.
