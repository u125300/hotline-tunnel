Hotline tunnel
==============

Mini peer to peer vpn. You can easily connect to home, office and IDC network from outside.

Don't need a VPN server and don't have to configiture port-forwarding or open firewall port.
Remote and local peers are connected directly because 'Hotline tunnel' uses NAT traversal technology.


### Usage ###
-------

> Remote peer: htunnel -server [-p newpassword]
> 
> Local peer: htunnel localport remotehost:port [-r room_id -p password

If you run remote peer, new room id will be displayed. In local peer you can set the room id with -r option.


### Example ###

Connect to AMAZON EC2 ssh service from your PC without opening port 22.

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
