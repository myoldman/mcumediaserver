/*
 * RTPParticipant.java
 *
 * Copyright (C) 2007  Sergio Garcia Murillo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.murillo.mcuWeb;

import com.ffcs.mcu.SipEndPointManager;
import com.ffcs.mcu.Spyer;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.mail.BodyPart;
import javax.mail.MessagingException;
import javax.mail.Multipart;
import javax.mail.internet.MimeBodyPart;
import javax.mail.internet.MimeMultipart;
import javax.servlet.sip.Address;
import javax.servlet.sip.SipApplicationSession;
import javax.servlet.sip.SipFactory;
import javax.servlet.sip.SipServletRequest;
import javax.servlet.sip.SipServletResponse;
import javax.servlet.sip.SipSession;
import javax.servlet.sip.SipURI;
import org.apache.xmlrpc.XmlRpcException;
import org.murillo.MediaServer.Codecs;
import org.murillo.MediaServer.XmlRpcMcuClient;
import org.murillo.MediaServer.XmlRpcMcuClient.MediaStatistics;
import org.murillo.mcuWeb.Participant.State;

/**
 *
 * @author Sergio
 */
public class RTPParticipant extends Participant  implements Spyer.Listener {

    private Address address;
    private SipSession session = null;
    private SipApplicationSession appSession = null;
    private SipServletRequest inviteRequest = null;
    private Integer mosaicId;
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
    private String location;
    private Integer totalPacketCount;
    private Map<String, MediaStatistics> stats;

    private HashMap<String,List<Integer>> supportedCodecs = null;
    private HashMap<String,HashMap<Integer,Integer>> rtpInMediaMap = null;
    private HashMap<String,HashMap<Integer,Integer>> rtpOutMediaMap = null;
    private boolean isSendingAudio;
    private boolean isSendingVideo;
    private boolean isSendingText;
    private String rejectedMedias;
    private String videoContentType;
    private String h264profileLevelId;
    private int h264packetization;
    private final String h264profileLevelIdDefault = "428014";
    private int videoBitrate;

    public RTPParticipant(Integer id,String name,Integer mosaicId,Conference conf) throws XmlRpcException {
        //Call parent
        super(id,name,mosaicId,conf,Type.SIP);
        //Not sending
        isSendingAudio = false;
        isSendingVideo = false;
        isSendingText = false;
        //No sending ports
        sendAudioPort = 0;
        sendVideoPort = 0;
        sendTextPort = 0;
        //Supported media
        audioSupported = true;
        videoSupported = true;
        textSupported = true;
        //Create supported codec map
        supportedCodecs = new HashMap<String, List<Integer>>();
        //Create media maps
        rtpInMediaMap = new HashMap<String,HashMap<Integer,Integer>>();
        rtpOutMediaMap = new HashMap<String,HashMap<Integer,Integer>>();
        //No rejected medias and no video content type
        rejectedMedias = "";
        videoContentType = "";
        //Set default level and packetization
        h264profileLevelId = "";
        h264packetization = 0;
}

    @Override
    public void restart() {
        //Get mcu client
        //XmlRpcMcuClient client = conf.getMCUClient();
        try {
            //Create participant in mixer conference and store new id
            id = client.CreateParticipant(id, mosaicId, name.replace('.', '_'), type.valueOf());
            //Check state
            if (state!=State.CREATED)
            {
                //Start sending
                startSending();
                //And receiving
                startReceiving();
            }
        } catch (XmlRpcException ex) {
            //End it
            end();
        }
    }

    public void addSupportedCodec(String media,Integer codec) {
         //Check if we have the media
         if (!supportedCodecs.containsKey(media))
             //Create it
             supportedCodecs.put(media, new Vector<Integer>());
         //Add codec to media
         supportedCodecs.get(media).add(codec);
     }

    public Address getAddress() {
        return address;
    }

    public String getUsername() {
         //Get sip uris
        SipURI uri = (SipURI) address.getURI();
        //Return username
        return uri.getUser();
    }

    public String getDomain() {
        //Get sip uris
        SipURI uri = (SipURI) address.getURI();
        //Return username
        return uri.getHost();
    }

    public String getUsernameDomain() {
        //Get sip uris
        SipURI uri = (SipURI) address.getURI();
        //Return username
        return uri.getUser()+"@"+uri.getHost();
    }

     boolean equalsUser(Address user) {
        //Get sip uris
        SipURI us = (SipURI) address.getURI();
        SipURI them = (SipURI) user.getURI();
        //If we have the same username and host/domain
        if (us.getUser().equals(them.getUser()) && us.getHost().equals(them.getHost()))
            return true;
        else
            return false;
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

    public String getSendAudioIp() {
        return sendAudioIp;
    }

    public void setSendAudioIp(String sendAudioIp) {
        this.sendAudioIp = sendAudioIp;
    }

    public String getSendTextIp() {
        return sendTextIp;
    }

    public void setSendTextIp(String sendTextIp) {
        this.sendTextIp = sendTextIp;
    }

    public String getSendVideoIp() {
        return sendVideoIp;
    }

    public void setSendVideoIp(String sendVideoIp) {
        this.sendVideoIp = sendVideoIp;
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

    public String getLocation() {
        return location;
    }

    public void setLocation(String location) {
        this.location = location;
    }

    @Override
    public boolean setVideoProfile(Profile profile) {
        //Check video is supported
        if (!getVideoSupported())
            //Exit
            return false;
        //Set new video profile
        this.profile = profile;
        try {
            //Get client
            //XmlRpcMcuClient client = conf.getMCUClient();
            //Get conf id
            Integer confId = conf.getId();
            //If it is sending video
            if (isSendingVideo)
            {
                //Stop sending video
                client.StopSendingVideo(confId, id);
                //Setup video with new profile
                client.SetVideoCodec(confId, id, getVideoCodec(), profile.getVideoSize(), profile.getVideoFPS(), profile.getVideoBitrate(), 0, 0, profile.getIntraPeriod());
                //Send video & audio
                client.StartSendingVideo(confId, id, getSendVideoIp(), getSendVideoPort(), getRtpOutMediaMap("video"));
            }
        } catch (XmlRpcException ex) {
            Logger.getLogger("global").log(Level.SEVERE, null, ex);
            return false;
        }
        return true;
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

        //Return
        return sdp;
    }

    public void createRTPMap(String media)
    {
        //Get supported codecs for media
        List<Integer> codecs = supportedCodecs.get(media);

        //Check if it supports media
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
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.FINE, "addMediaToSdp rtpInMap is null. Disabling media {0} ", new Object[]{mediaName});
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
                    if (codec==Codecs.H264)
                    {
                        //Check if we are offering first
                        if (h264profileLevelId.isEmpty())
                            //Set default profile
                            h264profileLevelId = h264profileLevelIdDefault;

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
                    } else if (codec == Codecs.T140RED) {
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

        //If not format has been found
        if (formatLine.isEmpty())
        {
            //Log
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.FINE, "addMediaToSdp no compatible codecs found for media {0} ", new Object[]{mediaName});
            //Empty media line
            return "m=" +mediaName + " 0 RTP/AVP\r\n";
        }

        //If it is video and we have found the content attribute
        if (mediaName.equals("video") && !videoContentType.isEmpty())
            mediaLine += "a=content:"+videoContentType+"\r\n";

        //Return the media string
        return mediaLine+formatLine;
    }

    private void proccesContent(String type, Object content) throws IOException {
        //No SDP
        String sdp = null;
        //Depending on the type
        if (type.equalsIgnoreCase("application/sdp"))
        {
            //Get it
            sdp = new String((byte[])content);
        } else if (type.startsWith("multipart/mixed")) {
            try {
                //Get multopart
                Multipart multipart = (Multipart) content;
                //For each content
                for (int i = 0; i < multipart.getCount(); i++)
                {
                    //Get content type
                    BodyPart bodyPart = multipart.getBodyPart(i);
                    //Get body type
                    String bodyType = bodyPart.getContentType();
                    //Check type
                    if (bodyType.equals("application/sdp"))
                    {
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
                Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
        //Parse sdp
        processSDP(sdp);
    }

    public void processSDP(String content)
    {
        //Search for the ip
        int i = content.indexOf("\r\nc=IN IP4 ");
        int j = content.indexOf("\r\n", i+1);
        //Get the ip
        String ip = content.substring(i + 11, j);
        setSendIp(ip);
        //Check if ip should be nat for this media mixer
        if (conf.getMixer().isNated(ip))
            //Do natting
            ip = "0.0.0.0";

        //Disable supported media
        audioSupported = false;
        videoSupported = false;
        textSupported = false;

        //Search the next media
        //int m = content.indexOf("\r\nm=", j);
        int m = content.indexOf("\r\nm=");

        //Find bandwith
        int b = content.indexOf("\r\nb=");

        //If it is before the first media
        if (b>0 && b<m)
        {
            //Get :
            i = content.indexOf(":",b);
            //Get end of line
            j = content.indexOf("\r\n", i);
            //Get bandwith modifier
            String bandwith = content.substring(b+4, i);
            //Get bitrate value
            videoBitrate = Integer.parseInt(content.substring(i+1, j));
            // Let some room for audio.
            if (videoBitrate>=128)
                //Remove maximum rate
                videoBitrate -= 64;
        } else {
            //NO bitrate by default
            videoBitrate = 0;
        }

        //Search medias
        while (m>0)
        {
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
            String transport = content.substring(i,j);

           //Search format list
            i = j + 1;
            j = content.indexOf("\r\n", i);
            //Get it
            String fmtString = content.substring(i, j);

            //Get fmts
            StringTokenizer fmts  = new StringTokenizer(fmtString);

            //Find bandwith modifier for media
            b = content.indexOf("\r\nb=",ini);

            //If it is in this media
            if (b>0 && b<m)
            {
                //Get :
                i = content.indexOf(":", b);
                //Get end of line
                j = content.indexOf("\r\n", i);
                //Get bandwith modifier
                String bandwith = content.substring(b+4, i);
                //Get bitrate value
                mediaBitrate = Integer.parseInt(content.substring(i+1, j));
            }
            int inactive = content.indexOf("\r\na=inactive",ini);
            if(inactive > 0 && (inactive < m || m < 0)) {
                port = "0";
            }
            int sendonly = content.indexOf("\r\na=sendonly",ini);
            if(sendonly > 0 && (sendonly < m || m < 0)) {
                port = "0";
            }
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
                //Store bitrate
                videoBitrate = mediaBitrate;
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
                if (conf.getMixer().isNated(mediaIp))
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
                            }                            //Check if we have a lower priority
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
                            }                            //Check if we have a lower priority
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
    }

    public void onInfoRequest(SipServletRequest request) throws IOException {
        //Check content type
        if (request.getContentType().equals("application/media_control+xml"))
        {
            //Send FPU
            sendFPU();
            //Send OK
            SipServletResponse req = request.createResponse(200, "OK");
            //Send it
            req.send();
        } else {
            SipServletResponse req = request.createResponse(500, "Not supported");
            //Send it
            req.send();
        }
    }

    public void onCancelRequest(SipServletRequest request) {
        try {
            //Create final response
            SipServletResponse resp = request.createResponse(200, "Ok");
            //Send it
            resp.send();
        } catch (IOException ex) {
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Disconnect
        setState(State.DISCONNECTED);
        //Check cdr
        //And terminate
        destroy();
    }

    public void onCancelResponse(SipServletResponse resp) {
        //Teminate
        destroy();
    }

    public void onInfoResponse(SipServletResponse resp) {
    }

    public void onOptionsRequest(SipServletRequest request) {
        try {
            //Linphone and other UAs may send options before invite to determine public IP
            SipServletResponse response = request.createResponse(200);
            //return it
            response.send();
        } catch (IOException ex) {
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void onInviteRequest(SipServletRequest request) throws IOException {
        //Store address
        address = request.getFrom();
        //Get name
        name = getUsernameDomain();
        if(request.getHeader("desktop_share") != null)
            this.is_desktop_share = true;
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
        if (request.getContentLength()>0)
            //Process it
            proccesContent(request.getContentType(),request.getContent());
        //Store invite request
        inviteRequest = request;
        //Check if we need to autoaccapt
        if (isAutoAccept())
            //Accept it
            accept();
    }

    @Override
    public boolean accept() {
        //Check state
        if (state!=State.WAITING_ACCEPT)
        {
            //LOG
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.WARNING, "Accepted participant is not in WAITING_ACCEPT state  [id:{0},state:{1}].", new Object[]{id,state});
            //Error
            return false;
        }

        try {
            //Start receiving
            startReceiving();
            //Create final response
            SipServletResponse resp = inviteRequest.createResponse(200, "Ok");
            //Attach body
            resp.setContent(createSDP(),"application/sdp");
            String userName = SipEndPointManager.INSTANCE.getSipEndPointNameByNumber(this.getUsername());
            String userId = SipEndPointManager.INSTANCE.existNumber(this.getUsername());
            if(userName != null)
            {
                resp.addHeader("username", userName);
                resp.addHeader("userid", id.toString());
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
                Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
            } catch (IOException ex1) {
                //Log
                Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex1);
            }
            //Terminate
            error(State.ERROR, "Error");
            //Error
            return false;
        }
        //Ok
        return true;
    }

    @Override
    public boolean reject(Integer code, String reason) {
        //Check state
        if (state!=State.WAITING_ACCEPT)
        {
            //LOG
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.WARNING, "Rejected participant is not in WAITING_ACCEPT state [id:{0},state:{1}].", new Object[]{id,state});
            //Error
            return false;
        }

        try {
            //Create final response
            SipServletResponse resp = inviteRequest.createResponse(code, reason);
            //Send it
            resp.send();
        } catch (IOException ex1) {
            //Log
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex1);
            //Terminate
            error(State.ERROR, "Error");
            //Error
            return false;
        }
        //Terminate
        error(State.DECLINED,"Rejected");
        //Exit
        return true;
    }

    void doInvite(SipFactory sf, Address from,Address to) throws IOException, XmlRpcException {
        doInvite(sf,from,to,null,1,null);
    }

    void doInvite(SipFactory sf, Address from,Address to,int timeout) throws IOException, XmlRpcException {
        doInvite(sf,from,to,null,timeout,null);
    }

    void doInvite(SipFactory sf, Address from,Address to,SipURI proxy,int timeout,String location) throws IOException, XmlRpcException {
        //Store to as participant address
        address = to;
        //Start receiving media
        startReceiving();
        //Create the application session
        appSession = sf.createApplicationSession();
        // create an INVITE request to the first party from the second
        inviteRequest = sf.createRequest(appSession, "INVITE", from, to);
        //Check if we have a proxy
        if (proxy!=null)
            //Set proxy
            inviteRequest.pushRoute(proxy);
        //Get sip session
        session = inviteRequest.getSession();
        //Set reference in sessions
        appSession.setAttribute("user", this);
        session.setAttribute("user", this);
        //Do not invalidate
        appSession.setInvalidateWhenReady(false);
        session.setInvalidateWhenReady(false);
        //Set expire time
        appSession.setExpires(timeout);
        //Create sdp
        String sdp = createSDP();
        //If it has location info
        if (location!=null && !location.isEmpty())
        {
            try {
                //Get SIP uri of calling user
                SipURI uri = (SipURI)from.getURI();
                //Add location header
                inviteRequest.addHeader("Geolocation","<cid:"+uri.getUser()+"@"+uri.getHost()+">;routing-allowed=yes");
                inviteRequest.addHeader("Geolocation-Routing","yes");
                //Create multipart body
                Multipart body = new MimeMultipart();
                //Create sdp body
                BodyPart sdpPart = new MimeBodyPart();
                //Set content
                sdpPart.setContent(sdp, "application/sdp");
                //Set content headers
                sdpPart.setHeader("Content-Type","application/sdp");
                sdpPart.setHeader("Content-Length", Integer.toString(sdp.length()));
                //Add sdp
                body.addBodyPart(sdpPart);
                //Create slocation body
                BodyPart locationPart = new MimeBodyPart();
                //Set content
                locationPart.setContent(location, "application/pidf+xml");
                //Set content headers
                locationPart.setHeader("Content-Type","application/pidf+xml");
                locationPart.setHeader("Content-ID","<"+uri.getUser()+"@"+uri.getHost()+">");
                locationPart.setHeader("Content-Length", Integer.toString(location.length()));
                //Add sdp
                body.addBodyPart(locationPart);
                //Add content
                inviteRequest.setContent(body, body.getContentType().replace(" \r\n\t",""));
            } catch (MessagingException ex) {
                Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else {
            //Attach body
            inviteRequest.setContent(sdp,"application/sdp");
        }
        String userName = SipEndPointManager.INSTANCE.getSipEndPointNameByNumber(this.getUsername());
        String userId = SipEndPointManager.INSTANCE.existNumber(this.getUsername());
        if(userName != null)
        {
            inviteRequest.addHeader("username", userName);
            inviteRequest.addHeader("userid", id.toString());
        }
        inviteRequest.setHeader("confUid", conf.getUID());
        //Send it
        inviteRequest.send();
        //Log
        Logger.getLogger(RTPParticipant.class.getName()).log(Level.WARNING, "doInvite [idSession:{0}]",new Object[]{session.getId()});
        //Set state
        setState(State.CONNECTING);
        //Check cdr
    }

    public void onInviteResponse(SipServletResponse resp) throws IOException {
        //Check state
        if (state!=State.CONNECTING)
        {
            //Log
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.WARNING, "onInviteResponse while not CONNECTING [id:{0},state:{1}]",new Object[]{id,state});
            //Exit
            return;
        }
        //Get code
        Integer code = resp.getStatus();
        //Check response code
        if (code<200) {
            //Check code
            switch (code)
            {
                case 180:
                    break;
                default:
                    //DO nothing
            }
        } else if (code >= 200 && code < 300) {
            //Extend expire time one minute
            appSession.setExpires(1);
            //Update name
            address = resp.getTo();
            //Update name
            name = getUsernameDomain();
            //Parse sdp
            processSDP(new String((byte[])resp.getContent()));
            try {
                //Create ringing
                SipServletRequest ack = resp.createAck();
                String doRoute = System.getProperty("DoRoute");
                if(doRoute != null) {
                    SipURI sipUri = SipEndPointManager.INSTANCE.getSipEndPointUriByNumber(this.getUsername());
                    if(sipUri != null) {                   
                        ack.pushRoute(sipUri);
                    }
                }
                //Send it
                ack.send();
                //Set state before joining
                setState(State.CONNECTED);
                //Join it to the conference
                conf.joinParticipant(this);
                //Start sending
                startSending();
            } catch (XmlRpcException ex) {
                Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
                //Terminate
                error(State.ERROR,"Error");
            }
        } else if (code>=400) {
            //Check code
            switch (code) {
                case 404:
                    //Terminate
                    error(State.NOTFOUND,"NOT_FOUND");
                    break;
                case 486:
                    //Terminate
                    error(State.BUSY,"BUSY");
                    break;
                case 603:
                    //Terminate
                    error(State.DECLINED,"DECLINED");
                    break;
                case 408:
                case 480:
                case 487:
                    //Terminate
                    error(State.TIMEOUT,"TIMEOUT",code);
                    break;
                default:
                    //Terminate
                    error(State.ERROR,"ERROR",code);
                    break;
            }
            //Set expire time
            if (appSession!=null && appSession.isValid())
            {
                appSession.setExpires(1);
            }
        }
    }

    public void onTimeout() {
        //Check state
        if (state==State.CONNECTED) {
            //Extend session two minutes
            appSession.setExpires(1);
            //Get statiscits
            stats = conf.getParticipantStats(id);
            //Calculate acumulated packets
            Integer num = 0;
            //For each media
            for (MediaStatistics s : stats.values())
                //Increase packet count
                num += s.numRecvPackets;
            //Check
            if (!num.equals(totalPacketCount)) {
                //Update
                totalPacketCount = num;
            }  else {
                //Terminate
                error(State.TIMEOUT,"TIMEOUT",id);
            }
        } else if (state==State.CONNECTING) {
            //Cancel request
            doCancel(true);
        } else {
            //Teminate
            destroy();
        }
    }

    public void onAckRequest(SipServletRequest request) throws IOException {
        //Check if it has content
        if (request.getContentLength()>0)
            //Process it
            proccesContent(request.getContentType(),request.getContent());
        try {
            //Set state before joining
            setState(State.CONNECTED);
            //Join it to the conference
            conf.joinParticipant(this);
            //Start sending
            startSending();

        } catch (XmlRpcException ex) {
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    void doCancel(boolean timedout) {
        try{
            //Create BYE request
            SipServletRequest req = inviteRequest.createCancel();
            //Send it
            req.send();
        } catch(IOException ex){
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        } catch(IllegalStateException ex){
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Set expire time
        appSession.setExpires(1);
        //Check which state we have to set
        if (timedout)
            //TImeout
            setState(State.TIMEOUT);
        else
            //Disconnected
            setState(State.DISCONNECTED);
        //Terminate
        destroy();
    }

    void doBye() {
        try{
            //Create BYE request
            SipServletRequest req = session.createRequest("BYE");
            String doRoute = System.getProperty("DoRoute");
            if(doRoute != null) {
                SipURI sipUri = SipEndPointManager.INSTANCE.getSipEndPointUriByNumber(this.getUsername());
                if(sipUri != null) {                   
                    req.pushRoute(sipUri);
                }
            }
            //Send it
            req.send();
        } catch(IOException ex){
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        } catch(IllegalStateException ex){
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        }
        try {
            //Set expire time
            appSession.setExpires(1);
        } catch (Exception ex) {
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, "Error expiring user", ex);
        }
        //Disconnect
        setState(State.DISCONNECTED);
        //Terminate
        destroy();
    }

    public void onByeResponse(SipServletResponse resp) {
    }

    public void onByeRequest(SipServletRequest request) {
        try {
            //Create final response
            SipServletResponse resp = request.createResponse(200, "Ok");
            //Send it
            resp.send();
        } catch (IOException ex) {
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Set expire time
        appSession.setExpires(1);
        //Disconnect
        setState(State.DISCONNECTED);
        //Terminate
        destroy();
    }

    @Override
    public void end() {
        //Log
        Logger.getLogger(RTPParticipant.class.getName()).log(Level.INFO, "Ending RTP user id:{0} in state {1}", new Object[]{id,state});
        //Depending on the state
        switch (state)
        {
            case CONNECTING:
                doCancel(false);
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

    @Override
    public void destroy() {
        //Logger.getLogger("global").log(Level.SEVERE, "destroy participant " + id);
        //Check if already was destroying
        synchronized( this) {
            if (isDestroying) //Do nothing
            {
                return;
            }
            //We are isDestroying
            isDestroying = true;
        }
        try {
            //Get client
            //XmlRpcMcuClient client = conf.getMCUClient();
            //Delete participant
            client.DeleteParticipant(conf.getId(), id);
        } catch (XmlRpcException ex) {
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        }

        try {
            //If ther was a session
            if (session!=null && session.isValid())
            {
                //Remove participant from session
                session.removeAttribute("user");
                //Invalidate the session when appropiate
                session.setInvalidateWhenReady(true);
            }
            //If there was an application session
            if (appSession!=null && appSession.isValid())
            {
                //Remove participant from session
                appSession.removeAttribute("user");
                //Set expire time to let it handle any internal stuff
                appSession.setExpires(1);
                //Invalidate the session when appropiate
                appSession.setInvalidateWhenReady(true);
            }
        } catch (Exception ex) {
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Set state
        setState(State.DESTROYED);
        conf.getMixer().releaseMcuClient(client);
        for(Spyer spyer : spyers.values()){
            spyer.end();
        }
        spyers.clear();
    }

    public void startSending() throws XmlRpcException {
        //Get client
        //XmlRpcMcuClient client = conf.getMCUClient();
        //Get conf id
        Integer confId = conf.getId();

        //Check audio
        if (getSendAudioPort()!=0)
        {
            //Set codec
            client.SetAudioCodec(confId, id, getAudioCodec());
            //Send
            client.StartSendingAudio(confId, id, getSendAudioIp(), getSendAudioPort(), getRtpOutMediaMap("audio"));
            //Sending Audio
            isSendingAudio = true;
        }

        //Check video
        if (getSendVideoPort()!=0)
        {
            //Get profile bitrat
            int bitrate = profile.getVideoBitrate();
            //Reduce to the maximum in SDP
            if (videoBitrate>0 && videoBitrate<bitrate)
                    //Reduce it
                    bitrate = videoBitrate;
            //Set codec
            client.SetVideoCodec(confId, id, getVideoCodec(), profile.getVideoSize() , profile.getVideoFPS(), bitrate, 0, 0, profile.getIntraPeriod());
            //Send
            client.StartSendingVideo(confId, id, getSendVideoIp(), getSendVideoPort(), getRtpOutMediaMap("video"));
            //Sending Video
            isSendingVideo = true;
        }

        //Check text
        if (getSendTextPort()!=0)
        {
            //Set codec
            client.SetTextCodec(confId, id, getTextCodec());
            //Send
            client.StartSendingText(confId, id, getSendTextIp(), getSendTextPort(), getRtpOutMediaMap("text"));
            //Sending Text
            isSendingText = true;
        }
    }

    public void startReceiving() throws XmlRpcException {
        //Get client
        //XmlRpcMcuClient client = conf.getMCUClient();
        //Get conf id
        Integer confId = conf.getId();

        //If supported
        if (getAudioSupported())
        {
            //Create rtp map for audio
            createRTPMap("audio");
            //Get receiving ports
            recAudioPort = client.StartReceivingAudio(confId, id, getRtpInMediaMap("audio"));
        }

        //If supported
        if (getVideoSupported())
        {
            //Create rtp map for video
            createRTPMap("video");
            //Get receiving ports
            recVideoPort = client.StartReceivingVideo(confId, id, getRtpInMediaMap("video"));
        }

        //If supported
        if (getTextSupported())
        {
            //Create rtp map for text
            createRTPMap("text");
            //Get receiving ports
            recTextPort = client.StartReceivingText(confId, id, getRtpInMediaMap("text"));
        }

        //And ip
        setRecIp(conf.getRTPIp());
    }

   public void sendFPU() {
        //Get client
        //XmlRpcMcuClient client = conf.getMCUClient();
        //Get id
        Integer confId = conf.getId();
        try {
            //Send fast pcture update
            client.SendFPU(confId, id);
        } catch (XmlRpcException ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    @Override
    void requestFPU() {
        //Send FPU
        String xml ="<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n<media_control>\r\n<vc_primitive>\r\n<to_encoder>\r\n<picture_fast_update></picture_fast_update>\r\n</to_encoder>\r\n</vc_primitive>\r\n</media_control>\r\n";
        try {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "Send INFO message");
             //Create ack
            SipServletRequest info = session.createRequest("INFO");
            String doRoute = System.getProperty("DoRoute");
            if(doRoute != null) {
                SipURI sipUri = SipEndPointManager.INSTANCE.getSipEndPointUriByNumber(this.getUsername());
                if(sipUri != null) {                   
                    info.pushRoute(sipUri);
                }
            }
            //Set content
            info.setContent(xml, "application/media_control+xml");
            //Send it
            info.send();
        } catch (IOException ex) {
            //Log it
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, "Error while requesting FPU for participant", ex);
        }
    }
	
	 public void sendMemberNofity(State state, Integer partId){
         //Send FPU
        //String xml ="<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n<media_control>\r\n<vc_primitive>\r\n<to_encoder>\r\n<picture_fast_update></picture_fast_update>\r\n</to_encoder>\r\n</vc_primitive>\r\n</media_control>\r\n";
        try {
             //Create ack
            SipServletRequest notify = session.createRequest("NOTIFY");
            notify.addHeader("Event", state.toString());
            notify.setHeader("partId", partId.toString());
            notify.setHeader("confId", conf.getId().toString());
            notify.setHeader("confUid", conf.getUID());
            notify.setHeader("partNumber", this.getUsername());
            String doRoute = System.getProperty("DoRoute");
            if(doRoute != null) {
                SipURI sipUri = SipEndPointManager.INSTANCE.getSipEndPointUriByNumber(this.getUsername());
                if(sipUri != null) {                   
                    notify.pushRoute(sipUri);
                }
            }
            notify.send();
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.WARNING, "Sending Nofity for participant");
            
        } catch (IOException ex) {
            //Log it
            Logger.getLogger(RTPParticipant.class.getName()).log(Level.SEVERE, "Error while requesting Nofity for participant", ex);
        }
        
    }

    public void onStateChanged(Spyer part, Spyer.State state) {
        if (state.equals(Spyer.State.DESTROYED)) {
            spyers.remove(part.getSpyer_number());
        }
    }
}
