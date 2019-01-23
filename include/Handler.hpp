#ifndef MSC_HANDLER_HPP
#define MSC_HANDLER_HPP

#include "PeerConnection.hpp"
#include "sdp/RemoteSdp.hpp"
#include "json.hpp"
#include <memory>
#include <set>
#include <string>

using json = nlohmann::json;

namespace mediasoupclient
{
class Handler : public PeerConnection::Listener
{
public:
	class Listener
	{
	public:
		virtual void OnConnect(json& transportLocalParameters) = 0;
		virtual void OnConnectionStateChange(
		  webrtc::PeerConnectionInterface::IceConnectionState connectionState) = 0;
	};

	// Methods to be implemented by child classes.
public:
	virtual void RestartIce(const json& remoteIceParameters) = 0;

public:
	static json GetNativeRtpCapabilities();
	static const std::string& GetName();

public:
	explicit Handler(
	  Listener* listener,
	  const json& iceServers                = json::array(),
	  const std::string& iceTransportPolicy = "all",
	  const json& proprietaryConstraints    = json::object(),
	  json sendingRtpParametersByKind       = json::array());

	json GetTransportStats();
	void UpdateIceServers(const json& iceServerUris);

	void Close();

	/* Methods inherited from PeerConnectionListener. */
public:
	void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState) override;

protected:
	void SetupTransport(const std::string& localDtlsRole);

protected:
	// Listener instance.
	Listener* listener{ nullptr };

	// Generic sending RTP parameters for audio and video.
	json sendingRtpParametersByKind;

	// Remote SDP instance.
	std::unique_ptr<Sdp::RemoteSdp> remoteSdp;

	// Got transport local and remote parameters.
	bool transportReady{ false };

	// PeerConnection instance.
	std::unique_ptr<PeerConnection> pc;
};

class SendHandler : public Handler
{
public:
	SendHandler(
	  Handler::Listener* listener,
	  const json& transportRemoteParameters,
	  const json& iceServers,
	  const std::string& iceTransportPolicy,
	  const json& proprietaryConstraints,
	  const json& rtpParametersByKind);

	json Send(webrtc::MediaStreamTrackInterface* track, const json& simulcast);
	void StopSending(webrtc::MediaStreamTrackInterface* track);
	void ReplaceTrack(
	  webrtc::MediaStreamTrackInterface* track, webrtc::MediaStreamTrackInterface* newTrack);
	void SetMaxSpatialLayer(webrtc::MediaStreamTrackInterface* track, uint8_t spatialLayer);
	json GetSenderStats(webrtc::MediaStreamTrackInterface* track);

	/* Methods inherided from Handler. */
public:
	void RestartIce(const json& remoteIceParameters) override;

private:
	// Sending tracks.
	std::set<webrtc::MediaStreamTrackInterface*> tracks;
};

class RecvHandler : public Handler
{
public:
	RecvHandler(
	  Handler::Listener* listener,
	  const json& transportRemoteParameters,
	  const json& iceServers,
	  const std::string& iceTransportPolicy,
	  const json& proprietaryConstraints);

	webrtc::MediaStreamTrackInterface* Receive(
	  const std::string& id, const std::string& kind, const json& rtpParameters);
	void StopReceiving(const std::string& id);
	json GetReceiverStats(const std::string& id);

	/* Methods inherided from Handler. */
public:
	void RestartIce(const json& remoteIceParameters) override;

	/*
	 * Receiver infos indexed by id.
	 *
	 * Receiver info:
	 * - mid {String}
	 * - kind {String}
	 * - closed {Boolean}
	 * - trackId {String}
	 * - ssrc {Number}
	 * - rtxSsrc {Number}
	 * - cname {String}
	 */
	std::map<const std::string, json> receiverInfos;
};

/* Inline methods */

inline json Handler::GetTransportStats()
{
	return this->pc->GetStats();
}

inline void Handler::Close()
{
	this->pc->Close();
};
} // namespace mediasoupclient

#endif
