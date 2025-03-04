/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* vim: set ft=javascript ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

loader.lazyRequireGetter(this, "getWarningGroupType", "devtools/client/webconsole/utils/messages", true);
loader.lazyRequireGetter(this, "getParentWarningGroupMessageId", "devtools/client/webconsole/utils/messages", true);

function getAllMessagesById(state) {
  return state.messages.messagesById;
}

function getMessage(state, id) {
  return getAllMessagesById(state).get(id);
}

function getAllMessagesUiById(state) {
  return state.messages.messagesUiById;
}
function getAllMessagesPayloadById(state) {
  return state.messages.messagesPayloadById;
}

function getAllGroupsById(state) {
  return state.messages.groupsById;
}

function getCurrentGroup(state) {
  return state.messages.currentGroup;
}

function getVisibleMessages(state) {
  return state.messages.visibleMessages;
}

function getFilteredMessagesCount(state) {
  return state.messages.filteredMessagesCount;
}

function getAllRepeatById(state) {
  return state.messages.repeatById;
}

function getAllNetworkMessagesUpdateById(state) {
  return state.messages.networkMessagesUpdateById;
}

function getGroupsById(state) {
  return state.messages.groupsById;
}

function getPausedExecutionPoint(state) {
  return state.messages.pausedExecutionPoint;
}

function getAllWarningGroupsById(state) {
  return state.messages.warningGroupsById;
}

function isMessageInWarningGroup(state, message) {
  if (!getWarningGroupType(message)) {
    return false;
  }

  return getVisibleMessages(state).includes(getParentWarningGroupMessageId(message));
}

module.exports = {
  getAllGroupsById,
  getAllWarningGroupsById,
  getAllMessagesById,
  getAllMessagesPayloadById,
  getAllMessagesUiById,
  getAllNetworkMessagesUpdateById,
  getAllRepeatById,
  getCurrentGroup,
  getFilteredMessagesCount,
  getGroupsById,
  getMessage,
  getVisibleMessages,
  getPausedExecutionPoint,
  isMessageInWarningGroup,
};
