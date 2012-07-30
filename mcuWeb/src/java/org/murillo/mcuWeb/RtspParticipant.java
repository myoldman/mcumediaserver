/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package org.murillo.mcuWeb;

import br.com.voicetechnology.rtspclient.RTSPClient;
import br.com.voicetechnology.rtspclient.concepts.Client;
import br.com.voicetechnology.rtspclient.concepts.ClientListener;
import br.com.voicetechnology.rtspclient.concepts.Request;
import br.com.voicetechnology.rtspclient.concepts.Response;
import br.com.voicetechnology.rtspclient.transport.PlainTCP;
import com.ffcs.mcu.RtspEndPointManager;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.apache.xmlrpc.XmlRpcException;
import org.murillo.MediaServer.Codecs;

enum MediaType {

    AUDIO,
    VIDEO,
    TEXT
}

class Track {

    public String controlURI;
    public MediaType type;
}

/**
 *
 * @author liuhong
 */
public class RtspParticipant extends RTPParticipant implements ClientListener  {
    private String controlURI;
    private String rtspId;
    private final List<Track> resourceList;
    private String aggregateControlResource;
    private RTSPClient rtspClient;
    private Timer timer = new Timer(true);

    RtspParticipant(Integer id,String name,Integer mosaicId,Conference conf) throws XmlRpcException {
        //Call parent
        super(id,name,mosaicId,conf);
        this.type = Participant.Type.RTSP;
        rtspClient = new RTSPClient();
        controlURI = "";
        aggregateControlResource = "";
        resourceList = Collections.synchronizedList(new LinkedList<Track>());
    }

    void doConnect(String dest, String name, String rtspId) throws IOException, URISyntaxException, XmlRpcException {
        this.name = name;
        this.controlURI = dest;
        this.rtspId = rtspId;
        URI requestUri = new URI(dest);
        rtspClient.setTransport(new PlainTCP());
        rtspClient.setClientListener(this);
        rtspClient.describe(requestUri);
        this.sendIp = requestUri.getHost();
        setState(State.CONNECTING);
    }

    public void generalError(Client client, Throwable thrwbl) {
        Logger.getLogger(RtspParticipant.class.getName()).log(Level.SEVERE, "generalError \n", thrwbl);
    }

    public void mediaDescriptor(Client client, String descriptor) {
        super.processSDP(descriptor);
        // searches for control: session and media arguments.
        final String target = "control:";
        System.out.println("Session Descriptor" + descriptor);
        int position = -1;
        String[] sdps = descriptor.split("\r\n");
        boolean foundMediaAttribute = false;
        if (sdps != null) {
            int sdpLines = sdps.length;
            Track currentTrack = null;
            for (int i = 0; i < sdpLines; i++) {
                if (sdps[i].indexOf("m=") > -1) {
                    foundMediaAttribute = true;
                }

                if (sdps[i].indexOf("m=audio") > -1) {
                    currentTrack = new Track();
                    currentTrack.type = MediaType.AUDIO;
                    resourceList.add(currentTrack);
                }
                if (sdps[i].indexOf("m=video") > -1) {
                    currentTrack = new Track();
                    currentTrack.type = MediaType.VIDEO;
                    resourceList.add(currentTrack);
                }

                position = sdps[i].indexOf(target);
                if (position > -1 && !foundMediaAttribute) {
                    aggregateControlResource = sdps[i].substring(position + target.length());
                } else if (position > -1 && foundMediaAttribute) {
                    currentTrack.controlURI = sdps[i].substring(position + target.length());
                }
            }
        }
        this.sendIp = client.getURI().getHost();
    }

    public void requestFailed(Client client, Request rqst, Throwable thrwbl) {
        Logger.getLogger(RtspParticipant.class.getName()).log(Level.SEVERE, "Request failed \n{0}", rqst);
    }

    public void response(Client client, Request request, Response response) {
        try {
            //System.out.println("Got response: \n" + response);
            //System.out.println("for the request: \n" + request);
            if (response.getStatusCode() == 200) {
                switch (request.getMethod()) {
                    case DESCRIBE:
                        this.startReceiving();
                        int firstTrackPort = getRecAudioPort();
                        if (resourceList.size() > 0) {
                            Track track = resourceList.remove(0);
                            if (track.type == MediaType.VIDEO) {
                                firstTrackPort = super.getRecVideoPort();
                            }
                            if (track.controlURI.startsWith("rtsp://")) {
                                client.setup(new URI(track.controlURI), firstTrackPort);
                            } else {
                                client.setup(new URI(controlURI), firstTrackPort, track.controlURI);
                            }
                        } else {
                            client.setup(new URI(controlURI), firstTrackPort);
                        }
                        break;

                    case SETUP:
                        //sets up next session or ends everything.
                        int secondTrackPort = super.getRecAudioPort();
                        if (resourceList.size() > 0) {
                            Track track = resourceList.remove(0);
                            if (track.type == MediaType.VIDEO) {
                                secondTrackPort = super.getRecVideoPort();
                            }
                            if (track.controlURI.startsWith("rtsp://")) {
                                client.setup(new URI(track.controlURI), secondTrackPort);
                            } else {
                                client.setup(new URI(controlURI), secondTrackPort, track.controlURI);
                            }
                        } else {
                            client.play();
                        }
                        break;
                    case PLAY:
                        //Set state before joining
                        setState(Participant.State.CONNECTED);
                        RtspEndPointManager.INSTANCE.setRtspEndPointState(rtspId, com.ffcs.mcu.pojo.RtspEndPoint.State.CONNECTED);
                        //Join it to the conference
                        conf.joinParticipant(this);
                        TimerTask task = new TimerTask() {

                            public void run() {
                                try {
                                    rtspClient.options(controlURI, rtspClient.getURI());
                                } catch (URISyntaxException ex) {
                                    Logger.getLogger(RtspParticipant.class.getName()).log(Level.SEVERE, null, ex);
                                } catch (IOException ex) {
                                    Logger.getLogger(RtspParticipant.class.getName()).log(Level.SEVERE, null, ex);
                                }
                            }
                        };
                        timer.schedule(task, 0, 10000);
                        break;
                }
            } else {
                client.teardown();
                setState(Participant.State.ERROR);
                RtspEndPointManager.INSTANCE.setRtspEndPointState(rtspId, com.ffcs.mcu.pojo.RtspEndPoint.State.ERROR);
            }
        } catch (Throwable t) {
            generalError(client, t);
        }
    }

    protected void sessionSet(Client client) throws IOException {
        client.teardown();
    }
     
    @Override
    public void end() {
        switch (state) {
            case CONNECTING:
            case CONNECTED:
                rtspClient.teardown();
                setState(State.DISCONNECTED);
                RtspEndPointManager.INSTANCE.setRtspEndPointState(rtspId, com.ffcs.mcu.pojo.RtspEndPoint.State.IDLE);
                timer.cancel();
                destroy();
                break;
        }
    }
}
