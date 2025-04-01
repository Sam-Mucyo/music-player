# Network Music Player

A client-server music player that allows streaming WAV audio files over the network. The server hosts the music files and multiple clients can connect to select and play songs.

## Features

- Client-server architecture for streaming music over the network
- Supports playing WAV audio files with various bit depths (8, 16, 32 bit)
- Multiple clients can connect to a single server
- Basic playback controls (play, pause, stop, seek)
- Modular design for maintainability and testing

## Requirements

- C++17 compatible compiler
- CMake 3.12 or higher
- macOS (for Core Audio and AudioToolbox frameworks)

## Building

Run the build script to compile the project:

```
./build.sh
```

This will create a `build` directory with the compiled binaries in `build/bin`.

## Running

### Server

Start the server with:

```
./build/bin/music_server [port] [music_directory]
```

Default values:
- `port`: 8080
- `music_directory`: "./music"

The server will scan the music directory for `.wav` files and make them available for streaming.

### Client

Start the client with:

```
./build/bin/music_client [server_host] [port]
```

Default values:
- `server_host`: "localhost"
- `port`: 8080

Once connected, the client will retrieve the list of available songs from the server. You can use the following commands:

- `list`: Show available songs
- `play <song_number>`: Request and play a song by number
- `resume`: Resume playback
- `pause`: Pause playback
- `stop`: Stop playback
- `seek <seconds>`: Seek to position
- `position`: Show current position
- `duration`: Show song duration
- `help`: Show help
- `exit`: Exit the client

## Implementation Details

### Server Components

- `MusicServer`: Main server class that manages client connections
- `ClientHandler`: Handles individual client connections
- `MusicLibrary`: Manages the library of WAV files
- `WavFile`: Represents a WAV audio file

### Client Components

- `MusicClient`: Main client class that communicates with the server
- `AudioPlayer`: Handles audio playback using Core Audio

### Common Components

- `Socket`: Network socket wrapper for TCP communication
- `Protocol`: Message formats for client-server communication
- `WavHeader`: WAV file format header structure

## Documentation

This project uses Doxygen with the Doxygen Awesome CSS theme to generate API documentation.

### Setting Up Documentation

1. Install Doxygen and Graphviz:
   ```bash
   # macOS
   brew install doxygen graphviz
   
   # Ubuntu/Debian
   sudo apt-get install doxygen graphviz
   ```

2. Generate the documentation:
   ```bash
   doxygen Doxyfile
   ```

3. Open the generated documentation:
   ```bash
   open docs/html/index.html
   ```

### Documentation Workflow for Team Members

1. **Document your code** using Doxygen-style comments. Example:
   ```cpp
   /**
    * @brief Brief description of the function
    * @param paramName Description of the parameter
    * @return Description of the return value
    */
   ```

2. **Generate documentation locally** to preview changes before committing.

3. **Do not commit generated files** - the `docs/html/` and `docs/latex/` directories are excluded in `.gitignore`.

4. **CI/CD will automatically generate and publish** documentation to GitHub Pages when changes are pushed to the main branch.

## Project Structure

```
music_player/
├── client/
│   └── src/
│       ├── main.cpp
│       ├── music_client.cpp
│       ├── music_client.h
│       ├── audio_player.cpp
│       └── audio_player.h
├── common/
│   └── include/
│       ├── protocol.h
│       ├── socket.h
│       └── wav_header.h
├── docs/
│   ├── doxygen-awesome-css/  # Doxygen theme files
│   └── header.html           # Custom Doxygen header
├── server/
│   └── src/
│       ├── main.cpp
│       ├── music_server.cpp
│       ├── music_server.h
│       ├── client_handler.cpp
│       ├── client_handler.h
│       ├── music_library.cpp
│       ├── music_library.h
│       ├── wav_file.cpp
│       └── wav_file.h
├── music/
│   └── (WAV files go here)
├── .github/
│   └── workflows/            # CI/CD configuration
├── .gitignore               # Excludes generated files
├── CMakeLists.txt
├── Doxyfile                 # Doxygen configuration
└── build.sh
```