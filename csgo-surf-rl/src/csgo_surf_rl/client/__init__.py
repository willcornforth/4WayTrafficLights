"""The client data domain: everything the model reads or writes lives here.

Observations are ClientState (what the client sees), actions are UserCmd
(what the client sends). GameClientBridge is the only interface between the
harness and a game client; MockSurfBridge is an offline stand-in.
"""

from csgo_surf_rl.client.bridge import GameClientBridge
from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.mock_bridge import MockSurfBridge
from csgo_surf_rl.client.socket_bridge import SocketClientBridge
from csgo_surf_rl.client.usercmd import Buttons, UserCmd

__all__ = [
    "Buttons",
    "ClientState",
    "GameClientBridge",
    "MockSurfBridge",
    "SocketClientBridge",
    "UserCmd",
]
