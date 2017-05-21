# System Design

DagBox uses a distributed design. Every component of the system is
independant from the other components. This makes it possible to run
every component concurrently, and it gives us several options on how
to run the system. Each component can be run on separate threads in a
single process, or separate processes in a single machine, or even in
multiple machines distributed accross a network.

The components of DagBox communicate using message passing. We have
chosen [ZeroMQ](http://zeromq.org/) (ØMQ) for messaging as it supports
a variety of transports such as IPC and TCP, and when used in-process
it has very minimal overhead. These properties allow DagBox to
effectively perform when used as a small embedded database, while
still allowing it to scale up when necessary.

![An overview of the architecture of DagBox.](resources/architecture-overview.svg)

At the core of DagBox, a message broker is responsible for routing the
communications within the system. This message broker acts as a
central point for all other components and the clients to connect
to. Everything only directly communicates with the broker, the broker
is responsible for routing those messages to their intended targets.

DagBox uses a service-oriented approach. The components of the system,
workers, register the services they are able to provide to the broker
and clients request specific services. The broker matches the requests
with workers. Here, the broker performs load distribution: the
requests are queued by the broker and distributed to the workers as
they become free to do more work.

Requests sent to the workers are serialized
with [MessagePack](http://msgpack.org/). MessagePack is a very
efficient binary serialization format, with support for many
programming languages.

The flexible design of DagBox makes it possible to write workers that
perform many kinds of tasks. In it's current state DagBox comes with
three kinds of services: data storage, logic, and locking.

## Data Storage

The data storage workers provide a key-value storage. They
use [LMDB](https://symas.com/lightning-memory-mapped-database/) to
store the data. We have chosen LMDB as it provides us very high
resistance to corruption and doesn't require any special recovery
procedures after crashes.

The data store is split into buckets, each stored in a separate LMDB
databases in a single storage environment. These buckets allow clients
to group data based on their needs. All data storage workers have
access to all buckets.

We have two kinds of data store workers: data readers and data
writers. This distinction is useful because LMDB is able to work
without any locking when there is only 1 writer and multiple readers.

Data read requests are serialized with MessagePack. In JSON form, they
would look like:

```
{
    'bucket': 'user',
    'key': 'f3eb6d44-eedf-484f-af0c-fe9ee79c4e89',
    'relations': {
        "comments": [
            {
                'bucket': 'comment',
                'key': '12bf8e82-837a-43a0-abf9-3324e6ae1465'
            }
        ]
    }
}
```

This structure may repeat recursively by using the "relations" key in
the objects. This allows very complex requests to be constructed. The
worker will reply with a similar message, with "data" keys in each
object that contain the data for the corresponding key.

Similarly, write requests are also serialized with MessagePack. In
JSON form:

```
{
    'bucket': 'user',
    'data': '...........'
}
```

The worker will respond with the key that was created to store the
data.


## Logic

The logic workers are designed to maintain a rule and fact database,
and perform queries on them. Logic workers are only made aware of the
keys of data, the actual data is only stored in data storage workers.
In DagBox it is possible for workers to cooperate on a single request,
this capability is used by the logic workers to perform the query and
then pass the request to data storage workers to retrieve the actual
data.

The implementation of logic workers is not yet complete.

## Locking

Locking worker provides allows clients to lock or unlock keys. This
can be used by the clients to avoid race conditions in their
applications by first locking the keys they will modify.

## Message Broker

![An example state for the broker.](resources/broker.svg)

The message broker keeps queues for every service. When under load,
the requests are queued up to be assigned to workers as they become
free. The broker also keeps track of all registered workers, ensuring
that they haven't crashed before assigning work to them.
