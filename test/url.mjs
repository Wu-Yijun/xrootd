/// <reference types="node" />

// =====================================
//        xrootd python test
// =====================================
// from XRootD import client
// import pytest, sys
// from env import *

// def test_creation():
//   u = client.FileSystem(SERVER_URL).url
//   assert u is not None

// def test_deletion():
//   u = client.FileSystem(SERVER_URL).url
//   del u
//   pytest.raises(NameError, eval, "u")

// def test_valid():
//   u = client.FileSystem(SERVER_URL).url
//   assert u.is_valid()

// def test_invalid():
//   u = client.FileSystem('root://').url
//   assert u.is_valid() == False

// def test_getters():
//   u = client.FileSystem("root://user1:passwd1@host1:123//path?param1=val1&param2=val2").url
//   assert u.is_valid()
//   assert u.hostid == 'user1:passwd1@host1:123'
//   assert u.protocol == 'root'
//   assert u.username == 'user1'
//   assert u.password == 'passwd1'
//   assert u.hostname == 'host1'
//   assert u.port == 123
//   assert u.path == '/path'
//   assert u.path_with_params == '/path?param1=val1&param2=val2'

// def test_setters():
//   u = client.FileSystem(SERVER_URL).url
//   u.protocol = 'root'
//   assert u.protocol == 'root'
//   u.username = 'user1'
//   assert u.username == 'user1'
//   u.password = 'passwd1'
//   assert u.password == 'passwd1'
//   u.hostname = 'host1'
//   assert u.hostname == 'host1'
//   u.port = 123
//   assert u.port == 123
//   u.path = '/path'
//   assert u.path == '/path'
//   u.clear()
//   assert str(u) == ''

// =====================================
//          typescript test
// =====================================
import { URL } from "../dist/index.mjs";
// import { Env, FileSystem, URL } from "../lib/index.ts";
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
