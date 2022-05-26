# Router UDP NAT Bug Tester
By: Sad Pencil

This program sends multiple DNS queries to `8.8.8.8` in parallel with the same UDP sender ports and expects to receive all the answers. It is useful to test whether the NAT gateway is buggy and causes DNS lookup timeouts for Linux devices. See [Racy conntrack and DNS lookup timeouts](https://www.weave.works/blog/racy-conntrack-and-dns-lookup-timeouts) for technical details.

## Compile
```bash
make
```

## Usage
```bash
./client
./client [thread-num]
```

## Examples

```bash
$ ./client
Thread 0 sent a DNS query.
Thread 1 sent a DNS query.
Thread 1 received a DNS answer with 48 bytes. Time: 17 ms.
Thread 0 received a DNS answer with 48 bytes. Time: 17 ms.
# note: two packets arrives successfully
```

```bash
$ ./client
Thread 0 sent a DNS query.
Thread 1 sent a DNS query.
Thread 0 received a DNS answer with 48 bytes. Time: 17 ms.
^C # note: the second packet can't arrive, indicating the bug of the NAT gateway
```

## Note
It is recommended to compile this project using glibc instead of others.
This is because the interval between two UDP packets MUST be sent as close as possible, e.g., `~0.00002s`, to reproduce this bug.
Other C libraries are much slower than glibc.

Besides, it is recommended to use a modern CPU that has enough single-core performance to run this tool.

## Acknowledgment
This project is modified from a dns client at [uberj/DNS-Client](https://github.com/uberj/DNS-Client).