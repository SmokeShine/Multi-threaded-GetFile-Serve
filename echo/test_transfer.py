# test_transfer.py
import unittest
import functools
import subprocess
import socket

popen = functools.partial(
    subprocess.Popen,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    cwd="/workspace/pr1/transfer/",
)


class TestClient(unittest.TestCase):
    def setUp(self):
        with popen(["make"]) as p:
            self.assertEqual(p.wait(1), 0)

    def test_connects_to_port_ipv4(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as test_server:
            test_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            test_server.bind(("", 3254))
            test_server.listen(1)
            test_server.settimeout(1)
            with popen(["./transferclient", "-p", "3254"]) as p:
                try:
                    conn, _ = test_server.accept()
                except socket.timeout:
                    self.assert_(False, "Should not timeout")
                conn.close()

    def test_connects_to_port_ipv6(self):
        with socket.socket(socket.AF_INET6, socket.SOCK_STREAM) as test_server:
            test_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            test_server.bind(("", 3254))
            test_server.listen(1)
            test_server.settimeout(1)
            with popen(["./transferclient", "-p", "3254"]) as p:
                try:
                    conn, _ = test_server.accept()
                except socket.timeout:
                    self.assert_(False, "Should not timeout")
                conn.close()

    def test_writes_to_file(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as test_server:
            test_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            test_server.bind(("", 5034))
            test_server.listen(1)
            test_server.settimeout(1)
            with popen(["rm", "test_writes_to_file.txt"]) as p:
                p.wait()
            with popen(
                ["./transferclient", "-p", "5034", "-o", "test_writes_to_file.txt"]
            ) as p:
                conn, _ = test_server.accept()
                conn.sendall(b"This should be a file!!!")
                conn.close()
                p.wait(1)
            with popen(["cat", "test_writes_to_file.txt"]) as p:
                contents, err = p.communicate()
                self.assertEqual(contents, b"This should be a file!!!", err)
            with popen(["rm", "test_writes_to_file.txt"]) as p:
                p.wait()

if __name__ == "__main__":
    unittest.main()
