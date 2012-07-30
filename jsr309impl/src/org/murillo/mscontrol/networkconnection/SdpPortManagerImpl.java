/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.murillo.mscontrol.networkconnection;

import java.util.EnumMap;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.media.mscontrol.EventType;
import javax.media.mscontrol.MediaErr;
import javax.media.mscontrol.MediaEventListener;
import javax.media.mscontrol.MediaSession;
import javax.media.mscontrol.MsControlException;
import javax.media.mscontrol.Parameters;
import javax.media.mscontrol.networkconnection.CodecPolicy;
import javax.media.mscontrol.networkconnection.NetworkConnection;
import javax.media.mscontrol.networkconnection.SdpException;
import javax.media.mscontrol.networkconnection.SdpPortManager;
import javax.media.mscontrol.networkconnection.SdpPortManagerEvent;
import javax.media.mscontrol.networkconnection.SdpPortManagerException;
import org.murillo.MediaServer.Codecs;
import org.murillo.MediaServer.Codecs.MediaType;
import org.murillo.mscontrol.MediaSessionImpl;

/**
 *
 * @author Sergio
 */
public final class SdpPortManagerImpl implements SdpPortManager{
    private final NetworkConnectionImpl conn;
    private final HashSet<MediaEventListener<SdpPortManagerEvent>> listeners;
    private String recIp;
    private Integer recAudioPort;
    private Integer recVideoPort;
    private Integer recTextPort;
    private Integer sendAudioPort;
    private String  sendAudioIp;
    private Integer sendVideoPort;
    private String  sendVideoIp;
    private Integer sendTextPort;
    private String  sendTextIp;
    private Integer audioCodec;
    private Integer videoCodec;
    private Integer textCodec;
    private Boolean audioMuted;
    private Boolean videoMuted;
    private Boolean textMuted;
    private Boolean audioSupported;
    private Boolean videoSupported;
    private Boolean textSupported;
    private String h264profileLevelId;
    private int h264packetization;
    private String  localSDP;
    private String  remoteSDP;

    private HashMap<String,List<Integer>> supportedCodecs = null;
    private HashSet<Integer> requiredCodecs = null;
    private HashMap<String,HashMap<Integer,Integer>> rtpInMediaMap = null;
    private HashMap<String,HashMap<Integer,Integer>> rtpOutMediaMap = null;
    private String rejectedMedias;
    private CodecPolicy codecPolicy;
    private String videoContentType;
    
    public SdpPortManagerImpl(NetworkConnectionImpl conn,Parameters params) {
        //Store connection
        this.conn = conn;
        //No sending ports
        this.sendAudioPort = 0;
        this.sendVideoPort = 0;
        this.sendTextPort = 0;
        //Supported media
        this.audioSupported = true;
        this.videoSupported = true;
        this.textSupported = true;
        //No codecs
        audioCodec = -1;
        videoCodec = -1;
        textCodec = -1;
        //Create supported codec map
        supportedCodecs = new HashMap<String, List<Integer>>();
        //Create required set
        requiredCodecs = new HashSet<Integer>();
        //Create media maps
        rtpInMediaMap = new HashMap<String,HashMap<Integer,Integer>>();
        rtpOutMediaMap = new HashMap<String,HashMap<Integer,Integer>>();
        //Create listeners
        listeners = new HashSet<MediaEventListener<SdpPortManagerEvent>>();
        //Enable all audio codecs
        addSupportedCodec("audio", Codecs.PCMU);
        addSupportedCodec("audio", Codecs.PCMA);
        addSupportedCodec("audio", Codecs.GSM);
        addSupportedCodec("audio", Codecs.AMR);
        addSupportedCodec("audio", Codecs.TELEFONE_EVENT);
        //Enable all video codecs
        addSupportedCodec("video", Codecs.H264);
        addSupportedCodec("video", Codecs.H263_1998);
        addSupportedCodec("video", Codecs.H263_1996);
        //Enable all text codecs
        addSupportedCodec("text", Codecs.T140RED);
        addSupportedCodec("text", Codecs.T140);
        //No rejected medias and no video content type
        rejectedMedias = "";
        videoContentType = "";
        //Set default level and packetization
        h264profileLevelId = "";
        h264packetization = 0;
    }

    @Override
    public void generateSdpOffer() throws SdpPortManagerException {
        try {
            //Start receivint
            conn.startReceiving(this);
        } catch (MsControlException ex) {
            Logger.getLogger(SdpPortManagerImpl.class.getName()).log(Level.SEVERE, null, ex);
            throw new SdpPortManagerException("Could not start receiving", ex);
        }
        //Set default profile
        h264profileLevelId = "428015";
        //Create sdp
        localSDP = createSDP();
        //Generate success
        success(SdpPortManagerEvent.OFFER_GENERATED);
    }

    @Override
    public void processSdpOffer(byte[] sdp) throws SdpPortManagerException {
        //Store remote sdp
        remoteSDP = new String(sdp);

        //Cacht any parsing error
        try {
            //Procces it
            processSDP(remoteSDP);
        } catch (SdpPortManagerException e) {
            //Send error
            error(SdpPortManagerEvent.SDP_NOT_ACCEPTABLE, e.getMessage());
            //exit
            throw new SdpException("SDP not acceptable: "+e.getMessage(), e);
        }
        try {
            //Start receivint
            conn.startReceiving(this);
            //Create sdp
            localSDP = createSDP();
            //Generate success
            success(SdpPortManagerEvent.ANSWER_GENERATED);
            //Start sending
            conn.startSending(this);
        } catch (MsControlException ex) {
            //And thow
            throw new SdpPortManagerException("Could not start", ex);
        }
    }

    @Override
    public void processSdpAnswer(byte[] sdp) throws SdpPortManagerException {
        
        //Store remote sdp
        remoteSDP = new String(sdp);
        
        try {
            //Procces it
            processSDP(remoteSDP);
            //Generate success
            success(SdpPortManagerEvent.ANSWER_PROCESSED);
        } catch (SdpPortManagerException e) {
            //Send error
            error(SdpPortManagerEvent.SDP_NOT_ACCEPTABLE, e.getMessage());
            //exit
            throw new SdpException("SDP not acceptable "+e.getMessage(), e);
        }
        
        try {
            //Start sending
            conn.startSending(this);
        } catch (MsControlException ex) {
            Logger.getLogger(SdpPortManagerImpl.class.getName()).log(Level.SEVERE, null, ex);
            throw new SdpPortManagerException("Could not start sending", ex);
        }
    }

    @Override
    public void rejectSdpOffer() throws SdpPortManagerException {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public byte[] getMediaServerSessionDescription() throws SdpPortManagerException {
        return localSDP.getBytes();
    }

    @Override
    public byte[] getUserAgentSessionDescription() throws SdpPortManagerException {
        return remoteSDP.getBytes();
    }

    @Override
    public void setCodecPolicy(CodecPolicy codecPolicy) throws SdpPortManagerException {
        //Get policies
        String[] mediaTypeCapabilities = codecPolicy.getMediaTypeCapabilities();
        String[] codecCapabilities = codecPolicy.getCodecCapabilities();
        String[] requiredCodecsNames = codecPolicy.getRequiredCodecs();
        String[] excludedCodecs = codecPolicy.getExcludedCodecs();
        String[] codecPreferences = codecPolicy.getCodecPreferences();

         //The temporal support
        HashSet<MediaType> mediaSupported = new HashSet<MediaType>();

        //If there are restrictions about media types supported
        if (mediaTypeCapabilities.length>0)
        {
            //Check supported medias
            for (String media: mediaTypeCapabilities)
            {
                //Check media
                if ("audio".equalsIgnoreCase(media)) {
                    //It is enabled
                    mediaSupported.add(MediaType.AUDIO);
                } else if ("video".equalsIgnoreCase(media)) {
                    //It is enabled
                    mediaSupported.add(MediaType.VIDEO);
                } else if ("text".equalsIgnoreCase(media)) {
                    //It is enabled
                    mediaSupported.add(MediaType.TEXT);
                }
            }
        } else {
            //Add all
            mediaSupported.add(MediaType.AUDIO);
            mediaSupported.add(MediaType.VIDEO);
            mediaSupported.add(MediaType.TEXT);
        }

        //Create temporal required
        HashSet<Integer> required = new HashSet<Integer>();

        //For each codec
        for (String codec : requiredCodecsNames)
        {
                //Get codec info
                Codecs.CodecInfo info = Codecs.getCodecInfoForname(codec);
                //If we do not know it
                if (info==null)
                    //Exit
                    throw new SdpPortManagerException("Required codec "+codec+" not supported");
                //Put codecs for that media
                required.add(info.getCodec());
        }

        //The new codecs
        EnumMap<MediaType,HashSet<Integer>> codecs = new EnumMap<MediaType, HashSet<Integer>>(MediaType.class);
        
        //Create codecs
        HashSet<Integer> audioCodecs = new HashSet<Integer>();
        HashSet<Integer> videoCodecs = new HashSet<Integer>();
        HashSet<Integer> textCodecs = new HashSet<Integer>();

        //If media supported
        if (mediaSupported.contains(MediaType.AUDIO))
            //Put in codecs
            codecs.put(MediaType.AUDIO, audioCodecs);
        //If media supported
        if (mediaSupported.contains(MediaType.VIDEO))
            //Put in codecs
            codecs.put(MediaType.VIDEO, videoCodecs);
        //If media supported
        if (mediaSupported.contains(MediaType.TEXT))
            //Put in codecs
            codecs.put(MediaType.TEXT, textCodecs);

        //Create temporal capabilities
        HashSet<Integer> capabilities = new HashSet<Integer>();

        //Check if there codec policies
        if (codecCapabilities.length>0)
        {
            //For each codec
            for(String codec : codecCapabilities)
            {
                //Get codec info
                Codecs.CodecInfo info = Codecs.getCodecInfoForname(codec);
                //If we know it and media is supported
                if (info!=null && mediaSupported.contains(info.getMedia()))
                {
                    //Put codecs for that media
                    codecs.get(info.getMedia()).add(info.getCodec());
                    //And in the capabilities
                    capabilities.add(info.getCodec());
                }
            }
        }

        //Check audio codecs
        if (mediaSupported.contains(MediaType.AUDIO) && audioCodecs.isEmpty())
        {
            //Enable all audio codecs
            audioCodecs.add(Codecs.PCMU);
            audioCodecs.add(Codecs.PCMA);
            audioCodecs.add(Codecs.GSM);
            audioCodecs.add(Codecs.AMR);
            audioCodecs.add(Codecs.TELEFONE_EVENT);
        }

        //Check video codecs
        if (mediaSupported.contains(MediaType.VIDEO) && videoCodecs.isEmpty())
        {
            //Enable all video codecs
            videoCodecs.add(Codecs.H264);
            videoCodecs.add(Codecs.H263_1998);
            videoCodecs.add(Codecs.H263_1996);
        }

        //Check text codecs
        if (mediaSupported.contains(MediaType.TEXT) && textCodecs.isEmpty())
        {
            //Enable all text codecs
            textCodecs.add(Codecs.T140RED);
            textCodecs.add(Codecs.T140);
        }

        //Create temporal excluded
        HashSet<Integer> excluded = new HashSet<Integer>();

        //For each exludec codec
        for(String codec : excludedCodecs)
        {
            //Get codec info
            Codecs.CodecInfo info = Codecs.getCodecInfoForname(codec);
            //If we know it
            if (info!=null)
            {
                //First check if it is in the required
                if (required.contains(info.getCodec()))
                    //Exception
                    throw new SdpPortManagerException("Codec "+codec +" in required and excluded list");
                //Also in the capabilities
                if (capabilities.contains(info.getCodec()))
                    //Exception
                    throw new SdpPortManagerException("Codec "+codec +" in required and capability list");
                //Remove that codec
                codecs.get(info.getMedia()).remove(info.getCodec());
                //Add to excluded list
                excluded.add(info.getCodec());
            }
        }

        //Disable all
        audioSupported = false;
        videoSupported = false;
        textSupported = false;

        //If audio is enabled
        if (codecs.containsKey(MediaType.AUDIO))
        {
            //Check if we have any codec to offer
            if (codecs.get(MediaType.AUDIO).isEmpty())
                //Exception
                throw new SdpPortManagerException("No supported codecs for audio");
            //Audio enabled
            audioSupported = true;
         }

        //If video is enabled
        if (codecs.containsKey(MediaType.VIDEO))
        {
            //Check if we have any codec to offer
            if (codecs.get(MediaType.VIDEO).isEmpty())
                //Exception
                throw new SdpPortManagerException("No supported codecs for video");
            //Video enabled
            videoSupported = true;
        }

        //If text is enabled
        if (codecs.containsKey(MediaType.TEXT))
        {
            //Check if we have any codec to offer
            if (codecs.get(MediaType.TEXT).isEmpty())
                throw new SdpPortManagerException("No supported codecs for text");
            //Video  enabled
            videoSupported = true;
        }

        //Store it
        this.codecPolicy = codecPolicy;

        //Clear audio codecs
        clearSupportedCodec("audio");
        //Clear video  codecs
        clearSupportedCodec("video");
        //Clear text codecs
        clearSupportedCodec("text");

        //Set preference
        for (String pref : codecPreferences)
        {
            //Get codec info
            Codecs.CodecInfo info = Codecs.getCodecInfoForname(pref);
            //If we know it and media is suported
            if (info!=null && mediaSupported.contains(info.getMedia()))
            {
                //Check it was not excluded
                if (excluded.contains(info.getCodec()))
                    //Exit
                    throw new SdpPortManagerException("Prefered codec "+pref+" in exclude list");
                //Get codecs for that media
                HashSet<Integer> mediaCodecs = codecs.get(info.getMedia());
                //If we had it enabled
                if (mediaCodecs!=null && mediaCodecs.contains(info.getCodec()))
                {
                    //Add supported codec
                    addSupportedCodec(info.getMedia().getName(), info.getCodec());
                    //Remove from set
                    mediaCodecs.remove(info.getCodec());
                } 
            }
        }

        //For all media
        for (MediaType type : codecs.keySet())
        {
            //For all pending codecs
            for(Integer codec : codecs.get(type))
            {
                //Add supported codec
                addSupportedCodec(type.getName(),codec);
            }
        }

        //Clear required
        requiredCodecs.clear();

        //Set new required
        required.addAll(required);
    }

    @Override
    public CodecPolicy getCodecPolicy() {
        //Return policy
        return codecPolicy;
    }

    @Override
    public NetworkConnection getContainer() {
        //return connection
        return conn;
    }

    @Override
    public void addListener(MediaEventListener<SdpPortManagerEvent> ml) {
        //Add it
        listeners.add(ml);
    }

    @Override
    public void removeListener(MediaEventListener<SdpPortManagerEvent> ml) {
        //Remove it
        listeners.remove(ml);
    }

    @Override
    public MediaSession getMediaSession() {
        //Return media sessions
        return conn.getMediaSession();
    }

    public void clearSupportedCodec(String media) {
         //Check if we have the media
         if (supportedCodecs.containsKey(media))
            //clear it
            supportedCodecs.get(media).clear();
     }

     public void addSupportedCodec(String media,Integer codec) {
         //Check if we have the media
         if (!supportedCodecs.containsKey(media))
             //Create it
             supportedCodecs.put(media, new Vector<Integer>());
         //Add codec to media
         supportedCodecs.get(media).add(codec);
     }

     public Integer getRecAudioPort() {
        return recAudioPort;
    }

    public void setRecAudioPort(Integer recAudioPort) {
        this.recAudioPort = recAudioPort;
    }

    public Integer getRecTextPort() {
        return recTextPort;
    }

    public void setRecTextPort(Integer recTextPort) {
        this.recTextPort = recTextPort;
    }

    public String getRecIp() {
        return recIp;
    }

    public void setRecIp(String recIp) {
        this.recIp = recIp;
    }

    public Integer getRecVideoPort() {
        return recVideoPort;
    }

    public void setRecVideoPort(Integer recVideoPort) {
        this.recVideoPort = recVideoPort;
    }

    public Integer getSendAudioPort() {
        return sendAudioPort;
    }

    public void setSendAudioPort(Integer sendAudioPort) {
        this.sendAudioPort = sendAudioPort;
    }

    public Integer getSendVideoPort() {
        return sendVideoPort;
    }

    public void setSendVideoPort(Integer sendVideoPort) {
        this.sendVideoPort = sendVideoPort;
    }

    public Integer getSendTextPort() {
        return sendTextPort;
    }

    public void setSendTextPort(Integer sendTextPort) {
        this.sendTextPort = sendTextPort;
    }

    public Integer getAudioCodec() {
        return audioCodec;
    }

    public void setAudioCodec(Integer audioCodec) {
        this.audioCodec = audioCodec;
    }

    public Integer getTextCodec() {
        return textCodec;
    }

    public void setTextCodec(Integer textCodec) {
        this.textCodec = textCodec;
    }

    public Integer getVideoCodec() {
        return videoCodec;
    }

    public void setVideoCodec(Integer videoCodec) {
        this.videoCodec = videoCodec;
    }

    public Boolean getAudioMuted() {
        return audioMuted;
    }

    public void setAudioMuted(Boolean audioMuted) {
        this.audioMuted = audioMuted;
    }

    public Boolean getVideoMuted() {
        return videoMuted;
    }

    public void setVideoMuted(Boolean videoMuted) {
        this.videoMuted = videoMuted;
    }

    public Boolean getTextMuted() {
        return textMuted;
    }

    public void setTextMuted(Boolean TextMuted) {
        this.textMuted = TextMuted;
    }

    public Boolean getAudioSupported() {
        return audioSupported;
    }

    public Boolean getTextSupported() {
        return textSupported;
    }

    public Boolean getVideoSupported() {
        return videoSupported;
    }

    public HashMap<Integer,Integer> getRtpInMediaMap(String media) {
        //Return rtp mapping for media
        return rtpInMediaMap.get(media);
    }

    HashMap<Integer, Integer> getRtpOutMediaMap(String media) {
        //Return rtp mapping for media
        return rtpOutMediaMap.get(media);
    }

    public String createSDP() {

        //Create the sdp string
        String sdp = "v=0\r\n";
        sdp += "o=- 0 0 IN IP4 " + getRecIp() +"\r\n";
        sdp += "s=MediaMixerSession\r\n";
        sdp += "c=IN IP4 "+ getRecIp() +"\r\n";
        sdp += "t=0 0\r\n";

        //Check if supported
        if (audioSupported)
            //Add audio related lines to the sdp
            sdp += addMediaToSdp("audio",recAudioPort,8000);
        //Check if supported
        if (videoSupported)
            //Add video related lines to the sdp
            sdp += addMediaToSdp("video",recVideoPort,90000);
        //Check if supported
        if (textSupported)
            //Add text related lines to the sdp
            sdp += addMediaToSdp("text",recTextPort,1000);
        //Add rejected medias
        sdp += rejectedMedias;
        //Return
        return sdp;
    }

    public void createRTPMap(String media)
    {
        //Get supported codecs for media
        List<Integer> codecs = supportedCodecs.get(media);

        //Check if it supports audio
        if (codecs!=null)
        {
            //Create map
            HashMap<Integer, Integer> rtpInMap = new HashMap<Integer, Integer>();
            //Check if rtp map exist already for outgoing
            HashMap<Integer, Integer> rtpOutMap = rtpOutMediaMap.get(media);
            //If we do not have it
            if (rtpOutMap==null)
            {
                //Add all supported audio codecs with default values
                for(Integer codec : codecs)
                    //Append it
                    rtpInMap.put(codec, codec);
            } else {
                //Add all supported audio codecs with already known mappings
                for(Map.Entry<Integer,Integer> pair : rtpOutMap.entrySet())
                    //Check if it is supported
                    if (codecs.contains(pair.getValue()))
                        //Append it
                        rtpInMap.put(pair.getKey(), pair.getValue());
            }

            //Put the map back in the map
            rtpInMediaMap.put(media, rtpInMap);
        }
    }

    private Integer findTypeForCodec(HashMap<Integer, Integer> rtpMap, Integer codec) {
        for (Map.Entry<Integer,Integer> pair  : rtpMap.entrySet())
            if (pair.getValue()==codec)
                return pair.getKey();
        return -1;
    }

    private String addMediaToSdp(String mediaName, Integer port, int rate)
    {
        //Check if rtp map exist
        HashMap<Integer, Integer> rtpInMap = rtpInMediaMap.get(mediaName);

        //Check not null
        if (rtpInMap==null)
        {
            //Log
            Logger.getLogger(SdpPortManager.class.getName()).log(Level.FINE, "addMediaToSdp rtpInMap is null. Disabling media ? ",mediaName);
            //Return empty media
            return "m=" +mediaName + " 0 RTP/AVP\r\n";
        }

        //Create media string
        String mediaLine = "m=" +mediaName + " " + port +" RTP/AVP";

        //The string for the formats
        String formatLine = "";

        //Add rtmpmap for each codec in supported order
        for (Integer codec : supportedCodecs.get(mediaName))
        {
            //Search for the codec
            for (Entry<Integer,Integer> mapping : rtpInMap.entrySet())
            {
                //Check codec
                if (mapping.getValue()==codec)
                {
                    //Get type mapping
                    Integer type = mapping.getKey();
                    //Append type
                    mediaLine += " " +type;
                    //Append to sdp
                    formatLine += "a=rtpmap:" + type + " "+ Codecs.getNameForCodec(mediaName, codec) + "/"+rate+"\r\n";
                    //Add h264 ftmp
                    if (codec==Codecs.H264)
                    {
                        //Check packetization mode
                        if (h264packetization>0)
                            //Add profile and packetization mode
                            formatLine += "a=fmtp:" + type + " profile-level-id="+h264profileLevelId+";packetization-mode="+h264packetization+"\r\n";
                        else
                            //Add profile id
                            formatLine += "a=fmtp:" + type + " profile-level-id="+h264profileLevelId+"\r\n";
                    } else if (codec==Codecs.H263_1996) {
                        //Add profile id
                        formatLine += "a=fmtp:" + type + " CIF=1;QCIF=1\r\n";
                    }  else if (codec == Codecs.T140RED) {
                        //Find t140 codec
                        Integer t140 = findTypeForCodec(rtpInMap,Codecs.T140);
                        //Check that we have founf it
                        if (t140>0)
                            //Add redundancy type
                            formatLine += "a=fmtp:" + type + " " + t140 + "/" + t140 + "/" + t140 +"\r\n";
                    }
                }
            }
        }
        //End media line
        mediaLine += "\r\n";

        //If it is video and set the default bitrate
        if (mediaName.equals("video"))
                //Set to 784-64 for audio
                mediaLine += "b=AS:720\r\n";

        //If not format has been found
        if (formatLine.isEmpty())
        {
            //Log
            Logger.getLogger(SdpPortManager.class.getName()).log(Level.FINE, "addMediaToSdp no compatible codecs found for media {0} ", new Object[]{mediaName});
            //Empty media line
            return "m=" +mediaName + " 0 RTP/AVP\r\n";
        }

        //If it is video and we have found the content attribute
        if (mediaName.equals("video") && !videoContentType.isEmpty())
            mediaLine += "a=content:"+videoContentType+"\r\n";

        //Return the media string
        return mediaLine+formatLine;
    }

    public void processSDP(String content) throws SdpPortManagerException {
        //Check required
        HashSet<Integer> required = (HashSet<Integer>) requiredCodecs.clone();
        //Search for the ip
        int i = content.indexOf("\r\nc=IN IP4 ");
        int j = content.indexOf("\r\n", i+1);
        //Get the ip
        String ip = content.substring(i + 11, j);

        //Check if ip should be nat for this media mixer
        if (conn.getMediaServer().isNated(ip))
            //Do natting
            ip = "0.0.0.0";

        //Disable supported media
        audioSupported = false;
        videoSupported = false;
        textSupported = false;

        //Search the next media
        int m = content.indexOf("\r\nm=", j);
        
        //Search medias
        while (m > 0) {
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
            String transport = content.substring(i,j);

            //Search format list
            i = j + 1;
            j = content.indexOf("\r\n", i);
            //Get it
            String fmtString = content.substring(i, j);

            //Get fmts
            StringTokenizer fmts  = new StringTokenizer(fmtString);

            //Add support for the media
            if (media.equals("audio")) {
                //Set as supported
                audioSupported = true;
            } else if (media.equals("video")) {
                //Check for content attribute inside the media
                i = content.indexOf("\r\na=content:", j);
                //Check if we found it inside this media
                if (i>0 && (i<m || m<0))
                {
                    //Get end
                    int k = content.indexOf("\r\n", i+1);
                    //Get it
                    String mediaContentType = content.substring(i+12, k);
                    //Check if it is not main
                    if (!mediaContentType.equalsIgnoreCase("main"))
                    {
                        //Add video content to the rejected media list
                        rejectedMedias += "m="+media+" 0 "+transport+" "+fmtString+"\r\na=content:"+mediaContentType+"\r\n";
                        //Skip it
                        continue;
                    } else {
                        //Add the content type to the line
                        videoContentType = mediaContentType;
                    }
                }
                //Set as supported
                videoSupported = true;
            } else if (media.equals("text")) {
                //Set as supported
                textSupported = true;
            } else {
                //Add it to the rejected media lines
                rejectedMedias += "m="+media+" 0 "+transport+" "+fmtString+"\r\n";
                //Not supported media type
                continue;
            }

            //Check if we have input map for that
            if (!rtpOutMediaMap.containsKey(media))
                //Create new map
                rtpOutMediaMap.put(media, new HashMap<Integer, Integer>());

            //Get all codecs
            while (fmts.hasMoreTokens()) {
                try {
                    //Get codec
                    Integer codec = Integer.parseInt(fmts.nextToken());
                    //If it is static
                    if (codec<96)
                        //Put it in the map
                        rtpOutMediaMap.get(media).put(codec,codec);
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
            if (i>0 && (i<m || m<0))
            {
                //Get end
                int k = content.indexOf("\r\n", i+1);
                //Get it
                mediaIp = content.substring(i+11, k);
                //Check if ip should be nat for this media mixer
                if (conn.getMediaServer().isNated(mediaIp))
                    //Do natting
                    mediaIp = "0.0.0.0";
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
                Integer type = Integer.parseInt(content.substring(f+9,i));
                //Get the media type
                String codecName = content.substring(i+1, j);
                //Get codec for name
                Integer codec = Codecs.getCodecForName(media,codecName);
                //Remove from required
                required.remove(codec);
                //if it is h264 TODO: FIIIIX!!!
                if (codec==Codecs.H264)
                {
                    //start of fmtp line
                    String start = "a=fmtp:"+type;
                    //Check if we have format for it
                    int k = content.indexOf(start,ini);
                    //If we have it
                    if (k!=-1)
                    {
                        //Get ftmp line
                        String ftmpLine = content.substring(k,content.indexOf("\r\n",k));
                        //Get profile id
                        k = ftmpLine.indexOf("profile-level-id=");
                        //Check it
                        if (k!=-1)
                        {
                            //Get profile
                            String profile = ftmpLine.substring(k+17,k+23);
                            //Convert and compare
                            if (maxh264profile.isEmpty() || Integer.parseInt(profile,16)>Integer.parseInt(maxh264profile,16))
                            {
                                //Store this type provisionally
                                h264type = type;
                                //store new profile value
                                maxh264profile = profile;
                                //Check if it has packetization parameter
                                if (ftmpLine.indexOf("packetization-mode=1")!=-1)
                                    //Set it
                                    h264packetization = 1;
                            }
                        }
                    } else {
                        //check if no profile has been received so far
                        if (maxh264profile.isEmpty())
                            //Store this type provisionally
                            h264type = type;
                    }
                } else if (codec!=-1) {
                    //Set codec mapping
                     rtpOutMediaMap.get(media).put(type,codec);
                }
                //Get next
                f = content.indexOf("a=rtpmap:", j);
            }

            //Check if we have type for h264
            if (h264type>0)
            {
                //Store profile level
                h264profileLevelId = maxh264profile;
                //add it
                rtpOutMediaMap.get(media).put(h264type,Codecs.H264);
            }

            //Check required
            if (!required.isEmpty())
                //Set error
                throw new SdpPortManagerException("Required codecs not found");
            
            //For each entry
            for (Integer codec : rtpOutMediaMap.get(media).values())
            {
                    //Check the media type
                    if (media.equals("audio"))
                    {
                        //Get suppoorted codec for media
                        List<Integer> audioCodecs = supportedCodecs.get("audio");
                        //Get index
                        for (int index=0;index<audioCodecs.size();index++)
                        {
                            //Check codec
                            if (audioCodecs.get(index)==codec)
                            {
                                //Check if it is first codec for audio
                                if (priority==Integer.MAX_VALUE)
                                {
                                    //Set port
                                    setSendAudioPort(Integer.parseInt(port));
                                    //And Ip
                                    setSendAudioIp(mediaIp);
                                }
                                //Check if we have a lower priority
                                if (index<priority)
                                {
                                    //Store priority
                                    priority = index;
                                    //Set codec
                                    setAudioCodec(codec);
                                }
                            }
                        }
                    }
                    else if (media.equals("video"))
                    {
                        //Get suppoorted codec for media
                        List<Integer> videoCodecs = supportedCodecs.get("video");
                        //Get index
                        for (int index=0;index<videoCodecs.size();index++)
                        {
                            //Check codec
                            if (videoCodecs.get(index)==codec)
                            {
                                //Check if it is first codec for audio
                                if (priority==Integer.MAX_VALUE)
                                {
                                    //Set port
                                    setSendVideoPort(Integer.parseInt(port));
                                    //And Ip
                                    setSendVideoIp(mediaIp);
                                }
                                //Check if we have a lower priority
                                if (index<priority)
                                {
                                    //Store priority
                                    priority = index;
                                    //Set codec
                                    setVideoCodec(codec);
                                }
                            }
                        }
                    }
                    else if (media.equals("text"))
                    {
                        //Get suppoorted codec for media
                        List<Integer> textCodecs = supportedCodecs.get("text");
                        //Get index
                        for (int index=0;index<textCodecs.size();index++)
                        {
                            //Check codec
                            if (textCodecs.get(index)==codec)
                            {
                                //Check if it is first codec for audio
                                if (priority==Integer.MAX_VALUE)
                                {
                                    //Set port
                                    setSendTextPort(Integer.parseInt(port));
                                    //And Ip
                                    setSendTextIp(mediaIp);
                                }
                                //Check if we have a lower priority
                                if (index<priority)
                                {
                                    //Store priority
                                    priority = index;
                                    //Set codec
                                    setTextCodec(codec);
                                }
                            }
                        }
                    }
                }
            }
        //Check we have at least one codec
        if (audioSupported && audioCodec==null)
            //Set error
            throw new SdpPortManagerException("No suitable codec found for audio");
        //Check we have at least one codec
        if (videoSupported && videoCodec==null)
            //Set error
            throw new SdpPortManagerException("No suitable codec found for video");
        //Check we have at least one codec
        if (textSupported && textCodec==null)
            //Set error
            throw new SdpPortManagerException("No suitable codec found for text");
    }


    private void success(EventType eventType) {
        //Create and fire event
        fireEvent(new SdpPortManagerEventImpl(this, eventType));
    }

    private void error(MediaErr error,String errorMsg) {
       //Create and fire event
        fireEvent(new SdpPortManagerEventImpl(this, error, errorMsg));
    }

    private void fireEvent(final SdpPortManagerEventImpl event) {
        //Get media session
        MediaSessionImpl session = (MediaSessionImpl) conn.getMediaSession();
        //exec async
        session.Exec(new Runnable() {
            @Override
            public void run() {
                //For each listener in set
                 for (MediaEventListener<SdpPortManagerEvent> listener : listeners)
                     //Send it
                    listener.onEvent(event);
            }
        });
    }

    public String getSendAudioIp() {
        return sendAudioIp;
    }

    public void setSendAudioIp(String sendAudioIp) {
        this.sendAudioIp = sendAudioIp;
    }

    public String getSendTextIp() {
        return sendTextIp;
    }

    public void setSendTextIp(String sendTextIP) {
        this.sendTextIp = sendTextIP;
    }

    public String getSendVideoIp() {
        return sendVideoIp;
    }

    public void setSendVideoIp(String sendVideoIp) {
        this.sendVideoIp = sendVideoIp;
    }
}
