#!/usr/bin/env python3

from nclib import netcat
import sys

MENU_END = b'(4) Quit\n'

TEST_LOG_NAME = b'adamd'
TEST_LOG_CONTENT = b'I am the content content'

def main():

    host = sys.argv[1]
    port = int(sys.argv[2])

    conn = netcat.Netcat((host, port))

    result = conn.recvuntil(MENU_END)

    to_send = b"2"
    conn.sendline(to_send)

    result = conn.recvuntil(MENU_END)
    assert b"Logs:" in result
    assert not TEST_LOG_NAME in result
    assert not TEST_LOG_CONTENT in result

    to_send = b"1"
    conn.sendline(to_send)

    result = conn.recvuntil(b'What is the name of the log?\n')
    conn.sendline(TEST_LOG_NAME)

    result = conn.recvuntil(b'What is the content of the log?\n')
    conn.sendline(TEST_LOG_CONTENT)

    result = conn.recvuntil(MENU_END)

    to_send = b"2"
    conn.sendline(to_send)

    result = conn.recvuntil(MENU_END)
    assert TEST_LOG_NAME in result

    to_send = b"3"
    conn.sendline(to_send)

    result = conn.recvuntil(b"What is the log number?\n")

    to_send = b"1"
    conn.sendline(to_send)

    result = conn.recvuntil(MENU_END)
    assert TEST_LOG_NAME in result
    assert TEST_LOG_CONTENT in result

    second_log_name = b"log2"
    second_log_content = b"content2"

    to_send = b"1"
    conn.sendline(to_send)

    result = conn.recvuntil(b'What is the name of the log?\n')
    conn.sendline(second_log_name)

    result = conn.recvuntil(b'What is the content of the log?\n')
    conn.sendline(second_log_content)

    result = conn.recvuntil(MENU_END)

    to_send = b"2"
    conn.sendline(to_send)
    result = conn.recvuntil(MENU_END)
    assert TEST_LOG_NAME in result, result
    assert second_log_name in result, result

    to_send = b"3"
    conn.sendline(to_send)

    result = conn.recvuntil(b"What is the log number?\n")

    to_send = b"5"
    conn.sendline(to_send)

    result = conn.recvuntil(MENU_END)
    assert not TEST_LOG_NAME in result
    assert not TEST_LOG_CONTENT in result
    assert not second_log_name in result
    assert not second_log_content in result
    assert b'Cannot find that log' in result

    to_send = b"3"
    conn.sendline(to_send)

    result = conn.recvuntil(b"What is the log number?\n")

    to_send = b"2"
    conn.sendline(to_send)

    result = conn.recvuntil(MENU_END)
    assert second_log_name in result
    assert second_log_content in result

    to_send = b"4"
    conn.sendline(to_send)

    result = conn.recvuntil(b'Goodbye!')
    assert b'So long' in result

    sys.exit(0)


if __name__ == '__main__':
    main()
    

