Hotline tunnel
==============

Mini peer to peer vpn. You can easily connect to home, office and IDC network
from outside.

Remote and local peers are connected directly because Hotline tunnel uses
**NAT traversal** technology.
You don't have to configiture port-forwarding or open firewall port.


### Usage ###
--------------

Remote peer:
```
$ htunnel -server [-p newpassword]
```

Local peer:
```
$ htunnel localport remotehost:port [-r room_id -p password]
```

If running remote peer, new room id will be displayed. In local peer set
the room id with -r option given by remote peer.



### Example ###
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
```

On your PC, run ssh client and connect to 127.0.0.1:9999.
Your ssh client will be connected to EC2 server's ssh service.

This will increase your security becuase no ssh port is opened to
public internet.
