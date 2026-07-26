import subprocess

BINARY_PATH = "./sikradio"


def read_request(conn):
    """Read from conn until the blank line ending HTTP headers."""
    data = b""
    conn.settimeout(3.0)
    while b"\r\n\r\n" not in data and b"\n\n" not in data:
        try:
            chunk = conn.recv(4096)
        except socket.timeout:
            break
        if not chunk:
            break
        data += chunk
    return data.decode(errors="replace")

def http_200(conn, body=b"AUDIO", metaint=None):
    headers  = "HTTP/1.1 200 OK\r\n"
    headers += "Content-Type: audio/mpeg\r\n"
    if metaint is not None:
        headers += f"icy-metaint: {metaint}\r\n"
    headers += "Connection: close\r\n\r\n"
    conn.sendall(headers.encode() + body)

def handler(conn):
    read_request(conn)
    http_200(conn, body=payload)


import socket
import threading


class MockServerV6:
    """
    Single-connection IPv6-only TCP server bound to ::1.
    Mirrors the interface of MockServer; IPV6_V6ONLY is set so it never
    accidentally accepts IPv4-mapped connections.
    """

    def __init__(self, handler):
        self.handler = handler
        self._sock   = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
        self._sock.setsockopt(socket.SOL_SOCKET,    socket.SO_REUSEADDR,  1)
        self._sock.setsockopt(socket.IPPROTO_IPV6,  socket.IPV6_V6ONLY,   1)
        #!/usr/bin/env python3
        """Standalone debugger test for IPv6 literal URL handling.

        Usage:
          python3 debuger.py
          python3 debuger.py -v
          python3 -m pytest -q debuger.py --color=yes
        """

        import os
        import socket
        import subprocess
        import threading
        import unittest


        CLIENT_BIN_CANDIDATES = (
            "./sikradio",
            "./cmake-build-release/sikradio",
            "./cmake-build-debug/sikradio",
        )
        CLIENT_BIN = None


        def read_request(conn):
            """Read one HTTP request until header terminator."""
            data = b""
            conn.settimeout(3.0)
            while b"\r\n\r\n" not in data and b"\n\n" not in data:
                try:
                    chunk = conn.recv(4096)
                except socket.timeout:
                    break
                if not chunk:
                    break
                data += chunk
            return data.decode(errors="replace")


        def http_200(conn, body=b"AUDIO"):
            headers = (
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: audio/mpeg\r\n"
                "Connection: close\r\n\r\n"
            )
            conn.sendall(headers.encode() + body)


        def _ipv6_available():
            try:
                s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            except OSError as e:
                return False, f"kernel has no AF_INET6 support: {e}"
            try:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)
                s.bind(("::1", 0))
                s.listen(1)
                port = s.getsockname()[1]
                c = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
                c.settimeout(1.0)
                c.connect(("::1", port))
                c.close()
                s.close()
                return True, "ok"
            except OSError as e:
                return False, f"::1 loopback unusable: {e}"


        _IPV6_OK, _IPV6_REASON = _ipv6_available()


        class MockServerV6:
            def __init__(self, handler):
                self.handler = handler
                self._sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
                self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                self._sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)
                self._sock.bind(("::1", 0))
                self._sock.listen(4)
                self._sock.settimeout(5.0)
                self.port = self._sock.getsockname()[1]
                self._thread = threading.Thread(target=self._serve, daemon=True)

            def start(self):
                self._thread.start()
                return self

            def stop(self):
                try:
                    self._sock.close()
                except OSError:
                    pass

            def _serve(self):
                try:
                    conn, _ = self._sock.accept()
                except OSError:
                    return
                try:
                    self.handler(conn)
                finally:
                    try:
                        conn.close()
                    except OSError:
                        pass

            def url(self, path="/stream"):
                return f"http://[::1]:{self.port}{path}"


        class DebugIpv6LiteralTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                global CLIENT_BIN
                for exe in CLIENT_BIN_CANDIDATES:
                    if os.path.exists(exe):
                        CLIENT_BIN = exe
                        break
                if CLIENT_BIN is None:
                    raise FileNotFoundError(
                        f"sikradio binary not found in {CLIENT_BIN_CANDIDATES}"
                    )
                if not _IPV6_OK:
                    raise unittest.SkipTest(f"IPv6 not available on this host: {_IPV6_REASON}")

            def launch_client(self, args, timeout=15):
                proc = subprocess.Popen(
                    [CLIENT_BIN] + args,
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                try:
                    out, err = proc.communicate(timeout=timeout)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    out, err = proc.communicate()
                    self.fail(f"client timed out: args={args}, stderr={err.decode(errors='replace')}")
                return proc.returncode, out, err

            def test_bare_ipv6literal(self):
                payload = b"LITERAL_IPV6_OK"

                def handler(conn):
                    read_request(conn)
                    http_200(conn, body=payload)

                srv = MockServerV6(handler).start()
                self.addCleanup(srv.stop)

                args = ["-u", srv.url(), "-6", "-t", "3000"]
                rc, out, err = self.launch_client(args)

                self.assertEqual(
                    0,
                    rc,
                    "parse_url should handle RFC 2732 IPv6 literal [::1] bracket notation; "
                    f"stderr={err.decode(errors='replace')!r}",
                )
                self.assertIn(payload, out)


        if __name__ == "__main__":
            unittest.main(verbosity=2)