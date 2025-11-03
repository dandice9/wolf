# WebSocket Support in Wolf

Wolf Web Framework includes built-in WebSocket support via Boost.Beast.

## ✅ WebSocket Fixed

The WebSocket implementation now correctly handles:
- WebSocket upgrade handshake (HTTP 101 Switching Protocols)
- Bidirectional message passing
- Echo server functionality (messages are echoed back to client)

## How It Works

1. **Automatic Detection**: Wolf automatically detects WebSocket upgrade requests
2. **Handshake**: Performs the HTTP → WebSocket protocol upgrade
3. **Echo Server**: Built-in echo functionality - any message sent is echoed back

## Testing WebSocket

### Method 1: Browser (Recommended)

Open `websocket_test.html` in your browser:

```bash
# Start the server
cd build/example
./example_web

# Open websocket_test.html in your browser
# File location: example/websocket_test.html
```

Then click "Connect" and send messages!

### Method 2: JavaScript Console

```javascript
const ws = new WebSocket("ws://localhost:8080");

ws.onopen = () => {
    console.log("Connected!");
    ws.send("Hello Wolf!");
};

ws.onmessage = (event) => {
    console.log("Received:", event.data);
};

ws.onerror = (error) => {
    console.error("Error:", error);
};

ws.onclose = () => {
    console.log("Disconnected");
};
```

### Method 3: Python Script

```bash
# Install websockets library
pip install websockets

# Run test
python3 example/test_websocket.py
```

### Method 4: curl (Just for handshake test)

```bash
curl -i -N \
  -H "Connection: Upgrade" \
  -H "Upgrade: websocket" \
  -H "Sec-WebSocket-Version: 13" \
  -H "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" \
  http://localhost:8080/
```

Expected response:
```
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
Server: WolfServer
```

## Implementation Details

### How WebSocket Upgrade Works

1. Client sends HTTP request with upgrade headers:
   ```
   GET / HTTP/1.1
   Host: localhost:8080
   Upgrade: websocket
   Connection: Upgrade
   Sec-WebSocket-Key: ...
   Sec-WebSocket-Version: 13
   ```

2. Wolf detects the upgrade request in `http_session`

3. Creates a `websocket_session` and passes the request

4. `websocket_session` accepts the upgrade and responds with HTTP 101

5. Connection switches to WebSocket protocol

6. Messages are echoed back to the client

### Code Structure

```cpp
// In web_server.hpp

class websocket_session {
    // Handles WebSocket connections
    // - Accepts upgrade from HTTP
    // - Reads WebSocket frames
    // - Echoes messages back
};

class http_session {
    void handle_request() {
        if (beast::websocket::is_upgrade(request_)) {
            // Create WebSocket session
            auto ws = std::make_shared<websocket_session>(std::move(socket_));
            ws->run(std::move(request_));
            return;
        }
        // ... normal HTTP handling
    }
};
```

## Customizing WebSocket Behavior

To customize WebSocket handling, modify `websocket_session::on_read()` in `web_server.hpp`:

```cpp
void on_read(beast::error_code ec, std::size_t bytes_transferred) {
    // ... error handling ...
    
    // Current: Echo back
    ws_.text(ws_.got_text());
    ws_.async_write(buffer_.data(), ...);
    
    // Custom: Process message
    std::string message = beast::buffers_to_string(buffer_.data());
    // ... your custom logic ...
    // Then send response
}
```

## Example Use Cases

1. **Chat Application**: Real-time messaging between clients
2. **Live Updates**: Push notifications to browsers
3. **Game Server**: Real-time multiplayer game state
4. **Dashboard**: Live data streaming
5. **Collaborative Editing**: Real-time document collaboration

## Browser Compatibility

WebSocket is supported in all modern browsers:
- Chrome 16+
- Firefox 11+
- Safari 7+
- Edge (all versions)
- Opera 12.1+

## Technical Notes

- **Binary & Text**: Wolf supports both text and binary WebSocket frames
- **Auto-detection**: Wolf automatically detects if message is text or binary
- **Thread-safe**: WebSocket sessions run in the async I/O thread pool
- **Keep-alive**: Connections stay open until explicitly closed

## Troubleshooting

### Connection Hangs

**Problem**: Browser console shows connection pending

**Solution**: The WebSocket upgrade fix is implemented. Make sure you've rebuilt the server:
```bash
cd build/example
make
./example_web
```

### Connection Refused

**Problem**: `ws.onerror` triggered immediately

**Solution**: Make sure the server is running on port 8080:
```bash
ps aux | grep example_web
```

### CORS Issues

**Problem**: WebSocket blocked by CORS policy

**Solution**: For browser testing, open the HTML file from the same origin or run a local web server.

## Next Steps

- Add authentication to WebSocket connections
- Implement WebSocket routing (different handlers for different paths)
- Add broadcast functionality (send to all connected clients)
- Implement ping/pong for connection health monitoring

---

**Status**: ✅ WebSocket Fully Functional
**Last Updated**: 2024
**Tested With**: Chrome, Firefox, Safari, curl, Python
