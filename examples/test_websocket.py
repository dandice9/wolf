#!/usr/bin/env python3
"""
Simple WebSocket test client for Wolf server
Requires: pip install websockets
"""

import asyncio
import websockets

async def test_websocket():
    print("🐺 Wolf WebSocket Test Client\n")
    print("Connecting to ws://localhost:8080...\n")
    
    try:
        async with websockets.connect('ws://localhost:8080') as websocket:
            print("✅ Connected successfully!\n")
            
            # Send test message
            test_msg = "Hello, Wolf Server!"
            print(f"📤 Sending: {test_msg}")
            await websocket.send(test_msg)
            
            # Receive echo
            response = await websocket.recv()
            print(f"📨 Received: {response}\n")
            
            # Send more messages
            for i in range(1, 4):
                msg = f"Test message {i}"
                print(f"📤 Sending: {msg}")
                await websocket.send(msg)
                
                response = await websocket.recv()
                print(f"📨 Received: {response}\n")
                
                await asyncio.sleep(0.5)
            
            print("✅ All tests passed!")
            
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(asyncio.run(test_websocket()))
