import { URL } from "xrootd";
import assert from "node:assert/strict";
import { test } from 'node:test';
import { SERVER_URL } from "./env.mjs";

test("test_creation", () => {
  const u = new URL(SERVER_URL);
  assert.equal(u.toString(), SERVER_URL);
});

test("test_vaild", () => {
  const u = new URL(SERVER_URL);
  assert(u.isValid);
});

test("test_invaild", () => {
  const u = new URL("root://");
  assert(!u.isValid);
});

test("test_getters", () => {
  const u = new URL("root://user1:passwd1@host1:123//path?param1=val1&param2=val2#Anchor")
  assert(u.isValid);
  assert.strictEqual(u.hostId, 'user1:passwd1@host1:123');
  assert.strictEqual(u.protocol, 'root');
  assert.strictEqual(u.userName, 'user1');
  assert.strictEqual(u.password, 'passwd1');
  assert.strictEqual(u.hostName, 'host1');
  assert.strictEqual(u.port, 123);
  assert.strictEqual(u.path, '/path');
  assert.strictEqual(u.pathWithParams, '/path?param1=val1&param2=val2');
  assert.strictEqual(u.searchParams, '?param1=val1&param2=val2');
  assert.strictEqual(u.anchor, '#Anchor');
});

test("test_setters", () => {
  const u = new URL(SERVER_URL);
  assert(u.isValid);
  assert.strictEqual(u.toString(), SERVER_URL);
  u.protocol = 'xroot';
  assert.strictEqual(u.protocol, 'xroot');
  u.userName = 'user1';
  assert.strictEqual(u.userName, 'user1');
  u.password = 'passwd1';
  assert.strictEqual(u.password, 'passwd1');
  u.hostName = 'host1';
  assert.strictEqual(u.hostName, 'host1');
  u.port = 123;
  assert.strictEqual(u.port, 123);
  u.anchor = "Anchor";
  assert.strictEqual(u.anchor, '#Anchor');
  u.searchParams = "param1=val1&param2=val2";
  assert.strictEqual(u.searchParams, '?param1=val1&param2=val2');
  u.path = "/path";
  assert.strictEqual(u.path, '/path');
  // Overall test
  assert.strictEqual(u.toString(), "xroot://user1:passwd1@host1:123//path?param1=val1&param2=val2#Anchor")

  u.pathWithParams = "/path?param1=val1&param3=val3";
  assert.strictEqual(u.pathWithParams, "/path?param1=val1&param3=val3");
  assert.strictEqual(u.toString(), "xroot://user1:passwd1@host1:123//path?param1=val1&param3=val3#Anchor")
  u.pathWithParams = "/path";
  assert.strictEqual(u.pathWithParams, "/path");
  assert.strictEqual(u.toString(), "xroot://user1:passwd1@host1:123//path#Anchor")
});
