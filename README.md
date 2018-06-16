# bs-grpc "chat" server example
This example requires bucklescript 3.1.5

This "chat" protocol is a bit odd because it uses polling to get updates to the
chat.

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
## TODO

[ ] Finish the "nick" feature to identify users
[ ] Replace timestamps with serial numbers
[ ]   Client should obtain "last serial" from server
[ ] Add support for multiple channels to client
[ ] Do a streaming RPC implementation
