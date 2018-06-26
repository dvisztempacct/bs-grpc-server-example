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

/* ChatService.sendMessage RPC */
let sendMessage = (call, callback) => {
  let request = call |. Grpc.Chat.ChatService.SendMessageRpc.request;
  let channelName =
    request |. Grpc.Chat.SendMessageRequest.channel |. Belt.Option.getExn;
  let text =
    request |. Grpc.Chat.SendMessageRequest.text |. Belt.Option.getExn;

  Js.log3("ChatServerValidated.re got SendMessageRequest", channelName, text);

  sendMessageImpl(channelName, text, callback);
};
/* ChatService.sendPasswordedMessage RPC */
let sendPasswordedMessage = (call, callback) => {
  let request = call |. Grpc.Chat.ChatService.SendPasswordedMessageRpc.request;
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

  Js.log3(
    "ChatServerValidated.re got SendPasswordedMessageRequest",
    channelName,
    text,
  );

  sendMessageImpl(channelName, text, callback);
};

/* ChatService.poll RPC */
let poll = (call, callback) => {
  let request = call |. Grpc.Chat.ChatService.PollRpc.request;
  let channelNames =
    request |. Grpc.Chat.PollRequest.channels |. Belt.Option.getExn;
  let time = request |. Grpc.Chat.PollRequest.time |. Belt.Option.getExn;

  Js.log3("got PollRequest", channelNames, time);

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

let credentials = Grpc.Server.Credentials.Insecure.make();

let chatService =
  Grpc.Chat.ChatService.t(~sendMessage, ~sendPasswordedMessage, ~poll);

let server = Grpc.Server.make("127.0.0.1:12345", ~credentials, ~chatService);
