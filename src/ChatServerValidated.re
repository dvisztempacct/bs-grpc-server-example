[@bs.val]
external dirname : string = "__dirname";

let certPath = dirname ++ "/../certs/";

let now = () => int_of_float(Js.Date.now() /. 1000.0);

type textMessage = {
  nick: string,
  text: string,
  time: int,
};

type channel = {recentTextMessages: ref(array(textMessage))};

/* keyed by channel name */
let channels = Js.Dict.empty();

let getOrCreateChannel = channelName => {
  let maybeChannel = channels |. Js.Dict.get(channelName);
  switch (maybeChannel) {
  | Some(maybeChannel) => maybeChannel
  | None =>
    let channel = {
      recentTextMessages:
        ref([|
          {
            nick: "nobody",
            text: "out of the silent planet came a channel",
            time: now(),
          },
        |]),
    };
    channels |. Js.Dict.set(channelName, channel);
    channel;
  };
};

let getChannelMaybe = channelName => channels |. Js.Dict.get(channelName);

let addMessageToChannel = (nick, text, time, channel) =>
  channel.recentTextMessages :=
    Belt.Array.concat(channel.recentTextMessages^, [|{nick, text, time}|]);

let sendMessageImpl = (channelName, text, callback) => {
  getOrCreateChannel(channelName)
  |> addMessageToChannel("anonymous", text, now());

  /* Send successful reply */
  Grpc.Chat.SendMessageReply.t() |> Grpc.reply(callback);
};

let channelsThatNeedPasswords = "#announcements";

/* Error handler for ChatService.SendMessage RPC */
let sendMessageErrorHandler = error =>
  Grpc.Chat.SendMessageReply.make(~error, ());
/* Implementation for ChatService.sendMessage RPC */
let sendMessage = (call, request, callback) => {
  let metaData = call |. Grpc.Chat.ChatService.SendMessageRpc.getMeta;
  let nick = metaData |. Js.Dict.get("nick");
  Js.log2("got metadata nick=", nick);
  let channelName =
    request |. Grpc.Chat.SendMessageRequest.channel |. Belt.Option.getExn;
  let text =
    request |. Grpc.Chat.SendMessageRequest.text |. Belt.Option.getExn;
  let urgency = request |. Grpc.Chat.SendMessageRequest.urgency;

  Js.log4("ChatServerValidated.re got SendMessageRequest", channelName, text, urgency);

  /* Some channels require a password I guess */
  if (channelName == channelsThatNeedPasswords) {
    "that channel is password-protected" |> sendMessageErrorHandler |> Grpc.reply(callback)
  } else {
    sendMessageImpl(channelName, text, callback)
  }
};
/* Error handler for ChatService.SendPasswordedMessage RPC */
let sendPasswordedMessageErrorHandler = error =>
  Grpc.Chat.SendMessageReply.make(~error, ());
/* Implementation for ChatService.sendPasswordedMessage RPC */
let sendPasswordedMessage = (call, request, callback) => {
  let metaData = call |. Grpc.Chat.ChatService.SendPasswordedMessageRpc.getMeta;
  let nick = metaData |. Js.Dict.get("nick");
  Js.log2("got metadata nick=", nick);
  let sendMessageRequest =
    request
    |. Grpc.Chat.SendPasswordedMessageRequest.sendMessageRequest
    |. Belt.Option.getExn;
  let channelName =
    sendMessageRequest
    |. Grpc.Chat.SendMessageRequest.channel
    |. Belt.Option.getExn;
  let text =
    sendMessageRequest
    |. Grpc.Chat.SendMessageRequest.text
    |. Belt.Option.getExn;
  let password =
    request |. Grpc.Chat.SendPasswordedMessageRequest.password |. Belt.Option.getExn;

  Js.log3(
    "ChatServerValidated.re got SendPasswordedMessageRequest",
    channelName,
    text,
  );

  /* Check the password */
  if (password == "correct-password") {
    sendMessageImpl(channelName, text, callback)
  } else {
    "password incorrect" |> sendPasswordedMessageErrorHandler |> Grpc.reply(callback)
  }
};

/* ChatService.poll RPC */
let poll = (call, request, callback) => {
  let metaData = call |. Grpc.Chat.ChatService.PollRpc.getMeta;
  let nick = metaData |. Js.Dict.get("nick");
  Js.log2("got metadata nick=", nick);
  let channelNames =
    request |. Grpc.Chat.PollRequest.channels |. Belt.Option.getExn;
  let time = request |. Grpc.Chat.PollRequest.time |. Belt.Option.getExn;

  Js.log3("ChatServerValidated.re got PollRequest", channelNames, time);

  channelNames
  |> Array.map(channelName =>
       channelName
       |. getChannelMaybe
       |. Belt.Option.map(channel =>
            channel.recentTextMessages^
            |. Belt.Array.keep(textMessage => textMessage.time >= time)
            |> Array.map(textMessage =>
                 Grpc.Chat.ChannelMessage.make(
                   ~channel=channelName,
                   ~nick="anon",
                   ~text=textMessage.text,
                   (),
                 )
               )
          )
     )
  |. Belt.Array.keep(Belt.Option.isSome)
  |> Array.map(Belt.Option.getExn)
  |. Belt.Array.concatMany
  |. Grpc.Chat.ChannelMessages.make(~channel_messages=_, ())
  |. (
    cm =>
      Grpc.Chat.PollReply.make(
        ~result=Grpc.Chat.PollReply.Channel_messages(cm),
        (),
      )
  )
  |> Grpc.reply(callback);
  ();
};
/* Error handler */
let pollErrorHandler = error =>
  Grpc.Chat.PollReply.make(~result=Grpc.Chat.PollReply.Error(error), ());

/* 64-bit integers as strings test */
let echo64 = (call, request, callback) => {
  let metaData = call |. Grpc.Chat.ChatService.Echo64Rpc.getMeta;
  let nick = metaData |. Js.Dict.get("nick");
  Js.log2("got metadata nick=", nick);
	Js.log2("got 64 bit values:", request);
	request |> Grpc.reply(callback);
};

let echo64ErrorHandler = x => failwith("unexpected error: " ++ x);

let credentials = Grpc.Server.Credentials.Ssl.make(
  Grpc.loadCert(certPath ++ "/root.crt"),
  [|
  Grpc.ServerKeyAndCert.t(
    ~privateKey=Grpc.loadCert(certPath ++ "/server-private-key.pem"),
    ~certChain=Grpc.loadCert(certPath ++ "/server-public-certificate.crt"),
  )
  |],
  false
);

let chatService =
  Grpc.Chat.ChatService.make(
    ~sendMessage,
    ~sendMessageErrorHandler,
    ~sendPasswordedMessage,
    ~sendPasswordedMessageErrorHandler,
    ~poll,
    ~pollErrorHandler,
		~echo64,
		~echo64ErrorHandler,
  );

let server = Grpc.Server.make("127.0.0.1:12345", ~credentials, ~chatService);
