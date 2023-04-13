"use strict";

class Client {
  constructor() {
    this.onopen = undefined
    this.onerror = undefined
    this.onoffer = undefined
    this.oncandidate = undefined
    this.__ws = undefined
  }

  connect() {
    if (this.__ws) {
      this.__ws.close()
    }

    this.__ws = new WebSocket(`ws://${window.location.host}/ws`)

    this.__ws.onopen = () => {
      if (this.onopen) {
        this.onopen()
      }
    }

    this.__ws.onmessage = message => {
      var msg = JSON.parse(message.data)

      switch (msg.msg) {
        case 'offer':
          this.onoffer(msg)
          break
        case 'candidate':
          this.oncandidate(msg.candidate)
          break
      }
    }

    this.__ws.onerror = (error) => {
      console.log(`websocket error: ${error.message}`)
      if (this.onerror) {
        this.onerror()
      }
    }
  }

  answer(sdp) {
    this.__sendRequest('answer', { sdp: sdp })
  }

  candidate(candidate) {
    this.__sendRequest('candidate', {candidate: candidate.toJSON()})
  }

  __sendRequest(type, args) {
    var request = Object.assign({msg: type}, args)

    this.__ws.send(JSON.stringify(request))
  }
}

export class WebRTCStream {
  constructor(video) {
    this.__videoElement = video
    this.__webrtc = undefined

    this.__signaling = new Client()

    this.__signaling.onopen = () => {
      this.__webrtc = new RTCPeerConnection()

      this.__webrtc.onicecandidate = (c) => {
        if (c.candidate) {
          this.__signaling.candidate(c.candidate)
        }
      }
      this.__webrtc.ontrack = (event) => {
        console.log(event)
        this.__videoElement.srcObject = event.streams[0];
      }
      this.__webrtc.onconnectionstatechange = (event) => {
        switch (this.__webrtc.connectionState) {
          case 'disconnected':
          case 'failed':
            this.__videoElement.srcObject = undefined
            this.__signaling.connect()
            break;
        }
        console.log(`WebRTC state changed: ${this.__webrtc.connectionState}`)
      }
    }

    this.__signaling.onoffer = async (msg) => {
      try {
        console.log(msg.sdp)
        await this.__webrtc.setRemoteDescription({ type: 'offer', sdp: msg.sdp })
        var answer = await this.__webrtc.createAnswer()
        console.log(answer)
        this.__webrtc.setLocalDescription(answer)
        this.__signaling.answer(answer.sdp)
      } catch (err) {
        console.log(err.message)
      }
    }
    this.__signaling.oncandidate = (c) => {
      console.log(c)
      this.__webrtc.addIceCandidate(c)
    }
    this.__signaling.onerror = () => {
      setTimeout(() => this.__signaling.connect(), 1000)
    }

    this.__signaling.connect()
  }
}

export class HLSStream {
  constructor(video) {
    this.__hls = new Hls({liveDurationInfinity: true});

    this.__hls.on(Hls.Events.ERROR, (e, data) => {
      switch (data.type) {
        case 'networkError':
        case 'mediaError':
          setTimeout(() => {
            this.__hls.recoverMediaError()
            this.__hls.loadSource('hls/playlist.m3u8')
          }, 1000)
          break;
      }
      console.log(data)
    })

    this.__hls.attachMedia(video);
    this.__hls.loadSource('hls/playlist.m3u8');
  }
}
