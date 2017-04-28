# Pub-Sub-Project
Distributed Systems Principles Final Project -- May.2017
## Improvements
This project make following improvements based on original pub-sub project:

Core features:

0. history: keep a history, and when subscriber first subscribe the topic, give him history first.

0. ownership strength: If two messages of same topic are received in the same time, the one with higher ownership survives.

1. message frequency: how often (in milliseconds) central service send messages to subscribers. note: 0 will be instantly.

2. resource limit: specify how long should central service keep the history.

3. time tag: every message born with a time tag which specify when this message is born.

4. life span: every message born with an life span, if life span passed before central service send it to subscriber, the message should be dropped.

5. destination order: every subscriber are guaranteed to receive the same order of messages.

6. availability: some topics may beg not appropriate for certain group of subscriber, publishers have right to decide each of their messages is available for which kind of subscriber group.

Other useful features:

1. pub/sub ID: every publisher/subscriber has one unique ID in their communication and logging.

2. subscriber interface: now we could manually subscribe or unsubscribe topics at anytime.

Implementation Details:

0. history: central service will store message history. Each topic has its own repository.

0. ownership strength: central service will send message to subscribers every #message frequency milliseconds.  Before sending messages, central service will loop through all messages and find those who have the same topic AND AVAILABLE GROUP, but different ownership strength. Message with higher ownership strength survive.

1. message frequency: when initialize central service, we can specify message frequency (in milliseconds), and central service will send message to subscribers every #message frequency milliseconds. Before sending message, central service will store messages to its internal buffer.

2. resource limit: when initialize central service, we can specify resource limit, which is the maximum length of history for each topic. Note central service store history according to topic. Each topic has its own repository, and the resource limit will be assigned to each repository.

3. time tag: every message born with a time tag which specify when this message is born.

4. life span: just like description above, every message born with an attribute called life span (in seconds), this should be assigned when writer give a message to publisher. Publisher will pass this information to central service. Before resending the message to subscriber, central service will check if its life span has been passed, if so, just drop the message.

5. destination order: to guarantee a consistent destination order, we maintained a message buffer for each subscriber. Subscriber will store its received messages in message buffer first, then every 30 seconds, another thread will check the message buffer, sort messages according to their born time.

6. availability: when initialize subscriber, we can specify which group this subscriber is belong to, and every message that publisher send should attach a group tag, only messages with the same group tag could be view by subscriber.

WorkFlow:

* Publisher: publishers have only one thread, it makes up some fake information and fill out critical attributes including availableGroup, strength, lifeSpan, timeTag, then send them to central service.

* Central Service: central service has two independent threads: receiver and sender.

    * Receiver thread will listen to the receiving port and keep receive messages from publishers, it will then store messages into the message buffer.

    * Sender thread will clear message buffer and send messages every X milliseconds, where X is specified by message frequency attribute. Before sending messages, it will also pre-process messages to enforce features including ownership strength and life span. And after sending messages, it will push all sent messages into history buffer.

    * History buffer: history buffer is a list of ring buffers with capacity specified by resource limit attribute. each buffer is associated with a topic, so that messages from different topic will be stored in different ring buffers.

* Subscriber: subscribers have four independent threads: receiver, inspector, logger, and interface.

    * History table: when user first subscribe a topic, the topic will automatically be added onto the history table, so that receiver could receive history messages for this topic. But we don't want to keep receiving history messages, thus there is a inspector thread periodically remove old entries from history table.

    * Receiver thread will keep receiving messages from central service, and preserve those are on subscription table and for the correct available group. Receiver will put all messages into a internal message buffer.

    * Inspector thread will check history table every 10 seconds, nothing should remain on history table for over 10 seconds.

    * Logger thread will clear message buffer every 30 seconds. Before logging them to local file, logger will pre-process those messages to enforce features including destination order.


* Message format (separated by '/n' when combining to a string message):

    * string topic

    * string content

    * int availableGroup

    * int ownershipStrength

    * int lifeSpan  --> in seconds

    * int timeTag

    * int isHistory

## how to compile and run the test

### compile

* install zmq library, boost library

* clone the repo to your local machine

* enter the project folder

* `$ make`

* then you will find three executable in current folder named "publisher", "subscriber", "center".

### build network

* enter the project folder

* `$ sudo python mr_mininet.py -p 5550 -r 3 -M 10 -R 1`

### run pub/sub/center

#### first we run center on h1s1

Note: here we could specify some parameter about the center (format: key=value, no space in between), or just leave them as default:

* rec("5555") --> _receiver_port

* send("5556") --> _sender_port

* period(5000) --> _send_period

* reslimit(10) --> _resource_limit

* path("centerLog.log") --> _log_path

###### example

* `$ xterm h1s1`
* `./center`

#### then we run some publishers on some hosts

Note: here we could specify some parameter about the publisher, or just leave them as default:

* ip("localhost")

* port("5555")

* dir("./") --> log dir

* first(0)  --> is this the first publisher in this test? (this will automatically give publisher unique ID)

* strength(1) --> ownership strength of this publisher

Note: each publisher will generate randomized messages every 1 second. To be specific, the randomized messages will be:

* topic: topic0 ~ topic4

* availableGroup: 0~1

* strength: specified by command line parameter

* lifeSpan: 10 sec or 0 sec

* timeTag: born time

* content: a string of above all information.

###### example

* `$ xterm h2s2`
* `./publisher first=1`
* `$ xterm h3s2`
* `./publisher`
* `$ xterm h4s2`
* `./publisher strength=2`
* `$ xterm h5s2`
* `./publisher strength=2`
* `$ xterm h6s2`
* `./publisher strength=3`

#### then we run some subscribers on some hosts

Note: here we could specify some parameter about the publisher, or just leave them as default:

* ip("localhost")

* port("5555")

* dir("./") --> log dir

* first(0)  --> is this the first subscriber in this test? (this will automatically give subscriber unique ID)

* group(0) --> the group this subscriber is belonged to

###### example

* `$ xterm h7s2`
* `./subscriber first=1`
* `$ xterm h8s2`
* `./subscriber`
* `$ xterm h9s2`
* `./subscriber group=1`
* `$ xterm h10s2`
* `./subscriber group=1`
* `$ xterm h11s2`
* `./subscriber group=2`

Then we could subscribe or unsubscribe topics through command line interface:

* sub topic0 --> subscribe topic0

* unsub topic1 --> unsubscribe topic1

* list --> print all subscriptions

We provide a sample result in the test_result folder.