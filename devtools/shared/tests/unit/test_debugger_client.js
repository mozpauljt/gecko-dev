/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

// DebuggerClient tests

const { DebuggerServer } = require("devtools/server/main");
const { DebuggerClient } = require("devtools/shared/client/debugger-client");

add_task(async function() {
  await testCloseLoops();
  await fakeTransportShutdown();
});

function createClient() {
  DebuggerServer.init();
  DebuggerServer.registerAllActors();
  const client = new DebuggerClient(DebuggerServer.connectPipe());
  return client;
}

// Ensure that closing the client while it is closing doesn't loop
async function testCloseLoops() {
  const client = createClient();
  await client.connect();

  await new Promise(resolve => {
    let called = false;
    client.on("closed", async () => {
      dump(">> CLOSED\n");
      if (called) {
        ok(false, "Calling client.close from closed event listener introduce loops");
        return;
      }
      called = true;
      await client.close();
      resolve();
    });
    client.close();
  });
}

// Check that, if we fake a transport shutdown (like if a device is unplugged)
// the client is automatically closed, and we can still call client.close.
async function fakeTransportShutdown() {
  const client = createClient();
  await client.connect();

  await new Promise(resolve => {
    const onClosed = async function() {
      client.off("closed", onClosed);
      ok(true, "Client emitted 'closed' event");
      resolve();
    };
    client.on("closed", onClosed);
    client.transport.close();
  });

  await client.close();
  ok(true, "client.close() successfully resolves");
}
