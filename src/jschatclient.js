const grpc = require('grpc')
const chat = grpc.load('chat.proto').chat
const chatServiceClient = new chat.ChatService('127.0.0.1:12345', grpc.credentials.createInsecure())
const now = () => Math.floor(Date.now() / 1000)

process.stdin.on('data', chunk => {
  const text = chunk.toString('utf8').trim()
  const msg = {
    channel: '#random',
    text: chunk.toString('utf8').trim()
  }
  switch (text) {
    case '/nochan':
      delete msg.channel
      break
    case '/notext':
      delete msg.text
      break
    case '/nothing':
      delete msg.channel
      delete msg.text
      break
    case '/passfail':
      return void passFail()
    case '/nopechan':
      msg.channel = '#nope'
      break
  }
  chatServiceClient.sendMessage(msg, (err, res) => {
    console.log("sendMessage sez", err, res)
    if (err) console.error('err', err)
  })
})
function passFail() {
  chatServiceClient.passwordReset
}
let lastPollTime = now()
const interval = setInterval(() => {
  const pollRequest = {
    channels: ['#random'],
    time: lastPollTime
  }
  chatServiceClient.poll(pollRequest, (err, res) => {
    if (err) console.error('err', err)
    else {
      if (res.result == 'error') {
        console.error('app err:', res.error)
      } else {
        if (res.channel_messages.channel_messages != null) {
          res.channel_messages.channel_messages.forEach(channel_message => {
            console.log(
              channel_message.channel,
              channel_message.nick,
              channel_message.text
            )
          })
        }
      }
    }
  })
  lastPollTime = now()
}, 1000)
process.stdin.on('end', () => clearInterval(interval))
