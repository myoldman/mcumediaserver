/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ffcs.mcu;

import java.io.IOException;
import java.io.InputStream;
import java.util.Map.Entry;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.mail.BodyPart;
import javax.mail.MessagingException;
import javax.mail.Multipart;
import javax.servlet.sip.*;
import org.apache.xmlrpc.XmlRpcException;
import org.murillo.MediaServer.Codecs;
import org.murillo.MediaServer.XmlRpcMcuClient;
import org.murillo.mcuWeb.Conference;
import org.murillo.mcuWeb.Participant;
import org.murillo.mcuWeb.Profile;

/**
 *
 * @author liuhong
 */
public class Spyer {

    public interface Listener {

        public void onStateChanged(Spyer part, Spyer.State state);
    };

    public enum State {

        CREATED, CONNECTING, WAITING_ACCEPT, CONNECTED, ERROR, TIMEOUT, BUSY, DECLINED, NOTFOUND, DISCONNECTED, DESTROYED
    }
    private Address address;
    private String name;
    private SipSession session = null;
    private SipApplicationSession appSession = null;
    private SipServletRequest inviteRequest = null;
    protected boolean isDestroying;
    private String videoContentType;
    private Integer sendVideoPort;
    private String sendVideoIp;
    private Integer videoCodec;
    private Integer id;
    private final String h264profileLevelIdDefault = "428014";
    private Integer spyer_number;
    protected XmlRpcMcuClient client;
    protected HashSet<Listener> listeners = null;
    protected Conference conf = null;
    protected Participant spyee;
    protected State state;
    protected String sendIp;
    private String location;
    private HashMap<String, HashMap<Integer, Integer>> rtpInMediaMap = null;
    private HashMap<String, HashMap<Integer, Integer>> rtpOutMediaMap = null;
    private String h264profileLevelId;
    private boolean videoSupported;
    private int videoBitrate;
    private String rejectedMedias;
    private int h264packetization;
    private HashMap<String, List<Integer>> supportedCodecs;
    private Integer recVideoPort;
    private String recIp;
    protected Profile profile;
    private boolean isSendingVideo;

    public HashMap<Integer, Integer> getRtpInMediaMap(String media) {
        //Return rtp mapping for media
        return rtpInMediaMap.get(media);
    }

    HashMap<Integer, Integer> getRtpOutMediaMap(String media) {
        //Return rtp mapping for media
        return rtpOutMediaMap.get(media);
    }

    public String getRecIp() {
        return recIp;
    }

    public void setRecIp(String recIp) {
        this.recIp = recIp;
    }

    public Boolean getVideoSupported() {
        return videoSupported;
    }

    /**
     * @return the spyer_number
     */
    public Integer getSpyer_number() {
        return spyer_number;
    }

    /**
     * @param spyer_number the spyer_number to set
     */
    public void setSpyer_number(Integer spyer_number) {
        this.spyer_number = spyer_number;
    }

    /**
     * @param sendVideoPort the sendVideoPort to set
     */
    public void setSendVideoPort(Integer sendVideoPort) {
        this.sendVideoPort = sendVideoPort;
    }

    /**
     * @param sendVideoIp the sendVideoIp to set
     */
    public void setSendVideoIp(String sendVideoIp) {
        this.sendVideoIp = sendVideoIp;
    }

    /**
     * @param videoCodec the videoCodec to set
     */
    public void setVideoCodec(Integer videoCodec) {
        this.videoCodec = videoCodec;
    }

    public Integer getSendVideoPort() {
        return sendVideoPort;
    }

    public Spyer(Integer spyerId, Integer spyer_number, Conference conf, Participant spyee) {
        this.id = spyerId;
        this.spyer_number = spyer_number;
        this.conf = conf;
        this.spyee = spyee;
        this.profile = conf.getProfile();
        //Create listeners
        listeners = new HashSet<Listener>();
        rejectedMedias = "";
        h264packetization = 0;
        rtpInMediaMap = new HashMap<String, HashMap<Integer, Integer>>();
        rtpOutMediaMap = new HashMap<String, HashMap<Integer, Integer>>();
        //Initial state
        state = State.CREATED;
        isSendingVideo = false;
        supportedCodecs = new HashMap<String, List<Integer>>();
        //Create the client
        this.client = conf.getMixer().createMcuClient();

    }

    private void setState(State state) {
        //Change it
        this.state = state;

        //Call listeners
        for (Listener listener : listeners) //Call it
        {
            listener.onStateChanged(this, state);
        }
    }

    public void setListener(Listener listener) {
        listeners.add(listener);
    }

    public String getUsernameDomain() {
        //Get sip uris
        SipURI uri = (SipURI) address.getURI();
        //Return username
        return uri.getUser() + "@" + uri.getHost();
    }

    public void setSendIp(String sendIp) {
        this.sendIp = sendIp;
    }

    public Integer getVideoCodec() {
        return videoCodec;
    }

    public String getSendVideoIp() {
        return sendVideoIp;
    }

    public void onByeRequest(SipServletRequest request) {
        try {
            //Create final response
            SipServletResponse resp = request.createResponse(200, "Ok");
            //Send it
            resp.send();
        } catch (IOException ex) {
            Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Set expire time
        appSession.setExpires(1);
        //Disconnect
        setState(State.DISCONNECTED);
        //Terminate
        destroy();
    }

    public void onAckRequest(SipServletRequest request) throws IOException {
        //Check if it has content
        if (request.getContentLength() > 0) //Process it
        {
            proccesContent(request.getContentType(), request.getContent());
        }
        try {
            //Set state before joining
            setState(State.CONNECTED);
            //Start sending
            startSending();

        } catch (XmlRpcException ex) {
            Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void onInviteRequest(SipServletRequest request) throws IOException {
        //Store address
        address = request.getFrom();
        //Get name
        name = getUsernameDomain();
        //Create ringing
        SipServletResponse resp = request.createResponse(180, "Ringing");
        //Send it
        resp.send();
        //Set state
        setState(State.WAITING_ACCEPT);
        //Check cdr
        //Get sip session
        session = request.getSession();
        //Get sip application session
        appSession = session.getApplicationSession();
        //Set expire time to wait for ACK
        appSession.setExpires(1);
        //Set reference in sessions
        appSession.setAttribute("user", this);
        session.setAttribute("user", this);
        //Do not invalidate
        appSession.setInvalidateWhenReady(false);
        session.setInvalidateWhenReady(false);
        //Check if it has content
        if (request.getContentLength() > 0) //Process it
        {
            proccesContent(request.getContentType(), request.getContent());
        }
        //Store invite request
        inviteRequest = request;
        //Check if we need to autoaccapt
        isDestroying = false;
        accept();
    }

    private void proccesContent(String type, Object content) throws IOException {
        //No SDP
        String sdp = null;
        //Depending on the type
        if (type.equalsIgnoreCase("application/sdp")) {
            //Get it
            sdp = new String((byte[]) content);
        } else if (type.startsWith("multipart/mixed")) {
            try {
                //Get multopart
                Multipart multipart = (Multipart) content;
                //For each content
                for (int i = 0; i < multipart.getCount(); i++) {
                    //Get content type
                    BodyPart bodyPart = multipart.getBodyPart(i);
                    //Get body type
                    String bodyType = bodyPart.getContentType();
                    //Check type
                    if (bodyType.equals("application/sdp")) {
                        //Get input stream
                        InputStream inputStream = bodyPart.getInputStream();
                        //Create array
                        byte[] arr = new byte[inputStream.available()];
                        //Read them
                        inputStream.read(arr, 0, inputStream.available());
                        //Set length
                        sdp = new String(arr);
                    } else if (bodyType.equals("application/pidf+xml")) {
                        //Get input stream
                        InputStream inputStream = bodyPart.getInputStream();
                        //Create array
                        byte[] arr = new byte[inputStream.available()];
                        //Read them
                        inputStream.read(arr, 0, inputStream.available());
                        //Set length
                        location = new String(arr);
                    }
                }
            } catch (MessagingException ex) {
                Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
        //Parse sdp
        processSDP(sdp);
    }

    public void processSDP(String content) {
        //Search for the ip
        int i = content.indexOf("\r\nc=IN IP4 ");
        int j = content.indexOf("\r\n", i + 1);
        //Get the ip
        String ip = content.substring(i + 11, j);
        setSendIp(ip);
        //Check if ip should be nat for this media mixer
        if (conf.getMixer().isNated(ip)) //Do natting
        {
            ip = "0.0.0.0";
        }

        //Disable supported media
        videoSupported = false;

        //Search the next media
        //int m = content.indexOf("\r\nm=", j);
        int m = content.indexOf("\r\nm=");

        //Find bandwith
        int b = content.indexOf("\r\nb=");

        //If it is before the first media
        if (b > 0 && b < m) {
            //Get :
            i = content.indexOf(":", b);
            //Get end of line
            j = content.indexOf("\r\n", i);
            //Get bandwith modifier
            String bandwith = content.substring(b + 4, i);
            //Get bitrate value
            videoBitrate = Integer.parseInt(content.substring(i + 1, j));
            // Let some room for audio.
            if (videoBitrate >= 128) //Remove maximum rate
            {
                videoBitrate -= 64;
            }
        } else {
            //NO bitrate by default
            videoBitrate = 0;
        }

        //Search medias
        while (m > 0) {
            //No default bitrate
            int mediaBitrate = 0;

            //Search the first blanck
            i = m + 4;
            j = content.indexOf(" ", i);
            //Get media type
            String media = content.substring(i, j);
            //Store ini of media
            int ini = m;
            //Search the next media
            m = content.indexOf("\r\nm=", j);

            //Search port
            i = j + 1;
            j = content.indexOf(" ", i);
            //Get port
            String port = content.substring(i, j);

            //Search transport
            i = j + 1;
            j = content.indexOf(" ", i);
            //Get transport
            String transport = content.substring(i, j);

            //Search format list
            i = j + 1;
            j = content.indexOf("\r\n", i);
            //Get it
            String fmtString = content.substring(i, j);

            //Get fmts
            StringTokenizer fmts = new StringTokenizer(fmtString);

            //Find bandwith modifier for media
            b = content.indexOf("\r\nb=", ini);

            //If it is in this media
            if (b > 0 && b < m) {
                //Get :
                i = content.indexOf(":", b);
                //Get end of line
                j = content.indexOf("\r\n", i);
                //Get bandwith modifier
                String bandwith = content.substring(b + 4, i);
                //Get bitrate value
                mediaBitrate = Integer.parseInt(content.substring(i + 1, j));
            }
            int inactive = content.indexOf("\r\na=inactive", ini);
            if (inactive > 0 && (inactive < m || m < 0)) {
                port = "0";
            }
            int sendonly = content.indexOf("\r\na=sendonly", ini);
            if (sendonly > 0 && (sendonly < m || m < 0)) {
                port = "0";
            }
            //Add support for the media
            if (media.equals("video")) {
                //Check for content attribute inside the media
                i = content.indexOf("\r\na=content:", j);
                //Check if we found it inside this media
                if (i > 0 && (i < m || m < 0)) {
                    //Get end
                    int k = content.indexOf("\r\n", i + 1);
                    //Get it
                    String mediaContentType = content.substring(i + 12, k);
                    //Check if it is not main
                    if (!mediaContentType.equalsIgnoreCase("main")) {
                        //Add video content to the rejected media list
                        rejectedMedias += "m=" + media + " 0 " + transport + " " + fmtString + "\r\na=content:" + mediaContentType + "\r\n";
                        //Skip it
                        continue;
                    } else {
                        //Add the content type to the line
                        videoContentType = mediaContentType;
                    }
                }
                //Store bitrate
                videoBitrate = mediaBitrate;
                //Set as supported
                videoSupported = true;
            } else {
                //Add it to the rejected media lines
                rejectedMedias += "m=" + media + " 0 " + transport + " " + fmtString + "\r\n";
                //Not supported media type
                continue;
            }

            //Check if we have input map for that
            if (!rtpOutMediaMap.containsKey(media)) //Create new map
            {
                rtpOutMediaMap.put(media, new HashMap<Integer, Integer>());
            }

            //Get all codecs
            while (fmts.hasMoreTokens()) {
                try {
                    //Get codec
                    Integer codec = Integer.parseInt(fmts.nextToken());
                    //If it is static
                    if (codec < 96) //Put it in the map
                    {
                        rtpOutMediaMap.get(media).put(codec, codec);
                    }
                } catch (Exception e) {
                    //Ignore non integer codecs, like '*' on application
                }
            }

            //No codec priority yet
            Integer priority = Integer.MAX_VALUE;

            //By default the media IP is the general IO
            String mediaIp = ip;

            //Search for the ip inside the media
            i = content.indexOf("\r\nc=IN IP4 ", j);

            //Check if we found it inside this media
            if (i > 0 && (i < m || m < 0)) {
                //Get end
                int k = content.indexOf("\r\n", i + 1);
                //Get it
                mediaIp = content.substring(i + 11, k);
                //Check if ip should be nat for this media mixer
                if (conf.getMixer().isNated(mediaIp)) //Do natting
                {
                    mediaIp = "0.0.0.0";
                }
            }

            //Search the first format in this media
            int f = content.indexOf("a=rtpmap:", j);

            //FIX
            Integer h264type = 0;
            String maxh264profile = "";
            //Check there is one format and it's from this media
            while (f > 0 && (m < 0 || f < m)) {
                //Get the format
                i = content.indexOf(" ", f + 10);
                j = content.indexOf("/", i);
                //Get the codec type
                Integer type = Integer.parseInt(content.substring(f + 9, i));
                //Get the media type
                String codecName = content.substring(i + 1, j);
                //Get codec for name
                Integer codec = Codecs.getCodecForName(media, codecName);
                //if it is h264 TODO: FIIIIX!!!
                if (codec == Codecs.H264) {
                    //start of fmtp line
                    String start = "a=fmtp:" + type;
                    //Check if we have format for it
                    int k = content.indexOf(start, ini);
                    //If we have it
                    if (k != -1) {
                        //Get ftmp line
                        String ftmpLine = content.substring(k, content.indexOf("\r\n", k));
                        //Get profile id
                        k = ftmpLine.indexOf("profile-level-id=");
                        //Check it
                        if (k != -1) {
                            //Get profile
                            String profile = ftmpLine.substring(k + 17, k + 23);
                            //Convert and compare
                            if (maxh264profile.isEmpty() || Integer.parseInt(profile, 16) > Integer.parseInt(maxh264profile, 16)) {
                                //Store this type provisionally
                                h264type = type;
                                //store new profile value
                                maxh264profile = profile;
                                //Check if it has packetization parameter
                                if (ftmpLine.indexOf("packetization-mode=1") != -1) //Set it
                                {
                                    h264packetization = 1;
                                }
                            }
                        }
                    } else {
                        //check if no profile has been received so far
                        if (maxh264profile.isEmpty()) //Store this type provisionally
                        {
                            h264type = type;
                        }
                    }
                } else if (codec != -1) {
                    //Set codec mapping
                    rtpOutMediaMap.get(media).put(type, codec);
                }
                //Get next
                f = content.indexOf("a=rtpmap:", j);
            }

            //Check if we have type for h264
            if (h264type > 0) {
                //Store profile level
                h264profileLevelId = maxh264profile;
                //add it
                rtpOutMediaMap.get(media).put(h264type, Codecs.H264);
            }

            //For each entry
            for (Integer codec : rtpOutMediaMap.get(media).values()) {
                //Check the media type
                if (media.equals("video")) {
                    //Get suppoorted codec for media
                    List<Integer> videoCodecs = supportedCodecs.get("video");
                    //Get index
                    for (int index = 0; index < videoCodecs.size(); index++) {
                        //Check codec
                        if (videoCodecs.get(index) == codec) {
                            //Check if it is first codec for audio
                            if (priority == Integer.MAX_VALUE) {
                                //Set port
                                setSendVideoPort(Integer.parseInt(port));
                                //And Ip
                                setSendVideoIp(mediaIp);
                            }
                            //Check if we have a lower priority
                            if (index < priority) {
                                //Store priority
                                priority = index;
                                //Set codec
                                setVideoCodec(codec);
                            }
                        }
                    }
                }
            }
        }
    }

    public boolean accept() {
        //Check state
        if (state != State.WAITING_ACCEPT) {
            //LOG
            Logger.getLogger(Spyer.class.getName()).log(Level.WARNING, "Accepted spyer is not in WAITING_ACCEPT state  [id:{0},state:{1}].", new Object[]{getSpyer_number(), state});
            //Error
            return false;
        }

        try {
            //Start receiving
            startReceiving();
            //Create final response
            SipServletResponse resp = inviteRequest.createResponse(200, "Ok");
            //Attach body
            resp.setContent(createSDP(), "application/sdp");
            String userName = SipEndPointManager.INSTANCE.getSipEndPointNameByNumber(spyee.getUsername());
            String userId = SipEndPointManager.INSTANCE.existNumber(spyee.getUsername());
            if (userName != null) {
                resp.addHeader("username", userName);
                resp.addHeader("userid", userId);
            }
            resp.setHeader("confUid", conf.getUID());
            //Send it
            resp.send();
        } catch (Exception ex) {
            try {
                //Create final response
                SipServletResponse resp = inviteRequest.createResponse(500, ex.getMessage());
                //Send it
                resp.send();
                //Log
                Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex);
            } catch (IOException ex1) {
                //Log
                Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex1);
            }
            //Terminate
            error(State.ERROR, "Error");
            //Error
            return false;
        }
        //Ok
        return true;
    }

    protected void error(State state, String message) {
        //Set the state
        setState(state);
        //Check cdr
        //Teminate
        destroy();
    }

    public void startSending() throws XmlRpcException {
        //Get client
        //XmlRpcMcuClient client = conf.getMCUClient();
        //Get conf id
        Integer confId = conf.getId();


        //Check video
        if (getSendVideoPort() != 0) {
            //Get profile bitrat
            int bitrate = profile.getVideoBitrate();
            //Reduce to the maximum in SDP
            if (videoBitrate > 0 && videoBitrate < bitrate) //Reduce it
            {
                bitrate = videoBitrate;
            }
            //Set codec
            //client.SetVideoCodec(confId, id, getVideoCodec(), profile.getVideoSize(), profile.getVideoFPS(), bitrate, 0, 0, profile.getIntraPeriod());
            //Send
            client.StartSendingVideoSpy(confId, id, getSendVideoIp(), getSendVideoPort(), getRtpOutMediaMap("video"));
            //Sending Video
            isSendingVideo = true;
        }
    }

    public void startReceiving() throws XmlRpcException {
        //Get client
        //XmlRpcMcuClient client = conf.getMCUClient();
        //Get conf id
        Integer confId = conf.getId();

        //If supported
        if (getVideoSupported()) {
            //Create rtp map for video
            createRTPMap("video");
            //Get receiving ports
            recVideoPort = client.StartReceivingVideoSpy(confId, id, getRtpInMediaMap("video"));
        }

        //And ip
        setRecIp(conf.getRTPIp());
    }

    public String createSDP() {

        //Create the sdp string
        String sdp = "v=0\r\n";
        sdp += "o=- 0 0 IN IP4 " + getRecIp() + "\r\n";
        sdp += "s=MediaMixerSession\r\n";
        sdp += "c=IN IP4 " + getRecIp() + "\r\n";
        sdp += "t=0 0\r\n";
        //Check if supported
        if (videoSupported) //Add video related lines to the sdp
        {
            sdp += addMediaToSdp("video", recVideoPort, 90000);
        }
        //Return
        return sdp;
    }

    private String addMediaToSdp(String mediaName, Integer port, int rate) {
        //Check if rtp map exist
        HashMap<Integer, Integer> rtpInMap = rtpInMediaMap.get(mediaName);

        //Check not null
        if (rtpInMap == null) {
            //Log
            Logger.getLogger(Spyer.class.getName()).log(Level.FINE, "addMediaToSdp rtpInMap is null. Disabling media {0} ", new Object[]{mediaName});
            //Return empty media
            return "m=" + mediaName + " 0 RTP/AVP\r\n";
        }

        //Create media string
        String mediaLine = "m=" + mediaName + " " + port + " RTP/AVP";

        //The string for the formats
        String formatLine = "";

        //Add rtmpmap for each codec in supported order
        for (Integer codec : supportedCodecs.get(mediaName)) {
            //Search for the codec
            for (Entry<Integer, Integer> mapping : rtpInMap.entrySet()) {
                //Check codec
                if (mapping.getValue() == codec) {
                    //Get type mapping
                    Integer type = mapping.getKey();
                    //Append type
                    mediaLine += " " + type;
                    //Append to sdp
                    formatLine += "a=rtpmap:" + type + " " + Codecs.getNameForCodec(mediaName, codec) + "/" + rate + "\r\n";
                    if (codec == Codecs.H264) {
                        //Check if we are offering first
                        if (h264profileLevelId.isEmpty()) //Set default profile
                        {
                            h264profileLevelId = h264profileLevelIdDefault;
                        }

                        //Check packetization mode
                        if (h264packetization > 0) //Add profile and packetization mode
                        {
                            formatLine += "a=fmtp:" + type + " profile-level-id=" + h264profileLevelId + ";packetization-mode=" + h264packetization + "\r\n";
                        } else //Add profile id
                        {
                            formatLine += "a=fmtp:" + type + " profile-level-id=" + h264profileLevelId + "\r\n";
                        }
                    }
                }
            }
        }
        //End media line
        mediaLine += "\r\n";

        //If not format has been found
        if (formatLine.isEmpty()) {
            //Log
            Logger.getLogger(Spyer.class.getName()).log(Level.FINE, "addMediaToSdp no compatible codecs found for media {0} ", new Object[]{mediaName});
            //Empty media line
            return "m=" + mediaName + " 0 RTP/AVP\r\n";
        }

        //If it is video and we have found the content attribute
        if (mediaName.equals("video") && !videoContentType.isEmpty()) {
            mediaLine += "a=content:" + videoContentType + "\r\n";
        }

        //Return the media string
        return mediaLine + formatLine;
    }

    public void createRTPMap(String media) {
        //Get supported codecs for media
        List<Integer> codecs = supportedCodecs.get(media);

        //Check if it supports media
        if (codecs != null) {
            //Create map
            HashMap<Integer, Integer> rtpInMap = new HashMap<Integer, Integer>();
            //Check if rtp map exist already for outgoing
            HashMap<Integer, Integer> rtpOutMap = rtpOutMediaMap.get(media);
            //If we do not have it
            if (rtpOutMap == null) {
                //Add all supported audio codecs with default values
                for (Integer codec : codecs) //Append it
                {
                    rtpInMap.put(codec, codec);
                }
            } else {
                //Add all supported audio codecs with already known mappings
                for (Map.Entry<Integer, Integer> pair : rtpOutMap.entrySet()) //Check if it is supported
                {
                    if (codecs.contains(pair.getValue())) //Append it
                    {
                        rtpInMap.put(pair.getKey(), pair.getValue());
                    }
                }
            }

            //Put the map back in the map
            rtpInMediaMap.put(media, rtpInMap);
        }
    }

    public void addSupportedCodec(String media, Integer codec) {
        //Check if we have the media
        if (!supportedCodecs.containsKey(media)) //Create it
        {
            supportedCodecs.put(media, new Vector<Integer>());
        }
        //Add codec to media
        supportedCodecs.get(media).add(codec);
    }

    public void destroy() {
        //Logger.getLogger("global").log(Level.SEVERE, "destroy participant " + id);
        //Check if already was destroying
        synchronized (this) {
            if (isDestroying) //Do nothing
            {
                return;
            }
            //We are isDestroying
            isDestroying = true;
        }
        try {
            //Get client
            //Delete participant
            client.DeleteSpy(conf.getId(), id);
        } catch (XmlRpcException ex) {
            Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex);
        }

        try {
            //If ther was a session
            if (session != null && session.isValid()) {
                //Remove participant from session
                session.removeAttribute("user");
                //Invalidate the session when appropiate
                session.setInvalidateWhenReady(true);
            }
            //If there was an application session
            if (appSession != null && appSession.isValid()) {
                //Remove participant from session
                appSession.removeAttribute("user");
                //Set expire time to let it handle any internal stuff
                appSession.setExpires(1);
                //Invalidate the session when appropiate
                appSession.setInvalidateWhenReady(true);
            }
        } catch (Exception ex) {
            Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Set state
        setState(State.DESTROYED);
        conf.getMixer().releaseMcuClient(client);
    }

    public void end() {
        //Log
        Logger.getLogger(Spyer.class.getName()).log(Level.INFO, "Ending RTP user id:{0} in state {1}", new Object[]{id, state});
        //Depending on the state
        switch (state) {
            case CONNECTING:
                break;
            case CONNECTED:
                doBye();
                break;
            case DISCONNECTED:
                break;
            default:
                //Destroy
                destroy();
        }
    }

    void doBye() {
        try {
            //Create BYE request
            SipServletRequest req = session.createRequest("BYE");
            String doRoute = System.getProperty("DoRoute");
            if (doRoute != null) {
                SipURI sipUri = SipEndPointManager.INSTANCE.getSipEndPointUriByNumber(getSpyer_number().toString());
                if (sipUri != null) {
                    req.pushRoute(sipUri);
                }
            }
            //Send it
            req.send();
        } catch (IOException ex) {
            Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex);
        } catch (IllegalStateException ex) {
            Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, null, ex);
        }
        try {
            //Set expire time
            appSession.setExpires(1);
        } catch (Exception ex) {
            Logger.getLogger(Spyer.class.getName()).log(Level.SEVERE, "Error expiring user", ex);
        }
        //Disconnect
        setState(State.DISCONNECTED);
        //Terminate
        destroy();
    }
}
