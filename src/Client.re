let serverAddress = "127.0.0.1:12345";

[@bs.val] external dirname : string = "__dirname";

let certPath = dirname ++ "/../certs/";

let metadataGenerator =
  (
    ((), complete) =>
      complete(
        Js.Nullable.null,
        Grpc.Client.Metadata.make()
        |. Grpc.Client.Metadata.set("nick", "guest12345"),
      )
  )
  |. Grpc.Client.Metadata.Generator.make;

let credentials =
  Grpc.Client.Credentials.createSsl(
    Grpc.loadCert(certPath ++ "/root.crt"),
    Grpc.loadCert(certPath ++ "/client-private-key.pem"),
    Grpc.loadCert(certPath ++ "/client-public-certificate.crt"),
  );

let credentials =
  Grpc.Client.Credentials.combine(credentials, metadataGenerator);

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
|. Grpc.Chat.ChatService.Client.Echo64Rpc.invokeFuture(payload)
|. Future.flatMapOk(res => {
     Js.log2("Ok", res);
     chatServiceClient
     |. Grpc.Chat.ChatService.Client.Echo64Rpc.invokeFuture(payload);
   })
|. Future.tapError(err => Js.log2("Error", err));
