"""Send a QEMU debugging command to the isolated local test VM."""
import json
import socket
import sys


def command(request):
    with socket.create_connection(('127.0.0.1', 55551), timeout=3) as sock:
        with sock.makefile('rwb', buffering=0) as connection:
            if not connection.readline():
                raise EOFError('QMP disconnected')

            def exchange(message):
                connection.write(json.dumps(message).encode() + b'\n')
                while True:
                    line = connection.readline()
                    if not line:
                        raise EOFError('QMP disconnected')
                    result = json.loads(line)
                    if 'return' in result or 'error' in result:
                        return result

            exchange({'execute': 'qmp_capabilities'})
            return exchange(request)


if __name__ == '__main__':
    print(command(json.loads(sys.argv[1])))
