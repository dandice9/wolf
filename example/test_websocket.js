#!/usr/bin/env node

// Simple WebSocket test client for Wolf server
const WebSocket = require('ws');

console.log('🐺 Wolf WebSocket Test Client\n');
console.log('Connecting to ws://localhost:8080...\n');

const ws = new WebSocket('ws://localhost:8080');

ws.on('open', function open() {
    console.log('✅ Connected successfully!');
    console.log('Sending test message...\n');
    ws.send('Hello, Wolf Server!');
});

ws.on('message', function incoming(data) {
    console.log('📨 Received:', data.toString());
    
    // Send a few more test messages
    setTimeout(() => {
        console.log('\nSending: "Test message 2"');
        ws.send('Test message 2');
    }, 1000);
    
    setTimeout(() => {
        console.log('Sending: "Final test"');
        ws.send('Final test');
    }, 2000);
    
    setTimeout(() => {
        console.log('\n✅ All tests passed!');
        console.log('Closing connection...');
        ws.close();
    }, 3000);
});

ws.on('error', function error(err) {
    console.error('❌ Error:', err.message);
    process.exit(1);
});

ws.on('close', function close() {
    console.log('\n🔌 Connection closed');
    process.exit(0);
});

// Timeout after 10 seconds
setTimeout(() => {
    console.error('\n❌ Timeout - connection failed');
    process.exit(1);
}, 10000);
