# bs-grpc "chat" server example
This example requires bucklescript 3.1.5

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
