# Node to Node Protocol

Based on UDP with content in JSON, N2N protocol defines functions of a NOT centralized local network.

# Subjects

## Device

Device is the instance in the local network which can be identify with mDNS.

Device can be one of Node, Terminal or Proxy.

## Node

Node refers to an ESP32 embeded module, which has resources to perform some functions. In functional perspect, a Node may contains two or more Entry Points, each of these is dedicated to specific function.

Node act as a UDP server not a client.

Node publish what functions it would provide and how.

Node ID is the identical number which is conducted from MAC address.

## Entry Point

Entry points of a Node has be identified by sequence number from 0.

0     - reserved for UDP Server, which implements N2N proto and Node registration, communication, etc.
1 ~ N - specific function to be performed such as environment detectation based a professional sonsor, led strip, etc.

## Terminal

Terminal may or may not provide human machine interface, provide a interface to use functions of Nodes or to manipulate or to supervise the Node.

Terminal act as both UDP client and server.

## Proxy

Porxy provide the ability for N2N network to inter-acting with other network like MQTT.

Proxy act as a UDP proxy.

# Procedure

## Discovery

With mDNS, broadcasting its attributes to the local network, every device(Node, Terminal, Proxy, etc) can be discovered.

- instance_name: device name
- service_type: _n2n
- proto: _udp
- port: 3999
- txt: Defined TXT keys:
    version         N2N proto version
    auth_type       type of authorization
    query_path      path for query description of device

## Query

Query procedure:

- Invoker: Terminal or Proxy
- CON(Confirmable Message), query type
- Remark: name filter, ability filter, ...

After receiving GET, Node would ignore it if filter not matched. Otherwise, Node shall piggybacked reponse with its Node Description.

Discovery can be occured at the moment when invoker power up, or when invoker scheduled.


## Observation

Observation procedure:

- Invoker: Terminal or Proxy
- CON(Confirmable Message), subscribe type
- Remark: range filter, ...

## Perform

Perform procedure:

- Invoker: Terminal or Proxy
- CON(Confirmable Message), command type
- Remark: Action Command, ...

# Notation

## Node Description

- name: logic name defined by user
- brand: [optional]
- mfr: [optional]
- model: [optional]
- pd: [optional]
- query_route: route for query status of Node
- query_schema: schema of query result body
- Array of Entry Point Description

## Termincal Description

- name: logic name defined by user
- brand: [optional]
- mfr: [optional]
- model: [optional]
- pd: [optional]
- query_route: route for query status of Terminal
- query_schema: schema of query result body

## Proxy Description

- name: logic name defined by user
- brand: [optional]
- mfr: [optional]
- model: [optional]
- pd: [optional]
- query_route: route for query status of Proxy
- query_schema: schema of query result body

## Entry Point Description
- name: logic name defined by user
- observe_route: route for registering observer
- observe_schema: schema of notification body
- perform_route: route for performing action
- perform_schema: schema of Action body


# Message Format

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     ver      |      type     |           msg_id              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            code              |         token                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                 route + JSON (if any) ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

hostname ---> MAC address

route ---> dev_name

instance name ---> mDNS ---> IP address

