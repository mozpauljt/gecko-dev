/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at <http://mozilla.org/MPL/2.0/>. */

// @flow

import { setupCommands, clientCommands } from "./firefox/commands";
import { setupEvents, clientEvents } from "./firefox/events";
import { features, prefs } from "../utils/prefs";
import type { Grip } from "../types";
let DebuggerClient;

function createObjectClient(grip: Grip) {
  return DebuggerClient.createObjectClient(grip);
}

export async function onConnect(connection: any, actions: Object) {
  const {
    tabConnection: { tabTarget, threadClient, debuggerClient },
  } = connection;

  DebuggerClient = debuggerClient;

  if (!tabTarget || !threadClient || !debuggerClient) {
    return;
  }

  const supportsWasm =
    features.wasm && !!debuggerClient.mainRoot.traits.wasmBinarySource;

  setupCommands({
    threadClient,
    tabTarget,
    debuggerClient,
    supportsWasm,
  });

  setupEvents({ threadClient, tabTarget, actions, supportsWasm });

  tabTarget.on("will-navigate", actions.willNavigate);
  tabTarget.on("navigate", actions.navigated);

  await threadClient.reconfigure({
    observeAsmJS: true,
    pauseWorkersUntilAttach: true,
    wasmBinarySource: supportsWasm,
    skipBreakpoints: prefs.skipPausing,
  });

  // Retrieve possible event listener breakpoints
  actions.getEventListenerBreakpointTypes();

  // In Firefox, we need to initially request all of the sources. This
  // usually fires off individual `newSource` notifications as the
  // debugger finds them, but there may be existing sources already in
  // the debugger (if it's paused already, or if loading the page from
  // bfcache) so explicity fire `newSource` events for all returned
  // sources.
  const traits = tabTarget.traits;
  await actions.connect(
    tabTarget.url,
    threadClient.actor,
    traits && traits.canRewind,
    tabTarget.isWebExtension
  );

  const fetched = clientCommands
    .fetchSources()
    .then(sources => actions.newGeneratedSources(sources));

  // If the threadClient is already paused, make sure to show a
  // paused state.
  const pausedPacket = threadClient.getLastPausePacket();
  if (pausedPacket) {
    clientEvents.paused(threadClient, pausedPacket);
  }

  return fetched;
}

export { createObjectClient, clientCommands, clientEvents };
