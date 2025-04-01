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
├── CMakeLists.txt
└── build.sh
```