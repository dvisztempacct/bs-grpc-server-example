let serverAddress = "127.0.0.1:12345";
let credentials = Grpc.Client.createInsecure();
let chatServiceClient =
  Grpc.Chat.ChatService.Client.makeClient(serverAddress, credentials);
let payload =
  Grpc.Chat.Some64BitValues.make(
    ~fix64="1",
    ~sfix64="2",
    ~varint64="3",
    ~svarint64="4",
    ~uvarint64="5",
    (),
  );
chatServiceClient
|. Grpc.Chat.ChatService.Client.Echo64Rpc.invoke(payload, (err, res) =>
     if (err |. Js.Nullable.test) {
       Js.log2("got result back:", res);
     } else {
       Js.log2("got err back:", err |. Js.String.make);
     }
   );
