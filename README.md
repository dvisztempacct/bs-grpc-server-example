# bs-grpc "chat" server example
This example requires bucklescript 3.1.5

This "chat" protocol is a bit odd because it uses polling to get updates to the
chat.

As of the time of this writing, I have included example validation criteria in `chat.proto` but it is more or less ignored by `bs-grpc`. Certainly my immediate target is full support for application-level validation in `bs-grpc`.

## Usage

```
# install dependencies
yarn install
# generate bucklescript bindings for your .proto
yarn reasonml-compile-proto chat.proto
# compile bucklesript code
bsb -make-world
# run server
node src/ChatServer.bs
```

Then in another shell, connect the provided Javascript client:

```
node src/jschatclient
```

Entering text on stdin to the client will send messages to the `#random` channel on the server. (Although it's not extremely careful about line cooking.)

Sending EOF (e.g. with `Control+D`) will close the client.

## TODO

- [ ] Finish implementing application-level validation in `bs-grpc`
- [ ] Finish the "nick" feature to identify users
- [ ] Replace timestamps with serial numbers
- [ ]   Client should obtain "last serial" from server
- [ ] Add support for multiple channels to client
- [ ] Do a streaming RPC implementation
