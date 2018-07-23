const Long = require('long')
const grpc = require('bs-grpc')
const chat = grpc.load('chat.proto').chat
const fs = require('fs')
const loadCert = x => fs.readFileSync(__dirname + '/../certs/'+ x)
const channelCredentials = grpc.credentials.createSsl(
  loadCert('root.crt'),
  loadCert('client-private-key.pem'),
  loadCert('client-public-certificate.crt'),
)
const metadataGenerator = (ignored, callback) => {
  const metadata = new grpc.Metadata
  metadata.set('nick', 'eich')
  callback(null, metadata)
}
const callCredentials = grpc.credentials.createFromMetadataGenerator(metadataGenerator)
const combinedCredentials = grpc.credentials.combineChannelCredentials(channelCredentials, callCredentials)
const chatServiceClient = new chat.ChatService('127.0.0.1:12345', combinedCredentials)
const now = () => Math.floor(Date.now() / 1000)

process.stdin.on('data', chunk => {
  const text = chunk.toString('utf8').trim()
  const msg = {
    channel: '#random',
    text: chunk.toString('utf8').trim(),
    urgency: 2
  }
  const words = text.split(/\s+/g)
  switch (words[0]) {
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
    case '/announce':
      return void announce(words[1], words.slice(2).join(' '))
    case '/announcebroken':
      return void announce(void 0, void 0)
    case '/nopechan':
      msg.channel = '#nope'
      break
    case '/urgentmsg':
      msg.urgency = 1
      break
    case '/64':
      return void a64bitTest()
    case '/pollfail':
      /* channel name is too short */
      return void chatServiceClient.poll({ channels: ['#random', '#announcements', '#x'] }, (err, res) => {
        if (err) console.error('err', err)
        else if (res.error) console.error('app error:', res.error)
        else console.error('poll should have failed but did not')
      })
  }
  chatServiceClient.sendMessage(msg, (err, res) => {
    if (err) console.error('err', err)
    else if (res.error) console.error('app error:', res.error)
  })
})
function passFail() {
  chatServiceClient.passwordReset
}
function announce(password, text) {
  const msg = {
    password,
    sendMessageRequest: {
      channel: '#announcements',
      text,
      urgency: 0
    }
  }
  console.log('sending announcement message', msg, chat.SendPasswordedMessageRequest.verify(msg), chat.SendPasswordedMessageRequest.fromObject(msg))
  chatServiceClient.sendPasswordedMessage(msg, (err, res) => {
    if (err) console.error('grpc error:', err)
    else if (res.error) console.error('app error:', res.error)
    else console.log('announcement acknowledged')
  })
}
function a64bitTest() {
	const msg = {
		fix64: '9223372036854775807',
		sfix64: '-9223372036854775807',
		varint64: '-9223372036854775807',
		svarint64: '-9223372036854775807',
		uvarint64: '9223372036854775807',
	}
  chatServiceClient.echo64(msg, (err, res) => {
    if (err) console.error('64 bit error:', err)
    else console.log('64 bit reply:', res)
  })
}
let lastPollTime = now()
const interval = setInterval(() => {
  const pollRequest = {
    channels: ['#random', '#announcements'],
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
