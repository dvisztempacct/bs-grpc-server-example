const grpc = require('grpc')
const chat = grpc.load('chat.proto').chat
const chatServiceClient = new chat.ChatService('127.0.0.1:12345', grpc.credentials.createInsecure())
const now = () => Math.floor(Date.now() / 1000)

process.stdin.on('data', chunk => {
  chatServiceClient.sendMessage({
    channel: '#random',
    text: chunk.toString('utf8').trim()
  }, (err, res) => {
    if (err) console.error('err', err)
  })
})
let lastPollTime = now()
const interval = setInterval(() => {
  chatServiceClient.poll({
    channels: ['#random'],
    time: lastPollTime
  }, (err, res) => {
    if (err) console.error('err', err)
    else {
      res.channel_messages.channel_messages.forEach(channel_message => {
        console.log(
          channel_message.channel,
          channel_message.nick,
          channel_message.text
        )
      })
    }
  })
  lastPollTime = now()
}, 1000)
process.stdin.on('end', () => clearInterval(interval))
