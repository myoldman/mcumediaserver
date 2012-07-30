/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.murillo.mscontrol.networkconnection;

import java.net.URI;
import java.util.Iterator;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.media.mscontrol.MediaConfig;
import javax.media.mscontrol.MediaObject;
import javax.media.mscontrol.MsControlException;
import javax.media.mscontrol.Parameters;
import javax.media.mscontrol.join.JoinableStream;
import javax.media.mscontrol.join.JoinableStream.StreamType;
import javax.media.mscontrol.networkconnection.NetworkConnection;
import javax.media.mscontrol.networkconnection.SdpPortManager;
import javax.media.mscontrol.resource.Action;
import org.apache.xmlrpc.XmlRpcException;
import org.murillo.MediaServer.Codecs.MediaType;
import org.murillo.mscontrol.resource.ContainerImpl;
import org.murillo.mscontrol.MediaServer;
import org.murillo.mscontrol.MediaSessionImpl;
import org.murillo.mscontrol.ParametersImpl;

/**
 *
 * @author Sergio
 */
public class NetworkConnectionImpl extends ContainerImpl implements NetworkConnection  {
    //Configuration pattern related to NetworkConnection.BASE
    public final static MediaConfig BASE_CONFIG = NetworkConnectionBasicConfigImpl.getConfig();
    
    private final int endpointId;
    private final SdpPortManagerImpl sdpPortManager;
    private final MediaServer mediaServer;

    public NetworkConnectionImpl(MediaSessionImpl sess, URI uri,Parameters params) throws MsControlException {
        //Call parent
        super(sess,uri,params);
        //The port manager
        sdpPortManager = new SdpPortManagerImpl(this,params);
        //Add streams
        AddStream(StreamType.audio, new NetworkConnectionJoinableStream(sess,this,StreamType.audio));
        AddStream(StreamType.video, new NetworkConnectionJoinableStream(sess,this,StreamType.video));
        AddStream(StreamType.message, new NetworkConnectionJoinableStream(sess,this,StreamType.message));
        //Get media server
        mediaServer = session.getMediaServer();
        try {
            //Create endpoint
            endpointId = mediaServer.EndpointCreate(session.getSessionId(), uri.toString(), true, true, true);
        } catch (XmlRpcException ex) {
            Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
            //Trhow it
            throw new MsControlException("Could not create network connection",ex);
        }
    }

    public MediaServer getMediaServer() {
        return mediaServer;
    }

    @Override
    public SdpPortManager getSdpPortManager() throws MsControlException {
        return sdpPortManager;
    }

    @Override
    public JoinableStream getJoinableStream(StreamType type) throws MsControlException {
        //Return array
        return (JoinableStream) streams.get(type);
    }

    @Override
    public JoinableStream[] getJoinableStreams() throws MsControlException {
        //Return object array
        return (JoinableStream[]) streams.values().toArray(new JoinableStream[3]);
    }

    @Override
    public Parameters createParameters() {
        //Create new map
        return new ParametersImpl();
    }

    public int getEndpointId() {
        return endpointId;
    }

    public int getSessId() {
        return session.getSessionId();
    }

    void startReceiving(SdpPortManagerImpl sdp) throws MsControlException
    {
        //Get media server
        MediaServer mediaServer = session.getMediaServer();
        //If supported
        if (sdp.getAudioSupported())
        {
            try {
                //Create rtp map for audio
                sdp.createRTPMap("audio");
                //Get receiving ports
                Integer recAudioPort = mediaServer.EndpointStartReceiving(session.getSessionId(), endpointId, MediaType.AUDIO, sdp.getRtpInMediaMap("audio"));
                //Set ports
                sdp.setRecAudioPort(recAudioPort);
            } catch (XmlRpcException ex) {
                Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
                 //Trhow it
                throw new MsControlException("Could not start receiving audio",ex);
            }
        }

        //If supported
        if (sdp.getVideoSupported())
        {
            try {
                //Create rtp map for video
                sdp.createRTPMap("video");
                //Get receiving ports
                Integer recVideoPort = mediaServer.EndpointStartReceiving(session.getSessionId(), endpointId, MediaType.VIDEO, sdp.getRtpInMediaMap("video"));
                //Set ports
                sdp.setRecVideoPort(recVideoPort);
            } catch (XmlRpcException ex) {
                Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
                //Trhow it
                throw new MsControlException("Could not start receiving video",ex);
            }
        }

        //If supported
        if (sdp.getTextSupported())
        {
            try {
                //Create rtp map for text
                sdp.createRTPMap("text");
                //Get receiving ports
                Integer recTextPort = mediaServer.EndpointStartReceiving(session.getSessionId(), endpointId, MediaType.TEXT, sdp.getRtpInMediaMap("text"));
                //Set ports
                sdp.setRecTextPort(recTextPort);
            } catch (XmlRpcException ex) {
                Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
                //Trhow it
                throw new MsControlException("Could not start receiving text",ex);
            }
        }

        //And set the sender ip
        sdp.setRecIp(mediaServer.getIp());
    }

    protected void startSending(SdpPortManagerImpl sdp) throws MsControlException
    {
        //Get media server
        MediaServer mediaServer = session.getMediaServer();
        //Check audio
        if (sdp.getSendAudioPort()!=0)
            //Send
            try {
                //Get the auido stream
                NetworkConnectionJoinableStream stream = (NetworkConnectionJoinableStream)getJoinableStream(StreamType.audio);
                //Update ssetAudioCodecneding codec
                stream.requestAudioCodec(sdp.getAudioCodec());
                //Send
                mediaServer.EndpointStartSending(session.getSessionId(), endpointId, MediaType.AUDIO, sdp.getSendAudioIp(), sdp.getSendAudioPort(), sdp.getRtpOutMediaMap("audio"));
            } catch (XmlRpcException ex) {
                Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
                //Trhow it
                throw new MsControlException("Could not start sending audio",ex);
            }

        //Check video
        if (sdp.getSendVideoPort()!=0)
            //Send
            try {
                //Get the auido stream
                NetworkConnectionJoinableStream stream = (NetworkConnectionJoinableStream)getJoinableStream(StreamType.video);
                //Update sneding codec
                stream.requestVideoCodec(sdp.getVideoCodec());
                //Send
                mediaServer.EndpointStartSending(session.getSessionId(), endpointId, MediaType.VIDEO, sdp.getSendVideoIp(), sdp.getSendVideoPort(), sdp.getRtpOutMediaMap("video"));
            } catch (XmlRpcException ex) {
                Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
                //Trhow it
                throw new MsControlException("Could not start sending video",ex);
            }


        //Check text
        if (sdp.getSendTextPort()!=0)
            //Send
            try {
                //Send
                mediaServer.EndpointStartSending(session.getSessionId(), endpointId, MediaType.TEXT, sdp.getSendTextIp(), sdp.getSendTextPort(), sdp.getRtpOutMediaMap("text"));
            } catch (XmlRpcException ex) {
                Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
                //Trhow it
                throw new MsControlException("Could not start sending text",ex);
            }
    }

    @Override
    public void triggerAction(Action action) {
        //Check if it is a picture_fast_update
        if (action.toString().equalsIgnoreCase("org.murillo.mscontrol.picture_fast_update"))
        {
            try {
                //Send FPU
                mediaServer.EndpointRequestUpdate(session.getSessionId(), endpointId, MediaType.VIDEO);
            } catch (XmlRpcException ex) {
                Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }

    @Override
    public <R> R getResource(Class<R> type) throws MsControlException {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public MediaConfig getConfig() {
        return BASE_CONFIG;
    }

    @Override
    public void release() {
        //Free joins
        releaseJoins();
        //Get media server
        MediaServer mediaServer = session.getMediaServer();
        try {
            //Delete endpoint
            mediaServer.EndpointDelete(session.getSessionId(),endpointId);
        } catch (XmlRpcException ex) {
            Logger.getLogger(NetworkConnectionImpl.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    @Override
    public Iterator<MediaObject> getMediaObjects() {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public <T extends MediaObject> Iterator<T> getMediaObjects(Class<T> type) {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}
