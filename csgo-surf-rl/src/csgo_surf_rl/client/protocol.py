"""Wire-protocol constants shared with the C++ telemetry plugin.

Keep these in lockstep with plugin/include/surf_protocol.h and
plugin/PROTOCOL.md.
"""

from __future__ import annotations

PROTOCOL_VERSION = 1
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 42813
