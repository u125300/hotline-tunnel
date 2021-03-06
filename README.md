Hotline tunnel
==============

Mini peer to peer vpn. You can easily connect to home, office and IDC network
from outside without VPN server.

Remote and local peers are connected directly because Hotline tunnel supports
**NAT traversal** technology.
You don't have to configiture port-forwarding or firewall ports.


### Usage ###
--------------

Remote peer:
```
$ htunnel -server [-p newpassword]
```

Local peer:
```
$ htunnel localport remotehost:port -r room_id [-p password -udp]
```

If running remote peer, new room id will be displayed. In local peer set
the room id with -r option given by remote peer.



### Example - Retote desktop ###
---------------

Connect to **VNC server** from **VNC viewer** without opening
*vnc port(5900)* in your router or firewall.

VNC server side:
```
$ htunnel -server -p yourpassword
Your room id is 12345.
```

VNC viewer side:
```
$ htunnel 9999 192.168.0.123:5900 -r 12345 -p yourpassword
Connected. Local socket(0.0.0.0:9999) opened.

```

Where 192.168.0.123:5900 is the ip address:port of vnc server
and 12345 is the room id assigned by htunnel on vnc server side.

Then run *vnc viewer* and connect to 127.0.0.1:9999.



### Example - Amazon EC2 ###
---------------

Connect to **AMAZON EC2** ssh service from **your PC** without opening
*ssh port(22)* in EC2 security group.

EC2 side:
```
$ htunnel -server -p yourpassword
Your room id is 12345.
```

PC side:
```
$ htunnel 9999 127.0.0.1:22 -r 12345 -p yourpassword
Connected. Local socket(0.0.0.0:9999) opened.

$ ssh 127.0.0.1 -p 9999
Ssh client will be connected to EC2 server.
```

On your PC, run ssh client and connect to 127.0.0.1:9999.
Your ssh client will be connected to EC2 server's ssh service.

This will increase your security becuase no ssh port is opened to
public internet.
